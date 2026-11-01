# TK-0006 — Z1-test: Auditar map_full_gguf_tensor_name contra tensores reais

**Classification:** PATCH_PLAN
**Status:** COMPLETED (2026-06-28) — root cause pinned + fix verified; ver _kiyotaka_project/z1-mapping-audit.md §RESOLUÇÃO
**Priority:** Alta
**Blast Radius:** Small (read-only; produz relatório)

## Descrição
Enumerar os nomes de tensores REAIS do GGUF full (`voicer.cpp/convert-gguf/Kokoro-82M-v1.0-F32.full.gguf`, 513 tensores) e compará-los contra:
1. A saída de `map_full_gguf_tensor_name` (kokoro.cpp:312-407) para cada nome `kokoro.*`;
2. A lista `must_have` de `sanity_check_weights` (kokoro.cpp:581+);
3. O que o grafo de síntese efetivamente consome.

Identificar gaps: regras ausentes, regras erradas, hazards de ordenação entre `replace_all` encadeados (kokoro.cpp:373-402), e casos onde a heurística `is_full_gguf_conv_alias` (kokoro.cpp:411) discorda do conjunto de conv do loader fonte (`kokoro-gguf.cpp`).

## Dependencies
- Nenhuma (TK-0001 forneceu o contrato base do GGUF full).

## Blockers
- Nenhum.

## Acceptance Criteria
- [ ] Lista exaustiva dos 513 nomes de tensores do GGUF F32 full dumpada.
- [ ] Para cada nome `kokoro.*`, registrado: mapeia para qual nome interno, ou "SEM MAPEAMENTO".
- [ ] Cada `must_have` do sanity_check tem pelo menos um nome externo que o produz (ou marcado como gap).
- [ ] Relatório classifica cada gap: (missing rule / wrong rule / ordering hazard / conv-alias heuristic miss / embedding-vs-linear transpose ambiguity).
- [ ] Output: `_kiyotaka_project/z1-mapping-audit.md`.

## Required Tests
- Comando de inspeção GGUF executado sobre os 4 arquivos em `convert-gguf/` (full BF16/F16/F32 + model-only).
- (Sem teste de unidade neste TK — o teste entra no TK-0008.)

## Affected Files
- Nenhum (read-only). Output em `_kiyotaka_project/z1-mapping-audit.md` (novo).

## Ground truth
- Loader fonte: `/home/rodrigo/Documentos/gguf-onnx-migrations/voicer.cpp/src/kokoro/kokoro-gguf.cpp`
- GGUFs: `/home/rodrigo/Documentos/gguf-onnx-migrations/voicer.cpp/convert-gguf/Kokoro-82M-v1.0-*.gguf`
