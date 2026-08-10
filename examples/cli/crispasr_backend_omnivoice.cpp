// crispasr_backend_omnivoice.cpp — unified realtime OmniVoice adapter.
//
// Uses the complete native pipeline: VoiceDesign, 600+ language aliases,
// reference-audio cloning, cancellation-ready streaming and the standalone
// RVQ codec. The companion codec can be supplied explicitly or discovered
// beside the language-model GGUF.

#include "crispasr_backend.h"
#include "crispasr_backend_utils.h"
#include "whisper_params.h"

#include "core/audio_resample.h"
#include "core/crispasr_env.h"
#include "core/wav_reader.h"
#include "omnivoice/omnivoice.h"

#include <cstdlib>
#include <cstdio>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace {

constexpr int kOmniVoiceGpuSteps = 16;

bool file_exists(const std::string & path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

std::string model_dir(const std::string & path) {
    const size_t sep = path.find_last_of("/\\");
    return sep == std::string::npos ? "." : path.substr(0, sep);
}

std::string discover_codec(const std::string & model_path) {
    const std::string dir = model_dir(model_path);
    static const char * names[] = {
        "omnivoice-tokenizer.gguf",
        "omnivoice-tokenizer-f16.gguf",
        "omnivoice-tokenizer-BF16.gguf",
        "omnivoice-audio-tokenizer.gguf",
    };
    for (const char * name : names) {
        const std::string candidate = dir + "/" + name;
        if (file_exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

bool is_wav_path(const std::string & path) {
    return path.size() >= 4 &&
           (path.compare(path.size() - 4, 4, ".wav") == 0 || path.compare(path.size() - 4, 4, ".WAV") == 0);
}

bool omnivoice_stream_chunk(const float * samples, int n_samples, void * user_data) {
    auto * cb = static_cast<CrispasrBackend::crispasr_pcm_stream_callback *>(user_data);
    if (cb && samples && n_samples > 0) {
        (*cb)(samples, n_samples, false);
    }
    return true;
}

class OmniVoiceBackend : public CrispasrBackend {
public:
    ~OmniVoiceBackend() override { shutdown(); }

    const char * name() const override { return "omnivoice"; }
    uint32_t capabilities() const override { return CAP_TTS | CAP_VOICE_CLONING | CAP_STREAMING; }

    std::vector<crispasr_segment> transcribe(const float *, int, int64_t, const whisper_params &) override {
        fprintf(stderr, "crispasr[omnivoice]: transcription is not supported by this backend\n");
        return {};
    }

    bool init(const whisper_params & p) override {
        std::string codec_path = p.tts_codec_model;
        if (codec_path.empty() || codec_path == "auto" || codec_path == "default") {
            codec_path = discover_codec(p.model);
        }
        if (codec_path.empty()) {
            fprintf(stderr, "crispasr[omnivoice]: --codec-model <codec.gguf> is required (or place a tokenizer beside the model)\n");
            return false;
        }

        ov_init_params ip;
        ov_init_default_params(&ip);
        ip.model_path = p.model.c_str();
        ip.codec_path = codec_path.c_str();
        ip.use_fa = p.flash_attn && crispasr_backend_should_use_gpu(p);
        ip.clamp_fp16 = false;

        ctx_ = ov_init(&ip);
        if (!ctx_) {
            fprintf(stderr, "crispasr[omnivoice]: failed to load model '%s' with codec '%s': %s\n",
                    p.model.c_str(), codec_path.c_str(), ov_last_error());
            return false;
        }

        // Diff-harness: OMNIVOICE_ENCODE_DIFF=<ref.gguf> runs the encode-path
        // stage diff and exits (#254 voice-clone port validation).
        if (const char* rp = crispasr_env::get("CRISPASR_OMNIVOICE_ENCODE_DIFF")) {
            const int rc = ov_encode_diff(ctx_, rp);
            std::exit(rc == 0 ? 0 : 1);
        }
        return true;
    }

    std::vector<float> synthesize(const std::string & text, const whisper_params & params) override {
        if (!ctx_ || text.empty()) {
            return {};
        }

        ov_tts_params tp;
        ov_tts_default_params(&tp);
        fill_tts_params(text, params, &tp);
        ov_audio audio = {};
        if (ov_synthesize(ctx_, &tp, &audio) != OV_STATUS_OK) {
            fprintf(stderr, "crispasr[omnivoice]: synthesis failed: %s\n", ov_last_error());
            return {};
        }
        std::vector<float> out(audio.samples, audio.samples + audio.n_samples);
        ov_audio_free(&audio);
        return out;
    }

    void synthesize_streaming(const std::string & text, const whisper_params & params,
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
        if (ov_synthesize(ctx_, &tp, nullptr) != OV_STATUS_OK) {
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
    void fill_tts_params(const std::string & text, const whisper_params & params, ov_tts_params * tp) {
        tp->text = text.c_str();
        const std::string & target_lang = !params.target_lang.empty() ? params.target_lang : params.language;
        tp->lang = target_lang == "auto" ? "" : target_lang.c_str();
        tp->instruct = params.tts_instruct.empty() ? "" : params.tts_instruct.c_str();
        tp->denoise = true;
        tp->preprocess_prompt = true;
        tp->speed = params.tts_speed > 0.0f ? params.tts_speed : 1.0f;
        if (params.seed != 0) {
            tp->mg_seed = (uint64_t) params.seed;
        }
        tp->ref_text = params.tts_ref_text.empty() ? "" : params.tts_ref_text.c_str();
        if (params.tts_num_steps > 0) {
            tp->mg_num_step = params.tts_num_steps;
        } else if (crispasr_backend_should_use_gpu(params)) {
            tp->mg_num_step = kOmniVoiceGpuSteps;
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
                    ref_audio_24k_ = core_audio::resample_polyphase(wav.data(), (int) wav.size(), sr, 24000);
                } else {
                    ref_audio_24k_ = std::move(wav);
                }
                tp->ref_audio_24k = ref_audio_24k_.data();
                tp->ref_n_samples = (int) ref_audio_24k_.size();
            } else {
                fprintf(stderr, "crispasr[omnivoice]: failed to load reference WAV '%s'\n", params.tts_voice.c_str());
            }
        }
    }

    ov_context * ctx_ = nullptr;
    std::vector<float> ref_audio_24k_;
};

} // namespace

std::unique_ptr<CrispasrBackend> crispasr_make_omnivoice_backend() {
    return std::make_unique<OmniVoiceBackend>();
}
