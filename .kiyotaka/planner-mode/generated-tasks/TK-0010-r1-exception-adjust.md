# TK-0010 — R1-adjust: Alinhar a divergência de exceções (mínimo)

**Classification:** PATCH_PLAN
**Status:** COMPLETED (2026-06-28) — cross-ref ADR adicionado em src/omnivoice/ov-error.h; auditoria da fronteira confirma que toda entrada extern "C" que pode throw (ov_init, ov_synthesize, ov_extract_voice_ref) está em try/catch → ov_status. Sem mudança comportamental.
**Priority:** Média
**Blast Radius:** Small (comentário/cross-ref)

## Descrição
"Ajuste" da divergência R1 — **escopo estrito: documentar/alinhar, NÃO remover o ov_throw**. OmniVoice é runtime isolado importado; a fronteira catch (`ov_init`/`ov_synthesize`) é a garantia de que exceções não cruzam a ABI C. Removê-las quebraria o load-path e a segurança da fronteira.

Ajuste = garantir que a divergência está explicitamente reconhecida no código (cross-reference ao ADR TK-0009) e que nenhum caller fora do subsistema possa receber uma exceção. Verificar que todos os pontos de entrada `extern "C"` de omnivoice têm try/catch (já confirmado em omnivoice.cpp para ov_init/ov_synthesize; checar ov_extract_voice_ref).

## Dependencies
- TK-0009 (ADR referencia).

## Blockers
- TK-0009.

## Acceptance Criteria
- [ ] Comentário em `src/omnivoice/ov-error.h` (ou omnivoice.cpp) cross-referencia o ADR.
- [ ] Audit confirma: toda entrada `extern "C"` pública de omnivoice está envolta em try/catch (nenhuma exceção escapa).
- [ ] Sem mudança comportamental na fronteira de exceções.

## Required Tests
- Build (sem regressão).
- (Opcional) teste que confirma status negativo (não crash) sob falha de load simulada.

## Affected Files
- `src/omnivoice/ov-error.h` (comentário) — mínimo
- possivelmente `src/omnivoice/omnivoice.cpp` (comentário cross-ref)
