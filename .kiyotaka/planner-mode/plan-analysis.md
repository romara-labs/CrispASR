# Analise do Plano: Kokoro Full GGUF

## Classificacao

MIGRATION_PLAN

## Fases

1. Observacao e contrato GGUF
2. Loader de metadados full
3. Origem de voz embutida
4. Integracao ABI/CLI
5. Testes e smoke com GGUF real
6. Documentacao publica

## Arquivos afetados previstos

- `src/kokoro.cpp`
- `src/kokoro.h`
- `examples/cli/crispasr_backend_kokoro.cpp`
- `models/convert-kokoro-to-gguf.py`
- `docs/tts.md`
- `docs/architecture.md`
- `tests/test-kokoro-params.cpp`
- possivel novo teste `tests/test-kokoro-full-gguf.cpp`

## Dependencias

- A fase 2 depende da confirmacao direta dos metadados dos GGUFs full reais.
- A fase 3 depende da decisao de representacao interna: tensor GGML F32 residente versus vetor F32 e tensor auxiliar.
- A fase 4 depende da ABI escolhida para listar/selecionar vozes embutidas.
- A fase 5 depende de pelo menos um GGUF full local acessivel ou fixture minima.

## Decisoes recomendadas

- Manter o runtime atual e adicionar caminho full incremental.
- Converter estilos embutidos para F32 no load, independentemente da precisao original.
- Usar `--voice` como id apenas quando nao for caminho existente.
- Se `--voice` vazio e o GGUF full tem vozes, selecionar a primeira voz listada em `kokoro.voices.ids`.

## Riscos de regressao

- Quebra de usuarios que dependem de `--voice` como path relativo inexistente.
- Offsets incorretos ao adaptar `(256,510)` para a logica atual que espera `(256,1,510)`.
- Divergencia de vocabulario se o GGUF full nao carregar `tokenizer.ggml.tokens`.
- Falha em backend GPU se o tensor F32 de estilo embutido nao for residente no backend correto.

