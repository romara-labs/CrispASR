# TKs Gerados: Delta Stabilization (PATCH_PLAN) — 2026-06-28

Sequência a partir de TK-0006 (predecessor MIGRATION_PLAN usou TK-0001..TK-0005).

| ID | Título | Dep | Blast | Status |
|----|--------|-----|-------|--------|
| TK-0006 | Z1-test: auditar map_full_gguf_tensor_name vs tensores reais | — | Small | Pending (UNBLOCKED) |
| TK-0007 | Z1-fix: corrigir regras de mapeamento | TK-0006 | Medium | Pending (blocked) |
| TK-0008 | Z1-unit-test: teste sintético do mapeamento | TK-0007 | Small | Pending (blocked) |
| TK-0009 | R1-doc: ADR exceções-no-boundary | — | Small | Pending (UNBLOCKED) |
| TK-0010 | R1-adjust: alinhar divergência (mínimo) | TK-0009 | Small | Pending (blocked) |
| TK-0011 | D2/D3-stabilize: remover/gate KOKORO_DEBUG_SHAPES | externo | Medium | Pending (BLOCKED externo) |

Detalhes em `generated-tasks/TK-000{6..9}-*.md`, `TK-001{0,1}-*.md`.

## Resumo de aceitação por TK
- **TK-0006:** relatório `_kiyotaka_project/z1-mapping-audit.md` com gaps classificados.
- **TK-0007:** todos `must_have` resolvem; heurística conv = loader fonte; sem regressão.
- **TK-0008:** `tests/test-kokoro-full-gguf-mapping.cpp` passa no CI sem GGUF grande.
- **TK-0009:** ADR existe e referencia ov-error.h:14-20 + omnivoice.cpp:240-269,323-332.
- **TK-0010:** cross-ref ADR no código; toda entrada `extern "C"` envolta em try/catch; sem mudança comportamental.
- **TK-0011:** core/lstm.h + kokoro.cpp sem shims em produção; core/ volta ao estado limpo.

## Autorização
Mantenedor autorizou edição dos arquivos antes bloqueados (regra non-negotiable), escopada a TK-0006..TK-0011. D4 descartado (risco aceito).
