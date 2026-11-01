# TK-0009 — R1-doc: ADR sobre exceções-no-boundary do OmniVoice

**Classification:** PATCH_PLAN
**Status:** COMPLETED (2026-06-28) — ADR criado em docs/adr/0001-omnivoice-exception-boundary.md
**Priority:** Média
**Blast Radius:** Small (docs/)

## Descrição
Escrever um ADR (Architecture Decision Record) documentando explicitamente a divergência R1: o subsistema OmniVoice (`src/omnivoice/`) usa exceções C++ internas (`ov_throw` → `std::runtime_error`) capturadas na fronteira `extern "C"` (`ov_init`, `ov_synthesize` em omnivoice.cpp), enquanto o restante do repo adota "assert-abort + return codes, zero exceções" (verification claim #4 de 25/06).

Registrar: decisão, contexto (load-path com 97 call sites → unwinding via exceção é mais limpo), consequências (fronteira catch é a garantia; exceções nunca cruzam a ABI C), e a regra operacional (não propagar esse padrão para fora do subsistema isolado).

## Dependencies
- Nenhuma.

## Blockers
- Nenhum.

## Acceptance Criteria
- [ ] ADR criado (e.g. `docs/adr/OMNIVOICE-exception-boundary.md` ou em `_kiyotaka_project/`).
- [ ] States decision + why-safe + divergência do norm do repo + regra de contenção.
- [ ] Referencia evidência: ov-error.h:14-20, omnivoice.cpp:240-269 e 323-332.

## Required Tests
- Nenhum (documento). Aceitação = existeência + completude.

## Affected Files
- `docs/adr/` (novo) ou `_kiyotaka_project/` (novo ADR)
