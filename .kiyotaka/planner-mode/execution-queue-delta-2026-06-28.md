# Execution Queue — Delta Stabilization (2026-06-28)

Ordem respeitando dependências. Frentes Z1 e R1 são independentes e podem avançar em paralelo.

## Wave 1 (unblocked)
1. **TK-0006** — Z1-test: auditar mapeamento (read-only) ← **PRÓXIMA TAREFA SEGURA**
2. **TK-0009** — R1-doc: ADR exceções-no-boundary (paralelo, independente)

## Wave 2 (após Wave 1)
3. **TK-0007** — Z1-fix: corrigir regras (dep TK-0006)
4. **TK-0010** — R1-adjust: alinhar divergência (dep TK-0009)

## Wave 3 (após Wave 2)
5. **TK-0008** — Z1-unit-test (dep TK-0007)

## Bloqueado (condição externa)
6. **TK-0011** — D2/D3-stabilize: aguarda vocoder MI-LSTM estável.

## Status
**READY_TO_EXECUTE.** Próxima tarefa segura: **TK-0006** (read-only, ground truth disponível em `voicer.cpp/convert-gguf/`). TK-0009 pode rodar em paralelo.

## Blockers
- TK-0007 → TK-0006; TK-0008 → TK-0007; TK-0010 → TK-0009.
- TK-0011 → externo (vocoder). Não bloqueia o resto do plano.
