# Kokoro Full GGUF Analysis

Data: 2026-06-28
Modo Kiyotaka: project_evolution

## Objetivo

Analisar a migracao do runtime Kokoro atual do CrispASR para aceitar o formato GGUF full gerado no projeto externo `gguf-onnx-migrations/voicer.cpp`, onde modelo, vocoder e vozes ficam no mesmo GGUF.

## Evidencias

- O runtime atual fica em `src/kokoro.cpp` e carrega o GGUF principal por `kokoro_init_from_file`.
- O runtime atual exige voz separada via `kokoro_load_voice_pack`.
- A ABI publica em `src/kokoro.h` documenta o formato de voz separado `general.architecture = "kokoro-voice"` com tensor unico `voice.pack[max_phon, 1, 256]`.
- O backend CLI em `examples/cli/crispasr_backend_kokoro.cpp` tenta `--voice`, fallback por idioma e `kokoro-voice-af_heart.gguf` ao lado do modelo.
- O ponto de uso de voz no grafo atual e `kokoro_voice_style_slice`, que indexa `c->vp.pack` e assume layout `voice.pack` carregado separadamente.
- O conversor atual `models/convert-kokoro-to-gguf.py` escreve pesos do modelo e vocabulario em `tokenizer.ggml.tokens`, mas explicitamente nao inclui `voices/*.pt`.
- O conversor atual `models/convert-kokoro-voice-to-gguf.py` escreve uma voz separada como tensor `voice.pack`.

## Formato externo observado

Fonte: `/home/rodrigo/Documentos/gguf-onnx-migrations/voicer.cpp/src/kokoro/kokoro-gguf.cpp` e `.h`.

O loader externo valida:

- `general.architecture = "kokoro"`
- `general.name`
- `kokoro.model_version`
- `kokoro.precision` em `F32`, `BF16` ou `F16`
- `kokoro.gguf_schema_version = 1`
- `kokoro.sample_rate = 24000`
- `kokoro.audio.channels = 1`
- `kokoro.n_token = 178`
- `kokoro.vocab.id_space_size`
- `kokoro.vocab.token_count`
- `kokoro.vocab.tokens`
- `kokoro.vocab.ids`
- `kokoro.voice.style_dim = 256`
- `kokoro.voice.max_context = 510`
- `kokoro.voices.count > 0`
- `kokoro.voices.ids`
- `kokoro.voices.source = "voices_pt_with_bin_crosscheck"`
- checksums por voz: `kokoro.voices.pt_sha256`, `kokoro.voices.bin_sha256`, `kokoro.voices.style_sha256`

As vozes ficam em tensores:

- `voices.<voice_id>.style`
- shape esperado: `ne[0] = 256`, `ne[1] = 510`
- tipo aceito: F32, BF16, F16 ou tensor GGML dequantizavel

O loader externo possui `copy_tensor_as_f32`, que converte F32/BF16/F16/quant para `std::vector<float>`.

## Incompatibilidades confirmadas

1. Origem da voz:
   - Atual: GGUF separado com `voice.pack`.
   - Externo: tensores de voz embutidos `voices.<id>.style`.

2. Layout da voz:
   - Atual: tensor 3D `voice.pack` com shape efetivo `(256, 1, 510)` e indice em `ne[2]`.
   - Externo: tensor 2D `(256, 510)` e indice em `ne[1]`.

3. Metadados de vocabulario:
   - Atual: `tokenizer.ggml.tokens` e `kokoro.vocab_size`.
   - Externo: `kokoro.vocab.tokens`, `kokoro.vocab.ids`, `kokoro.vocab.id_space_size`, `kokoro.vocab.token_count`.

4. Precisao:
   - Atual: pesos principais ja podem ser F16/F32 conforme conversor, mas voz separada e gravada como F32.
   - Externo: declara `kokoro.precision` e permite BF16/F16/F32 tambem para estilos de voz embutidos.

5. UX do CLI:
   - Atual: `--voice` e obrigatorio na pratica quando nao ha fallback.
   - Novo formato: `--voice` deve virar seletor de voz por id quando o modelo full tem vozes embutidas, mantendo compatibilidade com path GGUF separado.

## Hipotese de migracao minima

Adicionar suporte incremental sem remover o runtime atual:

1. Detectar no carregamento do modelo principal se existem chaves `kokoro.voices.ids` e tensores `voices.<id>.style`.
2. Indexar vozes embutidas no `kokoro_context`.
3. Manter `kokoro_load_voice_pack` para compatibilidade com GGUFs antigos.
4. Fazer `kokoro_voice_style_slice` escolher a origem:
   - se uma voz embutida foi selecionada, usar `voices.<id>.style`;
   - senao, usar `c->vp.pack`.
