# TK-0011 — D2/D3-stabilize: Remover/gate scaffolding KOKORO_DEBUG_SHAPES

**Classification:** PATCH_PLAN
**Status:** Pending (BLOCKED por condição externa)
**Priority:** Baixa (quando o vocoder estabilizar)
**Blast Radius:** Medium (src/core/lstm.h — módulo mais limpo — + src/kokoro.cpp)

## Descrição
Estabilizar o scaffolding de debug introduzido para o vocoder MI-LSTM: `lstm_debug_mul_mat` (core/lstm.h:45-67), `kokoro_debug_mul_mat` e `kokoro_debug_conv_1d` (kokoro.cpp), todos gateados por env var `KOKORO_DEBUG_SHAPES`. Quando o grafo do vocoder MI-LSTM estiver estável (trilha de commits 32K→65K→131K nós convergiu), remover os shims OU movê-los para trás de uma flag de compilação (e.g. `#ifdef CRISPASR_KOKORO_DEBUG`) para que `core/` volte ao estado limpo (zero TODOs, sem paths de debug em produção).

## Dependencies
- Condição externa: estabilização do vocoder MI-LSTM (fora do escopo destes TKs).

## Blockers
- EXTERNO — vocoder MI-LSTM ainda em iteração. Não é bloqueador dos outros TKs.

## Acceptance Criteria
- [ ] `core/lstm.h` não contém mais shims `lstm_debug_*` em produção (removidos OU sob `#ifdef`).
- [ ] `kokoro.cpp` não referencia mais `kokoro_debug_*` em produção (ou estão sob flag).
- [ ] `core/` volta ao estado "zero TODOs / zero paths de debug" (claim #24 da verificação 25/06).
- [ ] Build + teste de regressão do Kokoro passam.

## Required Tests
- Build (default = sem shims ativos).
- Regressão do vocoder Kokoro (síntese de referência inalterada).

## Affected Files
- `src/core/lstm.h` (45-67)
- `src/kokoro.cpp` (`kokoro_debug_mul_mat`, `kokoro_debug_conv_1d` e call sites)
