# Plano: Suporte Kokoro Full GGUF

## Tipo

MIGRATION_PLAN

## Entrada

Migrar o suporte Kokoro do CrispASR para aceitar GGUF full com modelo, vocoder e vozes embutidas, nos formatos BF16, F16 e F32, conforme implementacao analisada em `/home/rodrigo/Documentos/gguf-onnx-migrations/voicer.cpp`.

## Escopo

- Preservar compatibilidade com o formato atual de modelo + voice pack separado.
- Adicionar leitura de metadados do formato full.
- Adicionar origem de estilo embutida para `voices.<id>.style`.
- Suportar conversao de estilo BF16/F16/F32 para F32 antes do grafo.
- Ajustar CLI para aceitar `--voice` como caminho antigo ou id embutido.
- Documentar o contrato GGUF esperado.

## Fora de escopo inicial

- Reescrever todo `src/kokoro.cpp` para a arquitetura modular do projeto externo.
- Remover suporte a `kokoro-voice-*.gguf`.
- Alterar os modelos ASR/TTS nao relacionados.
- Quantizar o modelo Kokoro alem de BF16/F16/F32 full.

## Bloqueador atual

A regra local em `AGENTS.md` declara: "Never delete, modify, or overwrite pre-existing project files." A execucao de codigo que edite `src/kokoro.cpp`, `src/kokoro.h`, CMake ou backend CLI depende de liberacao explicita dessa restricao ou de um modo de trabalho que permita patches nesses arquivos.

