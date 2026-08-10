// omnivoice.cpp: public ABI implementation.
//
// Every entry declared in omnivoice.h lives here under one extern "C" block
// so the symbols carry C linkage and are linkable from C, Rust, Go, Python
// ctypes and any other binding generator. The struct ov_context opaque
// handle owns one BackendPair, one PipelineTTS, one PipelineCodec
// (optional), one BPETokenizer and one VoiceDesign instance. ov_init walks
// the load chain in dependency order and unwinds whatever it already
// allocated when any step fails. ov_free mirrors that order in reverse.

#include "omnivoice.h"

#include "audio-postproc.h"
#include "backend.h"
#include "bpe.h"
#include "duration-estimator.h"
#include "ov-error.h"
#include "pipeline-codec.h"
#include "pipeline-tts.h"
#include "maskgit-tts.h"
#include "text-chunker.h"
#include "version.h"
#include "voice-design.h"

#include "core/audio_resample.h"
#include "core/crispasr_env.h"
#include "core/omnivoice_instruct.h"
#include "core/omnivoice_lang.h"
#include "core/wav_reader.h"

#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

// Internal definition of the opaque handle. C++ types are fine here
// because nothing in this struct ever crosses the public ABI boundary :
// callers only ever see `struct ov_context *`.
struct ov_context {
    BackendPair   bp;
    PipelineTTS   pt;
    PipelineCodec pc;
    BPETokenizer  tok;
    VoiceDesign   vd;
    bool          codec_loaded;
};

// Thread-local backing store for ov_last_error(). std::string sized once
// per thread, grows on demand, never freed across calls: the std runtime
// reclaims it on thread exit. An empty string means "no error recorded on
// this thread yet", which ov_last_error() exposes as "".
static thread_local std::string g_last_error;

void ov_set_error_v(const char * fmt, va_list ap) {
    if (!fmt) {
        g_last_error.clear();
        return;
    }
    // Two-pass vsnprintf: first call sizes the buffer, second writes the
    // message. va_copy keeps the original ap valid for the second pass.
    va_list ap2;
    va_copy(ap2, ap);
    int needed = std::vsnprintf(nullptr, 0, fmt, ap2);
    va_end(ap2);
    if (needed < 0) {
        g_last_error = "ov_set_error: vsnprintf failed";
        return;
    }
    g_last_error.resize(static_cast<size_t>(needed));
    std::vsnprintf(g_last_error.data(), static_cast<size_t>(needed) + 1, fmt, ap);
}

void ov_set_error(const char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    ov_set_error_v(fmt, ap);
    va_end(ap);
}

// Formats a message with printf semantics and throws std::runtime_error.
// The catch sites at the ABI boundary inspect the what() string and feed
// it into ov_set_error so the user-visible diagnostic is identical
// whether the failure used the bool-return path or the throw path.
void ov_throw(const char * fmt, ...) {
    char buf[1024];
    if (fmt) {
        va_list ap;
        va_start(ap, fmt);
        std::vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
    } else {
        buf[0] = '\0';
    }
    throw std::runtime_error(buf);
}

// Process-wide log callback. Atomic so ov_log_set can replace it without
// locking: write happens with memory_order_release, every reader sees a
// fully published callback pointer paired with its user_data slot.
// std::atomic on a function pointer is lock-free on every platform we
// target. user_data is a plain pointer because it is only ever published
// alongside cb under the same release ordering.
static std::atomic<ov_log_cb> g_log_cb{ nullptr };
static void *                 g_log_cb_user = nullptr;

void ov_log_set(ov_log_cb cb, void * user_data) {
    g_log_cb_user = user_data;
    g_log_cb.store(cb, std::memory_order_release);
}

// Routes one log line to the installed callback or to stderr. Two-pass
// vsnprintf sizes the heap buffer when the message exceeds the stack
// scratchpad, which keeps the common case allocation-free.
void ov_log(enum ov_log_level level, const char * fmt, ...) {
    if (!fmt) {
        return;
    }

    char    stackbuf[512];
    char *  buf    = stackbuf;
    int     needed = 0;
    va_list ap;
    va_start(ap, fmt);
    {
        va_list ap2;
        va_copy(ap2, ap);
        needed = std::vsnprintf(stackbuf, sizeof(stackbuf), fmt, ap2);
        va_end(ap2);
    }
    if (needed < 0) {
        va_end(ap);
        return;
    }
    std::string heapbuf;
    if ((size_t) needed >= sizeof(stackbuf)) {
        heapbuf.resize((size_t) needed);
        std::vsnprintf(heapbuf.data(), (size_t) needed + 1, fmt, ap);
        buf = heapbuf.data();
    }
    va_end(ap);

    ov_log_cb cb = g_log_cb.load(std::memory_order_acquire);
    if (cb) {
        cb(level, buf, g_log_cb_user);
    } else {
        std::fprintf(stderr, "%s\n", buf);
    }
}

