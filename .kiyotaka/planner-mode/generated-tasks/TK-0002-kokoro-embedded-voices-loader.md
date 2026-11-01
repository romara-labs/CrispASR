# TK-0002: Adicionar loader de vozes embutidas no contexto Kokoro

## Objetivo

Adicionar suporte interno a `voices.<id>.style` sem remover o caminho `voice.pack` separado.

## Acceptance Criteria

- O contexto Kokoro detecta e armazena vozes embutidas do GGUF principal.
- Estilos BF16/F16/F32 sao convertidos para F32 antes de serem usados pelo grafo.
- O caminho antigo `kokoro_load_voice_pack` continua aceito.

## Required Tests

- Unit test de conversao de estilo BF16/F16/F32 para F32.
- Unit test de runtime sem vozes embutidas.

## Affected Files

- `src/kokoro.cpp`
- `src/kokoro.h`
- `tests/test-kokoro-params.cpp` ou novo teste Kokoro full.

