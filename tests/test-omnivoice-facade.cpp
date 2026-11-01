// Exercise the active Romara compatibility facade, without model downloads.
#include "../src/omnivoice/omnivoice.cpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <limits>

// Exercise the production DAC/RVQ readers with actual F16 GGUF payloads.
// A successful model load alone cannot detect interpreting F16 bits as BF16.
static void test_f16_codec_readers() {
    ggml_backend_load_all();
    ggml_backend_t cpu = cpu_backend_new(1);
    assert(cpu);
    ggml_context * meta = ggml_init({1024 * 1024, nullptr, false});
    gguf_context * header = gguf_init_empty();
    assert(meta && header);
    const float values[] = {0.5f, 1.0f, -0.25f, 2.0f};
    auto add = [&](const std::string & name, bool vector) {
        ggml_tensor * t = vector ? ggml_new_tensor_1d(meta, GGML_TYPE_F16, 2)
                                : ggml_new_tensor_2d(meta, GGML_TYPE_F16, 2, 2);
        ggml_set_name(t, name.c_str());
        ggml_fp32_to_fp16_row(values, (ggml_fp16_t *) t->data, ggml_nelements(t));
        gguf_add_tensor(header, t);
    };
    for (int k = 0; k < RVQ_NUM_CODEBOOKS; ++k) {
        const std::string prefix = "quantizer.quantizers." + std::to_string(k);
        add(prefix + ".codebook.embed", false);
        add(prefix + ".project_in.weight", false);
        add(prefix + ".project_in.bias", true);
        add(prefix + ".project_out.weight", false);
        add(prefix + ".project_out.bias", true);
    }
    gguf_set_val_u32(header, "omnivoice.codebook_dim", 2);
    gguf_set_val_u32(header, "omnivoice.codebook_size", 2);
    ggml_tensor * alpha = ggml_new_tensor_3d(meta, GGML_TYPE_F16, 1, 2, 1);
    ggml_set_name(alpha, "alpha");
    ggml_fp32_to_fp16_row(values, (ggml_fp16_t *) alpha->data, 2);
    gguf_add_tensor(header, alpha);
    size_t data_size = 0;
    for (int i = 0; i < gguf_get_n_tensors(header); ++i) {
        ggml_tensor * t = ggml_get_tensor(meta, gguf_get_tensor_name(header, i));
        data_size = std::max(data_size, gguf_get_tensor_offset(header, i) + ggml_nbytes(t));
    }
    std::vector<uint8_t> bytes(data_size);
    for (int i = 0; i < gguf_get_n_tensors(header); ++i) {
        ggml_tensor * t = ggml_get_tensor(meta, gguf_get_tensor_name(header, i));
        std::memcpy(bytes.data() + gguf_get_tensor_offset(header, i), t->data, ggml_nbytes(t));
    }
    GGUFModel gf = {};
    gf.gguf = header;
    gf.meta = meta;
    gf.mapping = bytes.data();
    WeightCtx weights = {};
    wctx_init(&weights, 64);
    RVQCodec rvq = {};
    assert(rvq_load(&rvq, &weights, gf));
    assert(wctx_alloc(&weights, cpu));
    float norms[2];
    ggml_backend_tensor_get(rvq.cb[0].embed_sq, norms, 0, sizeof(norms));
    assert(norms[0] == 1.25f && norms[1] == 4.0625f);
    ggml_context * output = ggml_init({4096, nullptr, true});
    ggml_tensor * dst = ggml_new_tensor_1d(output, GGML_TYPE_F32, 2);
    ggml_backend_buffer_t buffer = ggml_backend_alloc_ctx_tensors(output, cpu);
    assert(buffer);
    float actual[2];
    dac_load_bias_f32(dst, gf, "quantizer.quantizers.0.project_in.bias");
    ggml_backend_tensor_get(dst, actual, 0, sizeof(actual));
    assert(actual[0] == 0.5f && actual[1] == 1.0f);
    dac_load_alpha(dst, gf, "alpha", false);
    ggml_backend_tensor_get(dst, actual, 0, sizeof(actual));
    assert(actual[0] == 0.5f && actual[1] == 1.0f);
    dac_load_alpha(dst, gf, "alpha", true);
    ggml_backend_tensor_get(dst, actual, 0, sizeof(actual));
    assert(actual[0] == 2.0f && actual[1] == 1.0f);
    ggml_backend_buffer_free(buffer);
    ggml_free(output);
    wctx_free(&weights);
    gguf_free(header);
    ggml_free(meta);
    ggml_backend_free(cpu);
}

int main() {
    test_f16_codec_readers();
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
