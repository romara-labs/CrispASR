# TK-0003: Selecionar voz embutida por ABI e fallback

## Objetivo

Expor operacoes minimas para listar e selecionar vozes embutidas.

## Acceptance Criteria

- ABI C permite consultar quantidade/ids de vozes embutidas.
- ABI C permite selecionar voz por id.
- Default deterministico selecionado quando modelo full tem vozes e o usuario nao informou voz.
- Erros sao claros para id inexistente.

## Required Tests

- Selecao default.
- Selecao por id valido.
- Falha por id invalido.
- Compatibilidade com voice pack separado.

## Affected Files

- `src/kokoro.cpp`
- `src/kokoro.h`

