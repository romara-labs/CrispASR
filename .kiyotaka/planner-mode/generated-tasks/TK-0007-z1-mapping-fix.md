# TK-0007 — Z1-fix: Corrigir regras de map_full_gguf_tensor_name + is_full_gguf_conv_alias

**Classification:** PATCH_PLAN
**Status:** COMPLETED (2026-06-29) — fix em is_full_gguf_conv_alias (kokoro.cpp): dec.gen adain linears (`.adain1.`/`.adain2.`+`.weight`) agora transpostos. Correção complementar em `kokoro_make_har`: `dec.gen.m_source.weight` usa `tensor_to_f32` em vez de leitura fixa F16, pois o GGUF full armazena esse peso como F32. Verificado contra ruído perceptual: WAV antigo CrispASR `peak=1196 rms=300.4 zcr=0.4002`; WAV corrigido `peak=15218 rms=1658.5 zcr=0.131`, próximo da referência `voicer-cli` `peak=13749 rms=1833.9 zcr=0.0866`.
**Priority:** Alta
**Blast Radius:** Medium (src/kokoro.cpp — núcleo do Kokoro)

## Descrição
Aplicar as correções identificadas no audit TK-0006 em `map_full_gguf_tensor_name` (kokoro.cpp:312-407), `is_full_gguf_conv_alias` (kokoro.cpp:411) e, se necessário, `sanity_check_weights` (kokoro.cpp:581+). Garantir que todo tensor `must_have` resolve e que nenhum tensor externo relevante fica sem mapeamento silencioso.

Respeitar a convenção local do arquivo (snake_case, assert-abort/return-code, zero exceções em kokoro, comentários densos no estilo existente).

## Dependencies
- TK-0006 (audit identifica o quê corrigir).

## Blockers
- TK-0006.

## Acceptance Criteria
- [ ] Cada gap do audit TK-0006 corrigido (regra adicionada / corrigida / reordenada).
- [ ] Todos os tensores `must_have` do sanity_check resolvem a partir do GGUF full real.
- [ ] Heurística `is_full_gguf_conv_alias` coincide com o conjunto de conv do loader fonte.
- [ ] Hazards de ordenação entre `replace_all` eliminados (verificado com casos sobrepostos).
- [ ] Sem regressão: runtime antigo (model-only + voice pack separado) continua funcionando.

## Required Tests
- TK-0008 (unit test do mapeamento) deve passar.
- Build de regressão do Kokoro (compila + sanity_check passa com GGUF F32 full real).
- Smoke: síntese curta com voz embutida produz áudio não-silencioso no sample rate esperado (24kHz) e sem perfil de ruído de alta taxa de cruzamento por zero.

## Affected Files
- `src/kokoro.cpp` (linhas ~312-407, ~411, ~581+)
