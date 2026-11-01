# TK-0005: Atualizar conversor/documentacao do formato full

## Objetivo

Documentar e, se necessario, alinhar o conversor para o formato full.

## Acceptance Criteria

- Documentacao diferencia modelo antigo, voice pack separado, model-only e full.
- O contrato de metadados full e publicado.
- O conversor atual nao promete gerar full se ainda nao escreve vozes embutidas.

## Required Tests

- Validacao de metadata do GGUF gerado.

## Affected Files

- `models/convert-kokoro-to-gguf.py`
- `docs/tts.md`
- possivel doc dedicada em `docs/`.

