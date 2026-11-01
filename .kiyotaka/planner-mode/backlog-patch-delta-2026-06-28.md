# Backlog Patch — Delta Stabilization (2026-06-28)

## Reason
Follow-up PATCH_PLAN ao MIGRATION_PLAN Kokoro Full-GGUF (TK-0001..0005). Estabiliza a migração em 3 frentes (Z1/R1/D2-D3) com autorização explícita do mantenedor para editar os arquivos antes bloqueados.

## Tasks to add
- TK-0006 Z1-test: auditar map_full_gguf_tensor_name contra tensores reais (Small)
- TK-0007 Z1-fix: corrigir regras de mapeamento (Medium) — dep TK-0006
- TK-0008 Z1-unit-test: teste sintético do mapeamento (Small) — dep TK-0007
- TK-0009 R1-doc: ADR exceções-no-boundary (Small)
- TK-0010 R1-adjust: alinhar divergência via doc/comment (Small) — dep TK-0009
- TK-0011 D2/D3-stabilize: remover/gate KOKORO_DEBUG_SHAPES (Medium) — bloqueado por condição externa

## Confirmation checklist
- [x] Provenância confirmada (omnivoice.cpp-master, voicer.cpp) — observação direta
- [x] Ground truth Z1 localizado (convert-gguf/*.gguf + kokoro-gguf.cpp)
- [x] D4 descartado por decisão do mantenedor
- [x] Autorização de edição escopada registrada
- [x] TK-0006..0011 não colidem com TK-0001..0005 (numeração sequencial)
- [ ] Sem backlog.md ativo no modo (output _kiyotaka_project/) — patch fica registrado aqui para aplicação futura se um backlog for criado
