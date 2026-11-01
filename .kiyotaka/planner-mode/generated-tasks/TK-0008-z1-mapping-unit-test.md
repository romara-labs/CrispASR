# TK-0008 — Z1-unit-test: Teste sintético do mapeamento Full-GGUF

**Classification:** PATCH_PLAN
**Status:** Pending (blocked by TK-0007; pode ser co-desenvolvido)
**Priority:** Alta
**Blast Radius:** Small (tests/)

## Descrição
Criar teste de unidade que exercita `map_full_gguf_tensor_name` (e `is_full_gguf_conv_alias`) com nomes externos representativos, sem depender de GGUF grande — seguindo a convenção do repo (cf. `tests/test-kokoro-params.cpp`, "ZERO TODOs nos testes", null-guard style).

Cobrir pelo menos um caso por ramo de mapeamento: bert.embeddings, bert.enc, bert.pooler, bert_encoder, text_encoder (cnn/ln/lstm), predictor (dur_proj/lstm/shared/F0/N/adain), decoder (gen/encode/decode/asr_res/F0_conv/N_conv). Mais: embeddings NÃO transpostos, 2D lineares transpostos, conv-aliases NÃO transpostos.

Se `map_full_gguf_tensor_name` for `static` (não linkável do teste), expor via header de teste interno ou tornar a função testável (ex.: mover para um pequeno header `kokoro_full_gguf.h` incluso por ambos) — decisão de implementação, manter blast radius small.

## Dependencies
- TK-0007 (teste cobre o estado corrigido).

## Blockers
- TK-0007.

## Acceptance Criteria
- [ ] `tests/test-kokoro-full-gguf-mapping.cpp` criado e wired no CMake de testes.
- [ ] ≥1 caso por ramo de mapeamento; casos de transpose (embedding vs linear vs conv) distintos.
- [ ] Teste compila e passa no CI (catch2).
- [ ] Zero dependência de GGUF grande (fixture sintético).

## Required Tests
- Unit (este TK é o próprio teste).

## Affected Files
- `tests/test-kokoro-full-gguf-mapping.cpp` (novo)
- `tests/CMakeLists.txt` (wiring)
- possivelmente `src/kokoro.cpp`/novo header para expor a função ao teste (minimal)
