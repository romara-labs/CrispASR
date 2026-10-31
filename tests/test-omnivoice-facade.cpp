// Exercise the active Romara compatibility facade, without model downloads.
#include "../src/omnivoice/omnivoice.cpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <limits>

int main() {
    omnivoice_context ctx;
    assert(omnivoice_set_audio_processing(nullptr, true, true, 0.1f, 0.1f) == -1);
    for (float invalid : {-1.0f, 11.0f, std::numeric_limits<float>::infinity(),
                          std::numeric_limits<float>::quiet_NaN()}) {
        assert(omnivoice_set_audio_processing(&ctx, false, false, invalid, 0.0f) == -1);
        assert(omnivoice_set_audio_processing(&ctx, false, false, 0.0f, invalid) == -1);
        assert(ctx.preprocess_prompt && ctx.postprocess_output);
    }
    // Disabling silence removal must still allow independent fade and padding.
    assert(omnivoice_set_audio_processing(&ctx, false, false, 0.01f, 0.01f) == 0);
    assert(!ctx.preprocess_prompt && !ctx.postprocess_output);
    std::vector<float> pcm(1000, 0.2f);
    omnivoice_process_output(ctx, pcm);
    const size_t pad = (size_t)(ctx.pad_duration * 24000);
    assert(pcm.size() == 1000 + 2 * pad);
    assert(pcm.front() == 0.0f && pcm.back() == 0.0f);
    assert(pcm[pad] == 0.0f && pcm[pad + 999] == 0.0f);
    assert(std::fabs(pcm[pad + 500] - 0.5f) < 1e-6f);
    // Quiet voice references restore the original RMS instead of peak scaling.
    assert(omnivoice_set_audio_processing(&ctx, true, false, 0.0f, 0.0f) == 0);
    ctx.ref_audio_24k.assign(1, 0.05f);
    ctx.ref_rms = 0.05f;
    pcm.assign(1000, 0.2f);
    omnivoice_process_output(ctx, pcm);
    assert(pcm.size() == 1000 && std::fabs(pcm[500] - 0.1f) < 1e-6f);
    // Fully silent output is rejected by the decoder after cleanup.
    assert(omnivoice_set_audio_processing(&ctx, true, true, 0.1f, 0.1f) == 0);
    pcm.assign(24000, 0.0f);
    omnivoice_process_output(ctx, pcm);
    assert(pcm.empty());
    // The fork's explicit-duration reset continues to restore its environment value.
    ctx.env_target_duration_s = 2.0f;
    assert(omnivoice_set_target_duration(&ctx, 4.0f) == 0 && ctx.target_duration_s == 4.0f);
    assert(omnivoice_set_target_duration(&ctx, 0.0f) == 0 && ctx.target_duration_s == 2.0f);
    return 0;
}
