# Current Incoming Plan — Delta Stabilization

## Source
User provided via /kiyotaka-planner-mode (orchestrator hand-off).

## Received At
2026-06-29T01:10:17Z

## Raw Plan
PATCH_PLAN: estabilização da migração Kokoro/Omnivoice (provenância: omnivoice ← omnivoice.cpp-master; kokoro ← voicer.cpp; ambas com erros residuais). 3 frentes: Z1 (testar+corrigir mapeamento de nomes), R1 (documentar+ajustar divergência de exceções), D2/D3 (estabilizar/remover debug scaffolding). D4 descartado como bloqueador (risco aceito pelo mantenedor). Cópia integral em `incoming-plans/2026-06-29T0110Z-delta-stabilization.md`.

## Authorization
Mantenedor autorizou edição dos arquivos antes bloqueados (src/kokoro.cpp, src/core/lstm.h, docs), escopada aos TKs TK-0006..TK-0011 deste plano.
