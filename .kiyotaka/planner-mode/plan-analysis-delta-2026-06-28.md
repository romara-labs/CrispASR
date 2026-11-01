# Plan Analysis — Delta Stabilization (PATCH_PLAN)

**Data:** 2026-06-28
**Predecessor:** MIGRATION_PLAN TK-0001..TK-0005 (run 21:26-21:28 de hoje)

## Phases
1. **Z1 — Validar e corrigir o mapeamento de nomes** (prioridade alta; fragilidade confirmada na escavação)
2. **R1 — Documentar e ajustar a divergência de exceções** (paralelo, baixo risco)
3. **D2/D3 — Estabilizar o scaffolding de debug** (condicional à estabilização do vocoder)

## Tasks (mapeadas para TK-0006..TK-0011)
- TK-0006 Z1-test (read-only audit)
- TK-0007 Z1-fix (corrige kokoro.cpp) — dep TK-0006
- TK-0008 Z1-unit-test (teste sintético) — dep TK-0007
- TK-0009 R1-doc (ADR/nota) — sem dep
- TK-0010 R1-adjust (comentário/cross-ref mínimo) — dep TK-0009
- TK-0011 D2/D3-stabilize (remove/gate KOKORO_DEBUG_SHAPES) — bloqueado por condição externa (vocoder MI-LSTM estável)

## Dependencies
- TK-0007 → TK-0006 (não corrige o que não mediu)
- TK-0008 → TK-0007 (teste cobre o fix)
- TK-0010 → TK-0009 (ajuste referencia o doc)
- TK-0011 ← externo (estabilização do vocoder; fora do controle destes TKs)

## Risks
- **R-Z1a:** mapeamento depende da convenção de naming do exporter externo; mudança upstream quebra silenciosamente → mitigação: TK-0008 testa a tabela; TK-0006 documenta o contrato.
- **R-Z1b:** hazard de ordenação entre replace_all encadeados (kokoro.cpp:373-402) → TK-0006 deve verificar casos que se sobrepõem.
- **R-R1:** "ajustar" a divergência de exceções pode tentar remover ov_throw do omnivoice — **não fazer**; omnivoice é runtime isolado importado, a fronteira catch é a garantia. R1-adjust = documentar/alinhar, NÃO remover exceções.
- **R-D2D3:** remover o scaffolding cedo demais atrapalha o debug do vocoder em andamento → TK-0011 fica bloqueado até o vocoder estabilizar.

## Blockers
- TK-0011 bloqueado por condição externa (vocoder MI-LSTM estável). Não é bloqueador deste plano como um todo.
- Bloqueador anterior (regra non-negotiable) **levantado pelo mantenedor** para os TKs 0006-0011.

## Affected files
- src/kokoro.cpp (312-407 map_full_gguf_tensor_name; 411 is_full_gguf_conv_alias; 581+ sanity_check_weights)
- tests/ (novo test-kokoro-full-gguf-mapping.cpp + CMake)
- docs/ (novo ADR sobre exceções-no-boundary)
- src/omnivoice/ov-error.h (comentário cross-ref, minimal)
- src/core/lstm.h (45-67), src/kokoro.cpp (kokoro_debug_*) — TK-0011

## Decisions pending
- Nenhuma bloqueante. D4 já decidido (descartado). TK-0011 aguarda condição externa.