extern "C" {

const char * ov_version(void) {
    // OMNIVOICE_VERSION is a string literal injected by tools/version.cmake
    // ("<git-hash> (<date>)"), so its storage already has process lifetime
    // and no formatting wrapper is needed.
    return OMNIVOICE_VERSION;
}

const char * ov_last_error(void) {
    // c_str() on an empty std::string is guaranteed to point to a NUL
    // byte by C++11, so callers never have to NULL-check the result.
    return g_last_error.c_str();
}

void ov_audio_free(struct ov_audio * a) {
    if (!a) {
        return;
    }
    if (a->samples) {
        std::free(a->samples);
    }
    a->samples     = nullptr;
    a->n_samples   = 0;
    a->sample_rate = 0;
    a->channels    = 0;
}

void ov_init_default_params(struct ov_init_params * p) {
    p->abi_version = OV_ABI_VERSION;
    p->model_path  = nullptr;
    p->codec_path  = nullptr;
    p->use_fa      = true;
    p->clamp_fp16  = false;
}

void ov_tts_default_params(struct ov_tts_params * p) {
    p->abi_version             = OV_ABI_VERSION;
    p->text                    = nullptr;
    p->lang                    = nullptr;
    p->instruct                = nullptr;
    p->T_override              = 0;
    p->chunk_duration_sec      = 15.0f;
    p->chunk_threshold_sec     = 30.0f;
    p->denoise                 = true;
    p->preprocess_prompt       = true;
    p->mg_num_step             = 32;
    p->mg_guidance_scale       = 2.0f;
    p->mg_t_shift              = 0.1f;
    p->mg_layer_penalty_factor = 5.0f;
    p->mg_position_temperature = 5.0f;
    p->mg_class_temperature    = 0.0f;
    p->mg_seed                 = 42;
    p->ref_audio_tokens        = nullptr;
    p->ref_T                   = 0;
    p->ref_audio_24k           = nullptr;
    p->ref_n_samples           = 0;
    p->ref_text                = nullptr;
    p->dump_dir                = nullptr;
    p->cancel                  = nullptr;
    p->cancel_user_data        = nullptr;
    p->on_chunk                = nullptr;
    p->on_chunk_user_data      = nullptr;
    p->postproc                = true;
    p->speed                   = 1.0f;
}

struct ov_context * ov_init(const struct ov_init_params * params) {
    if (!params || !params->model_path) {
        ov_set_error("ov_init: params or model_path is NULL");
        ov_log(OV_LOG_ERROR, "[OmniVoice] ov_init requires a model_path");
        return nullptr;
    }
    if (params->abi_version > OV_ABI_VERSION) {
        ov_set_error("ov_init: params->abi_version %d > OV_ABI_VERSION %d (binding compiled against a newer header)",
                     params->abi_version, OV_ABI_VERSION);
        ov_log(OV_LOG_ERROR, "[OmniVoice] ov_init params struct is from a newer ABI (%d > %d)", params->abi_version,
               OV_ABI_VERSION);
        return nullptr;
    }

    ov_log(OV_LOG_INFO, "[OmniVoice] omnivoice.cpp %s", ov_version());

    // new ov_context() value-initialises every field: POD aggregates
    // (BackendPair, PipelineTTS, PipelineCodec) are zero-init, std
    // containers in BPETokenizer construct empty, codec_loaded falls to
    // false. Only VoiceDesign needs explicit population below.
    ov_context * ov = new ov_context();
    voice_design_init(&ov->vd);

    // The load chain runs inside a try block. Any failure deep in the GGUF
    // reader, the audio tokenizer load or the LM weight load throws via
    // ov_throw; the catch funnels every variant into one cleanup via
    // ov_free, which is idempotent on partial state (NULL-safe sched, NULL
    // GGUF handles, refcount-correct backend release).
    try {
        ov->bp = backend_init("LM");
        if (!ov->bp.backend) {
            ov_throw("ov_init: backend_init failed (no GGML backend available)");
        }

        if (!pipeline_tts_load(&ov->pt, params->model_path, ov->bp, params->use_fa, params->clamp_fp16)) {
            ov_throw("ov_init: pipeline_tts_load failed for '%s'", params->model_path);
        }

        // BPE tokenizer payload lives inside the same LM GGUF as the weights.
        // Load the base vocab + the OmniVoice-specific special tokens in one
        // shot.
        if (!load_bpe_from_gguf(&ov->tok, params->model_path) ||
            !bpe_load_omnivoice_specials(&ov->tok, params->model_path)) {
            ov_throw("ov_init: BPE / OmniVoice specials load failed for '%s'", params->model_path);
        }

        if (params->codec_path) {
            if (!pipeline_codec_load(&ov->pc, params->codec_path, ov->bp)) {
                ov_throw("ov_init: pipeline_codec_load failed for '%s'", params->codec_path);
            }
            ov->codec_loaded = true;
        }
    } catch (const std::exception & e) {
        ov_set_error("%s", e.what());
        ov_log(OV_LOG_ERROR, "[OmniVoice] %s", e.what());
        ov_free(ov);
        return nullptr;
    }

    return ov;
}

void ov_free(struct ov_context * ov) {
    if (!ov) {
        return;
    }
    if (ov->codec_loaded) {
        pipeline_codec_free(&ov->pc);
    }
    pipeline_tts_free(&ov->pt);
    backend_release(ov->bp.backend, ov->bp.cpu_backend);
    delete ov;
}

enum ov_status ov_synthesize(struct ov_context * ov, const struct ov_tts_params * params, struct ov_audio * out) {
    if (!ov || !params) {
        ov_set_error("ov_synthesize: ov / params is NULL");
        if (out) {
            ov_audio_free(out);
        }
        return OV_STATUS_INVALID_PARAMS;
    }
    // Streaming mode (on_chunk non NULL) emits through the callback and
    // leaves out unused, so out=NULL is valid there. Buffered mode requires
    // out to receive the synthesised waveform.
    if (!params->on_chunk && !out) {
        ov_set_error("ov_synthesize: out is NULL in buffered mode");
        return OV_STATUS_INVALID_PARAMS;
    }
    if (params->abi_version > OV_ABI_VERSION) {
        ov_set_error(
            "ov_synthesize: params->abi_version %d > OV_ABI_VERSION %d (binding compiled against a newer header)",
            params->abi_version, OV_ABI_VERSION);
        if (out) {
            ov_audio_free(out);
        }
        return OV_STATUS_INVALID_PARAMS;
    }
    if (!ov->codec_loaded) {
        ov_set_error("ov_synthesize: codec not loaded (pass codec_path to ov_init)");
        if (out) {
            ov_audio_free(out);
        }
        ov_log(OV_LOG_ERROR, "[OmniVoice] ov_synthesize requires a codec-loaded handle");
        return OV_STATUS_INVALID_PARAMS;
    }
    // Defense in depth: the synthesis path normally reports failures via
    // ov_status return + ov_set_error. A future load-style throw or any
    // std::bad_alloc deep inside the GGML backend is caught here and
    // converted to OV_STATUS_GENERATE_FAILED so an exception never crosses
    // the extern "C" boundary.
    try {
        return pipeline_tts_synthesize(&ov->pt, &ov->pc, &ov->tok, &ov->vd, params, out);
    } catch (const std::exception & e) {
        ov_set_error("%s", e.what());
        ov_log(OV_LOG_ERROR, "[OmniVoice] %s", e.what());
        if (out) {
            ov_audio_free(out);
        }
        return OV_STATUS_GENERATE_FAILED;
    }
}

enum ov_status ov_set_codec_path(struct ov_context * ov, const char * codec_path) {
    if (!ov || !codec_path || !*codec_path) {
        ov_set_error("ov_set_codec_path: ov or codec_path is NULL/empty");
        return OV_STATUS_INVALID_PARAMS;
    }

    try {
        if (ov->codec_loaded) {
            pipeline_codec_free(&ov->pc);
            ov->codec_loaded = false;
        }
        if (!pipeline_codec_load(&ov->pc, codec_path, ov->bp)) {
            ov_set_error("ov_set_codec_path: pipeline_codec_load failed for '%s'", codec_path);
            return OV_STATUS_GENERATE_FAILED;
        }
        ov->codec_loaded = true;
        return OV_STATUS_OK;
    } catch (const std::exception & e) {
        ov_set_error("ov_set_codec_path: %s", e.what());
        ov_log(OV_LOG_ERROR, "[OmniVoice] %s", e.what());
        return OV_STATUS_GENERATE_FAILED;
    }
}

void ov_set_n_threads(struct ov_context * ov, int n_threads) {
    if (!ov || n_threads <= 0 || !ov->bp.cpu_backend) {
        return;
    }
    ggml_backend_dev_t dev = ggml_backend_get_device(ov->bp.cpu_backend);
    ggml_backend_reg_t reg = dev ? ggml_backend_dev_backend_reg(dev) : nullptr;
    if (reg) {
        auto set_fn = (ggml_backend_set_n_threads_t) ggml_backend_reg_get_proc_address(reg, "ggml_backend_set_n_threads");
        if (set_fn) {
            set_fn(ov->bp.cpu_backend, n_threads);
        }
    }
}

void ov_codes_free(int32_t * codes) {
    std::free(codes);
}

enum ov_status ov_synthesize_codes(struct ov_context * ov,
                                   const struct ov_tts_params * params,
                                   int32_t ** out_codes,
                                   int * out_n_codes) {
    if (out_codes) {
        *out_codes = nullptr;
    }
    if (out_n_codes) {
        *out_n_codes = 0;
    }
    if (!ov || !params || !out_codes || !out_n_codes) {
        ov_set_error("ov_synthesize_codes: ov, params or output is NULL");
        return OV_STATUS_INVALID_PARAMS;
    }
    if (params->abi_version > OV_ABI_VERSION) {
        ov_set_error("ov_synthesize_codes: params ABI %d is newer than %d", params->abi_version, OV_ABI_VERSION);
        return OV_STATUS_INVALID_PARAMS;
    }

    try {
        const std::string text(params->text ? params->text : "");
        const std::string raw_lang(params->lang ? params->lang : "");
        std::string lang;
        if (!pipeline_tts_resolve_language(text, raw_lang, &lang)) {
            ov_set_error("ov_synthesize_codes: failed to resolve target language");
            return OV_STATUS_INVALID_PARAMS;
        }
        std::string ref_text(params->ref_text ? params->ref_text : "");
        if (params->preprocess_prompt && !ref_text.empty()) {
            ref_text = add_punctuation(ref_text);
        }

        std::string instruct;
        if (!pipeline_tts_resolve_instruct(&ov->vd, text, params->instruct ? params->instruct : "", &instruct)) {
            ov_set_error("ov_synthesize_codes: instruct could not be resolved");
            return OV_STATUS_INSTRUCT_INVALID;
        }

        MaskgitConfig mg_cfg;
        mg_cfg.num_step             = params->mg_num_step;
        mg_cfg.guidance_scale      = params->mg_guidance_scale;
        mg_cfg.t_shift              = params->mg_t_shift;
        mg_cfg.layer_penalty_factor = params->mg_layer_penalty_factor;
        mg_cfg.position_temperature = params->mg_position_temperature;
        mg_cfg.class_temperature    = params->mg_class_temperature;
        mg_cfg.seed                 = params->mg_seed;
        if (const char * e = crispasr_env::get("CRISPASR_OMNIVOICE_GUIDANCE")) {
            mg_cfg.guidance_scale = (float) std::atof(e);
        }
        if (const char * e = crispasr_env::get("CRISPASR_OMNIVOICE_POS_TEMP")) {
            mg_cfg.position_temperature = (float) std::atof(e);
        }
        if (const char * e = crispasr_env::get("CRISPASR_OMNIVOICE_CLASS_TEMP")) {
            mg_cfg.class_temperature = (float) std::atof(e);
        }
        if (const char * e = crispasr_env::get("CRISPASR_OMNIVOICE_NUM_STEPS")) {
            const int n = std::atoi(e);
            if (n > 0) {
                mg_cfg.num_step = n;
            }
        }

        std::vector<float> ref_audio;
        std::vector<int32_t> ref_codes;
        const int32_t * ref_ptr = params->ref_audio_tokens;
        int ref_T = params->ref_T;
        if (params->ref_audio_24k && params->ref_n_samples > 0) {
            if (!ov->codec_loaded) {
                ov_set_error("ov_synthesize_codes: raw reference requires a loaded codec");
                return OV_STATUS_INVALID_PARAMS;
            }
            ref_audio.assign(params->ref_audio_24k, params->ref_audio_24k + params->ref_n_samples);
            ref_preprocess_audio(ref_audio, 24000, params->preprocess_prompt);
            ref_codes = pipeline_codec_encode(&ov->pc, ref_audio.data(), (int) ref_audio.size(), params->dump_dir);
            if (ref_codes.empty()) {
                ov_set_error("ov_synthesize_codes: reference encoding failed");
                return OV_STATUS_GENERATE_FAILED;
            }
            ref_T = (int) ref_codes.size() / ov->pt.lm.num_audio_codebook;
            ref_ptr = ref_codes.data();
        }

        if (params->ref_audio_24k && params->ref_audio_tokens) {
            ov_set_error("ov_synthesize_codes: reference audio and tokens are mutually exclusive");
            return OV_STATUS_INVALID_PARAMS;
        }

        int T = params->T_override > 0 ? params->T_override : duration_estimate_tokens(text, ref_text, ref_T);
        const float speed = (params->abi_version >= 4 && params->speed > 0.0f) ? params->speed : 1.0f;
        if (params->T_override <= 0 && speed != 1.0f) {
            T = std::max(1, (int) std::lround((double) T / speed));
        }

        std::vector<int32_t> codes = pipeline_tts_generate(&ov->pt, &ov->tok, text, lang, instruct, T,
                                                            params->denoise, mg_cfg, ref_text, ref_ptr, ref_T,
                                                            params->dump_dir, nullptr);
        if (codes.empty()) {
            ov_set_error("ov_synthesize_codes: generation produced no codes");
            return OV_STATUS_GENERATE_FAILED;
        }
        int32_t * dst = (int32_t *) std::malloc(codes.size() * sizeof(int32_t));
        if (!dst) {
            ov_set_error("ov_synthesize_codes: out of memory");
            return OV_STATUS_OOM;
        }
        std::memcpy(dst, codes.data(), codes.size() * sizeof(int32_t));
        *out_codes = dst;
        *out_n_codes = (int) codes.size();
        return OV_STATUS_OK;
    } catch (const std::exception & e) {
        ov_set_error("ov_synthesize_codes: %s", e.what());
        ov_log(OV_LOG_ERROR, "[OmniVoice] %s", e.what());
        return OV_STATUS_GENERATE_FAILED;
    }
}

enum ov_status ov_decode_codes(struct ov_context * ov,
                               const int32_t * codes,
                               int n_codes,
                               struct ov_audio * out) {
    if (out) {
        ov_audio_free(out);
    }
    if (!ov || !codes || n_codes <= 0 || !out) {
        ov_set_error("ov_decode_codes: invalid parameters");
        return OV_STATUS_INVALID_PARAMS;
    }
    if (!ov->codec_loaded) {
        ov_set_error("ov_decode_codes: codec not loaded");
        return OV_STATUS_INVALID_PARAMS;
    }
    const int K = ov->pt.lm.num_audio_codebook;
    if (K <= 0 || n_codes % K != 0) {
        ov_set_error("ov_decode_codes: code count %d is not divisible by K=%d", n_codes, K);
        return OV_STATUS_INVALID_PARAMS;
    }
    try {
        const int T = n_codes / K;
        std::vector<float> audio = pipeline_codec_decode(&ov->pc, codes, K, T);
        if (audio.empty()) {
            ov_set_error("ov_decode_codes: codec decode failed");
            return OV_STATUS_GENERATE_FAILED;
        }
        float * samples = (float *) std::malloc(audio.size() * sizeof(float));
        if (!samples) {
            ov_set_error("ov_decode_codes: out of memory");
            return OV_STATUS_OOM;
        }
        std::memcpy(samples, audio.data(), audio.size() * sizeof(float));
        out->samples = samples;
        out->n_samples = (int) audio.size();
        out->sample_rate = ov->pc.sample_rate;
        out->channels = 1;
        return OV_STATUS_OK;
    } catch (const std::exception & e) {
        ov_set_error("ov_decode_codes: %s", e.what());
        ov_log(OV_LOG_ERROR, "[OmniVoice] %s", e.what());
        return OV_STATUS_GENERATE_FAILED;
    }
}

int ov_duration_sec_to_tokens(const struct ov_context * ov, float duration_sec) {
    if (!ov || !ov->codec_loaded) {
        ov_set_error("ov_duration_sec_to_tokens: codec not loaded");
        ov_log(OV_LOG_ERROR, "[OmniVoice] ov_duration_sec_to_tokens requires a codec-loaded handle");
        return 1;
    }
    return pipeline_tts_duration_sec_to_tokens(&ov->pc, duration_sec);
}

int ov_num_codebooks(const struct ov_context * ov) {
    if (!ov) {
        ov_set_error("ov_num_codebooks: ov is NULL");
        return 0;
    }
    return ov->pt.lm.num_audio_codebook;
}

void ov_voice_ref_free(struct ov_voice_ref * ref) {
    if (!ref) {
        return;
    }
    if (ref->ref_codes) {
        std::free(ref->ref_codes);
    }
    ref->ref_codes     = nullptr;
    ref->ref_T         = 0;
    ref->num_codebooks = 0;
}

enum ov_status ov_extract_voice_ref(struct ov_context *   ov,
                                    const float *         ref_audio_24k,
                                    int                   ref_n_samples,
                                    struct ov_voice_ref * out) {
    if (out) {
        ov_voice_ref_free(out);
    }
    if (!ov || !ref_audio_24k || !out) {
        ov_set_error("ov_extract_voice_ref: ov, ref_audio_24k or out is NULL");
        return OV_STATUS_INVALID_PARAMS;
    }
    if (!ov->codec_loaded) {
        ov_set_error("ov_extract_voice_ref: codec not loaded");
        return OV_STATUS_GENERATE_FAILED;
    }
    if (ref_n_samples < ov->pc.hop_length) {
        ov_set_error("ov_extract_voice_ref: ref_audio_24k too short for RVQ encode (%d samples, need at least %d)",
                     ref_n_samples, ov->pc.hop_length);
        return OV_STATUS_INVALID_PARAMS;
    }

    try {
        // Match the omnivoice-codec CLI and the --ref-wav synth path:
        // RMS auto-gain, silence trim, then truncation to the hop boundary.
        std::vector<float> buf(ref_audio_24k, ref_audio_24k + ref_n_samples);
        ref_preprocess_audio(buf, 24000, true);

        const int n_aligned = ((int) buf.size() / ov->pc.hop_length) * ov->pc.hop_length;
        if (n_aligned <= 0) {
            ov_set_error("ov_extract_voice_ref: input too short after preprocessing (%zu samples, hop %d)", buf.size(),
                         ov->pc.hop_length);
            return OV_STATUS_INVALID_PARAMS;
        }

        std::vector<int32_t> codes = pipeline_codec_encode(&ov->pc, buf.data(), n_aligned);
        if (codes.empty()) {
            ov_set_error("ov_extract_voice_ref: pipeline_codec_encode returned empty codes");
            return OV_STATUS_GENERATE_FAILED;
        }

        const int K = ov->pt.lm.num_audio_codebook;
        if (K <= 0) {
            ov_set_error("ov_extract_voice_ref: invalid codebook count %d", K);
            return OV_STATUS_GENERATE_FAILED;
        }
        if ((codes.size() % (size_t) K) != 0) {
            ov_set_error("ov_extract_voice_ref: encoded code count %zu is not divisible by %d", codes.size(), K);
            return OV_STATUS_GENERATE_FAILED;
        }

        const size_t codes_bytes = codes.size() * sizeof(int32_t);
        int32_t *    codes_copy  = (int32_t *) std::malloc(codes_bytes);
        if (!codes_copy) {
            ov_set_error("ov_extract_voice_ref: malloc failed for %zu code bytes", codes_bytes);
            return OV_STATUS_OOM;
        }
        std::memcpy(codes_copy, codes.data(), codes_bytes);

        out->ref_codes     = codes_copy;
        out->ref_T         = (int) (codes.size() / (size_t) K);
        out->num_codebooks = K;

        ov_log(OV_LOG_INFO, "[OmniVoice] Extracted voice ref: K=%d T=%d (%d/%d samples)", out->num_codebooks,
               out->ref_T, n_aligned, ref_n_samples);
        return OV_STATUS_OK;
    } catch (const std::bad_alloc &) {
        ov_set_error("ov_extract_voice_ref: out of memory");
        ov_voice_ref_free(out);
        return OV_STATUS_OOM;
    } catch (const std::exception & e) {
        ov_set_error("%s", e.what());
        ov_log(OV_LOG_ERROR, "[OmniVoice] %s", e.what());
        ov_voice_ref_free(out);
        return OV_STATUS_GENERATE_FAILED;
    }
}

}  // extern "C"

