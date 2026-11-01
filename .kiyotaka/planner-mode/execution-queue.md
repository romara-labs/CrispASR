# Execution Queue: Kokoro Full GGUF

1. TK-0001: Validar contrato dos GGUFs full reais - completed
2. TK-0002: Adicionar loader de vozes embutidas no contexto Kokoro - completed
3. TK-0003: Selecionar voz embutida por ABI e fallback - completed
4. TK-0004: Integrar CLI com `--voice` path ou id - completed
5. TK-0005: Atualizar conversor/documentacao do formato full - pending parcial

## Status

TK-0001 a TK-0004 concluidos. TK-0005 permanece parcial: a documentacao Kiyotaka existe, mas docs publicas/conversor ainda nao foram fechados nesta passagem.

## Evidencia

- `cmake --build build --target crispasr-cli` passou.
- GGUF full F32 sem `--voice` gerou `/tmp/crispasr-kokoro-full-default.wav`.
- GGUF full F32 com `--voice af_heart` gerou `/tmp/crispasr-kokoro-full-af_heart.wav`.
- GGUF full F16 com `--voice af_heart` gerou `/tmp/crispasr-kokoro-full-f16.wav`.
- `--voice no_such_voice` falhou com mensagem especifica de id embutido inexistente.
- Regressao de audio 2026-06-29: `/tmp/crispasr-kokoro-pf-dora-curto-fixed.wav` gerado com `pf_dora`/`pt-br` apos corrigir leitura F32 de `dec.gen.m_source.weight`; metricas `peak=15218 rms=1658.5 zcr=0.131`, contra WAV ruidoso anterior `peak=1196 rms=300.4 zcr=0.4002`.
- `/tmp/kokoro-pf-dora-pt-br.wav` regenerado e tocado com `aplay`; metricas `duration=4.62s peak=12581 rms=1449.0 zcr=0.1048`.
- Performance CPU realtime: baseline ~10.44s/4.62s (RTF ~2.26); apos reuso de `mag`/`phase`, predictor/F0N e F0 do decoder, `/tmp/kokoro-pf-dora-pt-br.wav` gerou em ~4.26s/4.62s (RTF ~0.92) com `--gpu-backend cpu`.