5. Converter BF16/F16/quant de voz para F32 ao carregar ou criar um tensor F32 residente, porque o grafo atual assume estilos F32.
6. Expor uma ABI pequena para listar/selecionar voz embutida.
7. Ajustar o backend CLI:
   - se `--voice` aponta para arquivo existente, usar o caminho antigo;
   - se `--voice` nao aponta para arquivo e ha vozes embutidas, tratar como voice id;
   - se `--voice` vazio e ha vozes embutidas, selecionar a primeira ou uma default documentada.

## Verificacao direta dos GGUFs reais

Fonte: leitura com `gguf.GGUFReader` dos arquivos em `/home/rodrigo/Documentos/gguf-onnx-migrations/voicer.cpp/convert-gguf`.

Arquivos observados:

- `Kokoro-82M-v1.0-BF16.full.gguf`: 192332736 bytes
- `Kokoro-82M-v1.0-F16.full.gguf`: 192332736 bytes
- `Kokoro-82M-v1.0-F32.full.gguf`: 355184960 bytes
- `Kokoro-82M-v1.0-F32.model-only.gguf`: 326980672 bytes

Metadados confirmados nos quatro arquivos:

- `general.architecture = "kokoro"`
- `general.name = "Kokoro-82M"`
- `kokoro.model_version = "v1.0"`
- `kokoro.gguf_schema_version = 1`
- `kokoro.sample_rate = 24000`
- `kokoro.audio.channels = 1`
- `kokoro.n_token = 178`
- `kokoro.vocab.id_space_size = 178`
- `kokoro.vocab.token_count = 114`
- `kokoro.voice.style_dim = 256`
- `kokoro.voice.max_context = 510`
- `kokoro.voices.count = 54`
- `kokoro.voices.source = "voices_pt_with_bin_crosscheck"`
- `kokoro.voices.ids` contem 54 ids; primeiros: `af_alloy`, `af_aoede`, `af_bella`, `af_heart`, `af_jessica`, `af_kore`, `af_nicole`, `af_nova`; ultimos: `zm_yunxi`, `zm_yunxia`, `zm_yunyang`.

Precisao e tensores:

- BF16 full: `kokoro.precision = "BF16"`, 513 tensores totais, 54 tensores de voz, tipos `F32:317`, `BF16:196`.
- F16 full: `kokoro.precision = "F16"`, 513 tensores totais, 54 tensores de voz, tipos `F32:317`, `F16:196`.
- F32 full: `kokoro.precision = "F32"`, 513 tensores totais, 54 tensores de voz, tipos `F32:513`.
- F32 model-only: `kokoro.precision = "F32"`, 459 tensores totais, 0 tensores de voz, tipos `F32:459`.

Os tensores de voz dos arquivos full seguem:

- nome: `voices.<voice_id>.style`
- shape: `(256, 510)`
- tipo observado: `F32` nos tres full, mesmo quando os pesos principais sao BF16 ou F16.

Nuance importante: `Kokoro-82M-v1.0-F32.model-only.gguf` preserva metadados `kokoro.voices.*`, mas nao contem tensores `voices.<id>.style`. Portanto, o runtime deve tratar "full" como presenca validada dos tensores de voz, nao apenas como `kokoro.voices.count > 0`.

## Riscos

- O exportador externo `tools/export-kokoro.py` lido nesta passagem parece mais antigo/incompleto que os GGUFs full reais: ele nao mostra escrita de `kokoro.voices.*` nem `voices.<id>.style` no trecho observado, embora os arquivos em `convert-gguf/` contenham esse contrato.
- A mudanca toca ABI, CLI e grafo de sintese; precisa de testes unitarios sem depender de GGUF grande e teste de smoke com um arquivo full real.
- BF16 em tensores de voz precisa ser convertido com cuidado antes de views por byte offset. Views diretas em BF16/F16 quebram os offsets atuais baseados em `sizeof(float)`.
- A regra local do repositorio diz para nao modificar arquivos preexistentes; por isso esta passagem cria plano e documentacao, mas nao aplica patch no runtime.

## Verificacao recomendada

1. Adicionar fixture GGUF minima ou teste sintetico com contexto GGML para validar selecao de origem de estilo.
2. Testar sintese curta com uma voz embutida e comparar nao-silencio/duracao/sample rate.
3. Testar que `model-only` falha com mensagem clara quando nenhuma voz separada foi carregada.
4. Testar regressao com modelo antigo + `kokoro-voice-af_heart.gguf`.
