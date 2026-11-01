# TK-0001: Validar contrato dos GGUFs full reais

## Status

COMPLETED nesta passagem.

## Objetivo

Confirmar o contrato real dos arquivos `Kokoro-82M-v1.0-*.full.gguf` antes de editar o runtime.

## Acceptance Criteria

- Metadados e tensores dos GGUFs BF16, F16 e F32 full foram inspecionados.
- `kokoro.precision`, `kokoro.gguf_schema_version`, `kokoro.voices.ids`, `kokoro.vocab.*` e `voices.<id>.style` foram confirmados ou negados.
- O resultado foi registrado em `_kiyotaka_project/kokoro-full-gguf-analysis.md`.

## Required Tests

- Dump de metadata/tensor index para cada GGUF full disponivel.

## Resultado

Inspecao feita com `gguf.GGUFReader` nos quatro arquivos Kokoro encontrados no projeto externo. Resultado consolidado em `_kiyotaka_project/kokoro-full-gguf-analysis.md`.

## Affected Files

- `_kiyotaka_project/kokoro-full-gguf-analysis.md`