// ---------------------------------------------------------------------------
// Legacy omnivoice_* facade
// ---------------------------------------------------------------------------
// Keep the old entry points source-compatible without bringing back a second
// model implementation. Every operation below delegates to the ov_* runtime
// above; the wrapper only stores the old setter-style session state.
struct omnivoice_context_params {
    int n_threads;
    int verbosity;
    bool use_gpu;
    int num_steps;
    float guidance_scale;
    float class_temperature;
    float position_temperature;
    float layer_penalty_factor;
    float t_shift;
    uint64_t seed;
    bool flash_attn;
};

struct omnivoice_context {
    ov_context * ov = nullptr;
    std::string model_path;
    std::string language;
    std::string instruct;
    std::string ref_text;
    std::vector<float> ref_audio_24k;
    float speed = 1.0f;
    int num_steps = 32;
    float guidance_scale = 2.0f;
    float class_temperature = 0.0f;
    float position_temperature = 5.0f;
    float layer_penalty_factor = 5.0f;
    float t_shift = 0.1f;
    uint64_t seed = 42;
};

extern "C" {

struct omnivoice_context_params omnivoice_context_default_params(void) {
    struct omnivoice_context_params p = {};
    p.n_threads = 4;
    p.verbosity = 1;
    p.use_gpu = true;
    p.num_steps = 32;
    p.guidance_scale = 2.0f;
    p.class_temperature = 0.0f;
    p.position_temperature = 5.0f;
    p.layer_penalty_factor = 5.0f;
    p.t_shift = 0.1f;
    p.seed = 42;
    p.flash_attn = true;
    return p;
}

struct omnivoice_context * omnivoice_init_from_file(const char * path_model,
                                                     struct omnivoice_context_params params) {
    if (!path_model || !*path_model) {
        ov_set_error("omnivoice_init_from_file: model path is empty");
        return nullptr;
    }
    ov_init_params ip;
    ov_init_default_params(&ip);
    ip.model_path = path_model;
    ip.use_fa = params.flash_attn && params.use_gpu;
    omnivoice_context * ctx = new (std::nothrow) omnivoice_context();
    if (!ctx) {
        ov_set_error("omnivoice_init_from_file: out of memory");
        return nullptr;
    }
    ctx->model_path = path_model;
    ctx->num_steps = params.num_steps > 0 ? params.num_steps : 32;
    ctx->guidance_scale = params.guidance_scale > 0.0f ? params.guidance_scale : 2.0f;
    ctx->class_temperature = params.class_temperature;
    ctx->position_temperature = params.position_temperature > 0.0f ? params.position_temperature : 5.0f;
    ctx->layer_penalty_factor = params.layer_penalty_factor > 0.0f ? params.layer_penalty_factor : 5.0f;
    ctx->t_shift = params.t_shift > 0.0f ? params.t_shift : 0.1f;
    ctx->seed = params.seed ? params.seed : 42;
    ctx->ov = ov_init(&ip);
    if (!ctx->ov) {
        delete ctx;
        return nullptr;
    }
    ov_set_n_threads(ctx->ov, params.n_threads);
    return ctx;
}

int omnivoice_set_tokenizer_path(struct omnivoice_context * ctx, const char * path) {
    if (!ctx || !ctx->ov || !path) {
        return -1;
    }
    return ov_set_codec_path(ctx->ov, path) == OV_STATUS_OK ? 0 : -1;
}

int omnivoice_set_voice_prompt(struct omnivoice_context * ctx, const char * wav_path, const char * ref_text) {
    if (!ctx || !ctx->ov) {
        return -1;
    }
    ctx->ref_audio_24k.clear();
    ctx->ref_text = ref_text ? ref_text : "";
    if (!wav_path || !*wav_path) {
        return 0;
    }
    std::vector<float> wav;
    int sr = 0;
    if (!crispasr::core::read_wav_mono_pcm16(wav_path, wav, sr) || wav.empty()) {
        ov_set_error("omnivoice_set_voice_prompt: failed to read '%s'", wav_path);
        return -1;
    }
    if (sr != 24000 && sr > 0) {
        wav = core_audio::resample_polyphase(wav.data(), (int) wav.size(), sr, 24000);
    }
    ctx->ref_audio_24k = std::move(wav);
    return 0;
}

int omnivoice_set_language(struct omnivoice_context * ctx, const char * lang) {
    if (!ctx) {
        return -1;
    }

    const std::string requested = lang ? lang : "";
    const auto resolved = core_omnivoice_lang::resolve(requested);
    ctx->language = resolved.id;
    if (resolved.status == core_omnivoice_lang::Status::unrecognized) {
        const std::string hint = core_omnivoice_lang::suggest(requested);
        std::fprintf(stderr, "crispasr[omnivoice]: language '%s' is unsupported%s%s; using language-agnostic synthesis\n",
                     requested.c_str(), hint.empty() ? "" : " (did you mean '",
                     hint.empty() ? "" : (hint + "')").c_str());
        return -2;
    }
    return 0;
}

int omnivoice_set_instruct(struct omnivoice_context * ctx, const char * instruct) {
    if (!ctx) {
        return -1;
    }

    const std::string requested = instruct ? instruct : "";
    const core_omnivoice_instruct::Parsed parsed = core_omnivoice_instruct::parse(requested);
    if (parsed.status != core_omnivoice_instruct::Status::ok &&
        parsed.status != core_omnivoice_instruct::Status::cleared) {
        std::fprintf(stderr, "crispasr[omnivoice]: %s\n", parsed.error.c_str());
        ctx->instruct.clear();
        return -2;
    }
    ctx->instruct = requested;
    return 0;
}

int omnivoice_set_speed(struct omnivoice_context * ctx, float speed) {
    if (!ctx) {
        return -1;
    }
    ctx->speed = speed > 0.0f ? speed : 1.0f;
    return 0;
}

int omnivoice_set_num_steps(struct omnivoice_context * ctx, int num_steps) {
    if (!ctx) {
        return -1;
    }
    if (num_steps > 0) {
        ctx->num_steps = num_steps;
    }
    return 0;
}

int omnivoice_set_seed(struct omnivoice_context * ctx, uint64_t seed) {
    if (!ctx) {
        return -1;
    }
    ctx->seed = seed;
    return 0;
}

int32_t * omnivoice_synthesize_codes(struct omnivoice_context * ctx, const char * text, int * out_n_codes) {
    if (!ctx || !ctx->ov || !text || !out_n_codes) {
        return nullptr;
    }
    ov_tts_params tp;
    ov_tts_default_params(&tp);
    tp.text = text;
    tp.lang = ctx->language.c_str();
    tp.instruct = ctx->instruct.c_str();
    tp.mg_num_step = ctx->num_steps;
    tp.mg_guidance_scale = ctx->guidance_scale;
    tp.mg_class_temperature = ctx->class_temperature;
    tp.mg_position_temperature = ctx->position_temperature;
    tp.mg_layer_penalty_factor = ctx->layer_penalty_factor;
    tp.mg_t_shift = ctx->t_shift;
    tp.mg_seed = ctx->seed;
    tp.speed = ctx->speed;
    tp.ref_text = ctx->ref_text.c_str();
    if (!ctx->ref_audio_24k.empty()) {
        tp.ref_audio_24k = ctx->ref_audio_24k.data();
        tp.ref_n_samples = (int) ctx->ref_audio_24k.size();
    }
    int32_t * codes = nullptr;
    int n_codes = 0;
    if (ov_synthesize_codes(ctx->ov, &tp, &codes, &n_codes) != OV_STATUS_OK) {
        *out_n_codes = 0;
        return nullptr;
    }
    *out_n_codes = n_codes;
    return codes;
}

void omnivoice_codes_free(int32_t * codes) {
    ov_codes_free(codes);
}

float * omnivoice_decode_codes(struct omnivoice_context * ctx,
                               const int32_t * codes,
                               int n_codes,
                               int * out_n_samples) {
    if (!ctx || !ctx->ov || !out_n_samples) {
        return nullptr;
    }
    ov_audio audio = {};
    if (ov_decode_codes(ctx->ov, codes, n_codes, &audio) != OV_STATUS_OK) {
        *out_n_samples = 0;
        return nullptr;
    }
    *out_n_samples = audio.n_samples;
    return audio.samples;
}

float * omnivoice_synthesize(struct omnivoice_context * ctx, const char * text, int * out_n_samples) {
    if (!ctx || !ctx->ov || !text || !out_n_samples) {
        return nullptr;
    }
    int n_codes = 0;
    int32_t * codes = omnivoice_synthesize_codes(ctx, text, &n_codes);
    if (!codes) {
        *out_n_samples = 0;
        return nullptr;
    }
    float * pcm = omnivoice_decode_codes(ctx, codes, n_codes, out_n_samples);
    omnivoice_codes_free(codes);
    return pcm;
}

void omnivoice_pcm_free(float * pcm) {
    std::free(pcm);
}

void omnivoice_free(struct omnivoice_context * ctx) {
    if (!ctx) {
        return;
    }
    ov_free(ctx->ov);
    delete ctx;
}

void omnivoice_sync(struct omnivoice_context * /*ctx*/) {
    // GGML scheduler calls are synchronous; retained as a no-op compatibility
    // barrier for callers that used the previous backend directly.
}

void omnivoice_set_n_threads(struct omnivoice_context * ctx, int n_threads) {
    if (ctx && ctx->ov) {
        ov_set_n_threads(ctx->ov, n_threads);
    }
}

int ov_encode_diff(struct ov_context * ov, const char * ref_gguf_path) {
    if (!ov || !ref_gguf_path || !*ref_gguf_path) {
        return -1;
    }
    if (!ov->codec_loaded) {
        ov_set_error("ov_encode_diff: codec not loaded");
        return -1;
    }

    ggml_context * ref_ctx = nullptr;
    gguf_init_params gp = {};
    gp.no_alloc = false;
    gp.ctx = &ref_ctx;
    gguf_context * ref = gguf_init_from_file(ref_gguf_path, gp);
    if (!ref || !ref_ctx) {
        if (ref) {
            gguf_free(ref);
        }
        ov_set_error("ov_encode_diff: cannot open '%s'", ref_gguf_path);
        return -1;
    }

    ggml_tensor * wav_t = ggml_get_tensor(ref_ctx, "input_wav24k");
    ggml_tensor * code_t = ggml_get_tensor(ref_ctx, "codes");
    if (!wav_t || !code_t || wav_t->type != GGML_TYPE_F32) {
        gguf_free(ref);
        ggml_free(ref_ctx);
        ov_set_error("ov_encode_diff: archive needs F32 input_wav24k and codes tensors");
        return -1;
    }

    const int n_samples = (int) ggml_nelements(wav_t);
    std::vector<int32_t> mine = pipeline_codec_encode(&ov->pc, (const float *) wav_t->data, n_samples);
    const size_t n_ref = ggml_nelements(code_t);
    std::vector<int32_t> expected(n_ref);
    if (code_t->type == GGML_TYPE_I32) {
        std::memcpy(expected.data(), code_t->data, n_ref * sizeof(int32_t));
    } else if (code_t->type == GGML_TYPE_F32) {
        const float * values = (const float *) code_t->data;
        for (size_t i = 0; i < n_ref; ++i) {
            expected[i] = (int32_t) std::lround(values[i]);
        }
    } else {
        gguf_free(ref);
        ggml_free(ref_ctx);
        ov_set_error("ov_encode_diff: unsupported codes tensor type");
        return -1;
    }

    size_t matches = 0;
    const size_t n = std::min(mine.size(), expected.size());
    for (size_t i = 0; i < n; ++i) {
        matches += mine[i] == expected[i] ? 1u : 0u;
    }
    const bool pass = mine.size() == expected.size() && matches == expected.size();
    std::fprintf(stderr, "omnivoice encode-diff: %zu/%zu exact (%.1f%%) %s\n", matches, expected.size(),
                 expected.empty() ? 0.0 : 100.0 * (double) matches / (double) expected.size(), pass ? "PASS" : "FAIL");
    gguf_free(ref);
    ggml_free(ref_ctx);
    if (!pass) {
        ov_set_error("ov_encode_diff: generated codes differ from '%s'", ref_gguf_path);
        return -1;
    }
    return 0;
}

int omnivoice_encode_diff(struct omnivoice_context * ctx, const char * ref_gguf_path) {
    return ctx && ctx->ov ? ov_encode_diff(ctx->ov, ref_gguf_path) : -1;
}

}  // extern "C"
