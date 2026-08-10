#pragma once

// Compatibility facade for the original CrispASR OmniVoice C ABI.
//
// The implementation now lives exclusively in src/omnivoice/ (the complete
// realtime pipeline).  This header keeps the older `omnivoice_*` entry points
// available to the C API, diff harnesses and downstream users while routing
// them through that single implementation.  New integrations should prefer
// the richer ov_* API declared by omnivoice/omnivoice.h.

#include <stdbool.h>
#include <stdint.h>

#include "omnivoice/omnivoice.h"

#ifdef __cplusplus
extern "C" {
#endif

struct omnivoice_context;

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

struct omnivoice_context_params omnivoice_context_default_params(void);

struct omnivoice_context * omnivoice_init_from_file(const char * path_model,
                                                     struct omnivoice_context_params params);

// The legacy tokenizer setter is backed by ov_set_codec_path and may be used
// after init.  Loading the companion codec is required before audio decode.
int omnivoice_set_tokenizer_path(struct omnivoice_context * ctx, const char * path);
int omnivoice_set_voice_prompt(struct omnivoice_context * ctx, const char * wav_path, const char * ref_text);
int omnivoice_set_language(struct omnivoice_context * ctx, const char * lang);
int omnivoice_set_instruct(struct omnivoice_context * ctx, const char * instruct);
int omnivoice_set_speed(struct omnivoice_context * ctx, float speed);
int omnivoice_set_num_steps(struct omnivoice_context * ctx, int num_steps);
int omnivoice_set_seed(struct omnivoice_context * ctx, uint64_t seed);

int32_t * omnivoice_synthesize_codes(struct omnivoice_context * ctx, const char * text, int * out_n_codes);
void      omnivoice_codes_free(int32_t * codes);
float *   omnivoice_decode_codes(struct omnivoice_context * ctx,
                                 const int32_t * codes,
                                 int n_codes,
                                 int * out_n_samples);
float * omnivoice_synthesize(struct omnivoice_context * ctx, const char * text, int * out_n_samples);
void    omnivoice_pcm_free(float * pcm);
void    omnivoice_free(struct omnivoice_context * ctx);
void    omnivoice_sync(struct omnivoice_context * ctx);
void    omnivoice_set_n_threads(struct omnivoice_context * ctx, int n_threads);

// Compare the unified codec encoder against a reference GGUF containing
// `input_wav24k` and `codes` tensors. Returns zero when every code matches.
int omnivoice_encode_diff(struct omnivoice_context * ctx, const char * ref_gguf_path);

#ifdef __cplusplus
}
#endif
