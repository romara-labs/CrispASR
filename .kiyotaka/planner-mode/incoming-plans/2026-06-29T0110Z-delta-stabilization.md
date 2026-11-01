# Incoming Plan — Delta Stabilization (Kokoro/Omnivoice)

## Source
User (maintainer Rodrigo) via orchestrator hand-off to /kiyotaka-planner-mode.

## Received At
2026-06-29T01:10:17Z (UTC) / 2026-06-28 local

## Classification
PATCH_PLAN (follow-up to MIGRATION_PLAN TK-0001..TK-0005 of 21:26-21:28 today).

## Raw Plan

Provenância (mantenedor):
- omnivoice herdado de /home/rodrigo/Documentos/github/omnivoice.cpp-master/src/ → importado direto para CrispASR/src/omnivoice/.
- kokoro Full-GGUF herdado de /home/rodrigo/Documentos/gguf-onnx-migrations/voicer.cpp/src/kokoro/ → CrispASR/src/kokoro.cpp = reimplementação consolidada; map_full_gguf_tensor_name (kokoro.cpp:312-407) ponteia nomes externos→internos.
- Ambas as migrações ainda têm erros (WIP).

Decisões do mantenedor (4 itens):
- D4 (commit-readiness kokoro Full-GGUF): NÃO bloqueador, risco aceito. Sem TK.
- Z1: TESTAR e CORRIGIR map_full_gguf_tensor_name + is_full_gguf_conv_alias + sanity_check_weights.
- R1: DOCUMENTAR + AJUSTAR a divergência de exceções (omnivoice ov_throw/exception no boundary vs repo zero-exceções).
- D2/D3: ESTABILIZAR scaffolding KOKORO_DEBUG_SHAPES em core/lstm.h + kokoro.cpp.

Ground truth Z1: voicer.cpp/src/kokoro/kokoro-gguf.{cpp,h}; GGUFs reais convert-gguf/Kokoro-82M-v1.0-{BF16,F16,F32}.full.gguf (513 tensores) e F32.model-only.gguf (459); tools/export-kokoro.py.

## Authorization note
O mantenedor autorizou EXPLICITAMENTE a edição dos arquivos antes bloqueados pela regra non-negotiable, escopada a estes TKs (Z1-fix, R1-adjust, D2/D3-stabilize). TK-0001..0005 do run anterior registravam bloqueio pela regra; TK-0002/0003/0004 foram implementados desde então na working tree.
