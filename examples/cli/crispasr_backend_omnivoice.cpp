// crispasr_backend_omnivoice.cpp — adapter for OmniVoice TTS.
//
// OmniVoice is imported as an isolated native GGML runtime under
// src/omnivoice/. The adapter keeps CrispASR's normal TTS surface:
// --model is the OmniVoice LM GGUF, --codec-model is the codec GGUF,
// --voice may be a WAV reference, --ref-text provides its transcript, and
// --instruct maps to OmniVoice's VoiceDesign prompt.

#include "crispasr_backend.h"
#include "crispasr_backend_utils.h"
#include "whisper_params.h"

#include "core/audio_resample.h"
#include "core/wav_reader.h"
#include "omnivoice/omnivoice.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

constexpr int kOmniVoiceRealtimeGpuSteps = 16;

bool is_wav_path(const std::string& path) {
    return path.size() >= 4 &&
           (path.compare(path.size() - 4, 4, ".wav") == 0 || path.compare(path.size() - 4, 4, ".WAV") == 0);
}

bool omnivoice_stream_chunk(const float* samples, int n_samples, void* user_data) {
    auto* cb = static_cast<CrispasrBackend::crispasr_pcm_stream_callback*>(user_data);
    if (cb && samples && n_samples > 0) {
        (*cb)(samples, n_samples, false);
    }
    return true;
}

class OmniVoiceBackend : public CrispasrBackend {
public:
    OmniVoiceBackend() = default;
    ~OmniVoiceBackend() override { OmniVoiceBackend::shutdown(); }

    const char* name() const override { return "omnivoice"; }

    uint32_t capabilities() const override { return CAP_TTS | CAP_VOICE_CLONING | CAP_STREAMING; }

    std::vector<crispasr_segment> transcribe(const float* /*samples*/, int /*n_samples*/, int64_t /*t_offset_cs*/,
                                             const whisper_params& /*params*/) override {
        fprintf(stderr, "crispasr[omnivoice]: transcription is not supported by this backend\n");
        return {};
    }

    bool init(const whisper_params& p) override {
        if (p.tts_codec_model.empty()) {
            fprintf(stderr, "crispasr[omnivoice]: --codec-model <codec.gguf> is required\n");
            return false;
        }

        ov_init_params ip;
        ov_init_default_params(&ip);
        ip.model_path = p.model.c_str();
        ip.codec_path = p.tts_codec_model.c_str();
        ip.use_fa = !p.flash_attn ? false : crispasr_backend_should_use_gpu(p);
        ip.clamp_fp16 = false;

        ctx_ = ov_init(&ip);
        if (!ctx_) {
            fprintf(stderr, "crispasr[omnivoice]: failed to load model '%s' with codec '%s': %s\n", p.model.c_str(),
                    p.tts_codec_model.c_str(), ov_last_error());
            return false;
        }
        return true;
    }

    std::vector<float> synthesize(const std::string& text, const whisper_params& params) override {
        if (!ctx_ || text.empty()) {
            return {};
        }

        ov_tts_params tp;
        ov_tts_default_params(&tp);
        fill_tts_params(text, params, &tp);

        ov_audio audio = {};
        ov_status rc = ov_synthesize(ctx_, &tp, &audio);
        if (rc != OV_STATUS_OK) {
            fprintf(stderr, "crispasr[omnivoice]: synthesis failed: %s\n", ov_last_error());
            return {};
        }

        std::vector<float> out;
        if (audio.samples && audio.n_samples > 0) {
            out.assign(audio.samples, audio.samples + audio.n_samples);
        }
        ov_audio_free(&audio);
        return out;
    }

    void synthesize_streaming(const std::string& text, const whisper_params& params,
                              crispasr_pcm_stream_callback cb) override {
        if (!ctx_ || text.empty()) {
            cb(nullptr, 0, true);
            return;
        }

        ov_tts_params tp;
        ov_tts_default_params(&tp);
        fill_tts_params(text, params, &tp);
        tp.on_chunk = omnivoice_stream_chunk;
        tp.on_chunk_user_data = &cb;

        ov_status rc = ov_synthesize(ctx_, &tp, nullptr);
        if (rc != OV_STATUS_OK) {
            fprintf(stderr, "crispasr[omnivoice]: streaming synthesis failed: %s\n", ov_last_error());
        }
        cb(nullptr, 0, true);
    }

    int tts_sample_rate() const override { return 24000; }

    void shutdown() override {
        if (ctx_) {
            ov_free(ctx_);
            ctx_ = nullptr;
        }
    }

private:
    void fill_tts_params(const std::string& text, const whisper_params& params, ov_tts_params* tp) {
        tp->text = text.c_str();
        tp->lang = (!params.language.empty() && params.language != "auto") ? params.language.c_str() : "";
        tp->instruct = params.tts_instruct.empty() ? "" : params.tts_instruct.c_str();
        tp->denoise = true;
        tp->preprocess_prompt = true;
        tp->mg_seed = (uint64_t)params.seed;
        tp->ref_text = params.tts_ref_text.empty() ? "" : params.tts_ref_text.c_str();

        if (params.tts_num_steps > 0) {
            tp->mg_num_step = params.tts_num_steps;
        } else if (crispasr_backend_should_use_gpu(params)) {
            tp->mg_num_step = kOmniVoiceRealtimeGpuSteps;
        } else if (params.tts_steps > 0) {
            tp->mg_num_step = params.tts_steps;
        }
        if (params.tts_cfg_scale >= 0.0f) {
            tp->mg_guidance_scale = params.tts_cfg_scale;
        }

        ref_audio_24k_.clear();
        if (!params.tts_voice.empty() && is_wav_path(params.tts_voice)) {
            std::vector<float> wav;
            int sr = 0;
            if (crispasr::core::read_wav_mono_pcm16(params.tts_voice, wav, sr)) {
                if (sr != 24000 && sr > 0) {
                    ref_audio_24k_ = core_audio::resample_polyphase(wav.data(), (int)wav.size(), sr, 24000);
                } else {
                    ref_audio_24k_ = std::move(wav);
                }
                tp->ref_audio_24k = ref_audio_24k_.data();
                tp->ref_n_samples = (int)ref_audio_24k_.size();
            } else {
                fprintf(stderr, "crispasr[omnivoice]: failed to load reference WAV '%s'; using VoiceDesign path\n",
                        params.tts_voice.c_str());
            }
        }
    }

    ov_context* ctx_ = nullptr;
    std::vector<float> ref_audio_24k_;
};

} // namespace

std::unique_ptr<CrispasrBackend> crispasr_make_omnivoice_backend() {
    return std::make_unique<OmniVoiceBackend>();
}
