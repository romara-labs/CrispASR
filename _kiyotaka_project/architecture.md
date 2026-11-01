# Leitura Arquitetural — CrispASR

**Modo:** `project_evolution`
**Fase:** architecture
**Data:** 2026-06-25

## 1. Visao Geral da Arquitetura

```
┌──────────────────────────────────────────────────────────────────┐
│                    CONSUMIDORES EXTERNOS                          │
│  CLI (cli.cpp) │ Python (ctypes) │ Rust (FFI) │ Dart (FFI)       │
│  Go (CGo) │ Java (JNA) │ JS (WASM) │ Ruby (C-ext)               │
└───────────────────────────────┬──────────────────────────────────┘
                                │
┌───────────────────────────────▼──────────────────────────────────┐
│              C ABI FACADE (crispasr_c_api.cpp, 7,752 LOC)         │
│  • Unified session API (crispasr_session_*)                       │
│  • 40+ backend wrappers (#if __has_include guards)                │
│  • Parameter getters/setters (boilerplate)                        │
│  • Progress polling, stereo decode, watermarking                  │
└───────────────────────────────┬──────────────────────────────────┘
                                │
┌───────────────────────────────▼──────────────────────────────────┐
│              BACKEND DISPATCH LAYER                               │
│  • CrispasrBackend (virtual interface)                            │
│  • Capability matrix (23-bit bitmask)                             │
│  • Model registry (1,171-line static table)                       │
│  • GGUF auto-detection (2-pass: filename + metadata)              │
│  • Factory (crispasr_create_backend)                              │
│  • 50+ concrete backend adapters                                  │
└───────┬───────────────────────┬───────────────────────┬──────────┘
        │                       │                       │
┌───────▼───────┐   ┌───────────▼───────┐   ┌───────────▼───────┐
│  ASR Backends │   │  TTS Backends     │   │  Support Backends │
│  • whisper    │   │  • bark_tts       │   │  • VAD (silero,   │
│  • parakeet   │   │  • cosyvoice3     │   │    marblenet,     │
│  • canary     │   │  • kokoro         │   │    firered)       │
│  • qwen3_asr  │   │  • dia_tts        │   │  • LID (ecapa,    │
│  • granite    │   │  • f5_tts         │   │    firered,       │
│  • voxtral    │   │  • ... (23 total) │   │    silero)        │
│  • ... (22+)  │   │                   │   │  • Diarize        │
│                │   │                   │   │  • Align          │
└───────┬───────┘   └───────────┬───────┘   │  • Enhance        │
        │                       │           │  • Watermark      │
        │                       │           └───────────────────┘
        │                       │
┌───────▼───────────────────────▼──────────────────────────────────┐
│              CORE PRIMITIVES (src/core/, 49 files)                 │
│  mel (spectrograms) │ attention (MHA) │ bpe (tokenizer)          │
│  fft (FFT) │ rvq (quantization) │ gguf_loader (weight loading)  │
│  sentencepiece │ beam_decode │ greedy_decode │ conformer         │
│  HiFi-GAN │ LSTM │ CTC │ SNAC │ ...                              │
└───────────────────────────────┬──────────────────────────────────┘
                                │
┌───────────────────────────────▼──────────────────────────────────┐
│              GGML FRAMEWORK (ggml/, ~250k LOC)                     │
│  • Computation graph (define-then-execute)                        │
│  • 17 GPU backends: CPU, CUDA, Metal, Vulkan, OpenCL, SYCL,      │
│    ROCm/HIP, CoreML, OpenVINO, WebGPU, Hexagon, BLAS, RPC,       │
│    virtGPU, ZENN, ZENdnn, CANN, MUSA                             │
│  • Memory arena + graph allocator                                 │
│  • Backend scheduler (multi-device split)                         │
│  • Operation fusion framework                                     │
└──────────────────────────────────────────────────────────────────┘
```

## 2. Decisoes Arquiteturais Identificadas

### 2.1 Decisoes Explicitas (encontradas no codigo)

| Decisao | Evidencia | Impacto |
|---------|-----------|---------|
| **Manter legacy code path + unified dispatch** | `cli.cpp` tem dois caminhos: `whisper_full_parallel` vs `CrispasrBackend::transcribe` | Dupla manutencao, saida byte-identica para whisper |
| **Reutilizar `whisper_params` para novos backends** | `crispasr_backend.h:26` — "instead of introducing a parallel crispasr_params" | Evita duplicacao mas acopla backends ao legado |
| **Header-only para primitivas core** | `bpe.h`, `fft.h`, `attention.h` — zero arquivos .cpp | Menor complexidade de build, custo de recompilacao |
| **Enums como substituto de Strategy Pattern** | `mel.h` — 7 enums controlam comportamento do algoritmo | C-compativel, sem vtable, mas switch-case complexo |
| **Tabela estatica de registro de modelos** | `crispasr_model_registry.cpp` — `constexpr Entry k_registry[]` de 1171 linhas | Sem plugins dinamicos, build monolitico |
| **GGML fork com patch customizado** | `GGML_OP_AA_SNAKE_BETA` em `ggml/include/ggml.h:582` — "MUST RE-APPLY after every ggml bump" | Risco de merge a cada sync upstream |

### 2.2 Decisoes Implicitas (padroes sem documentacao formal)

| Padrao | Evidencia | Inferencia |
|--------|-----------|------------|
| **Assert-first error handling** | 1100+ `GGML_ASSERT`, 400+ em ggml.c, `CRISPASR_ASSERT` com `abort()` | Filosofia: falhe rapido. Nao ha recuperacao graciosa para erros internos. |
| **API C como unica fronteira publica** | Apenas `include/crispasr.h` e exportado; todo o resto e interno | Boundary arquitetural forte: consumidores externos nunca dependem de C++ interno |
| **Snake_case universal com prefixos de namespace** | `whisper_`, `crispasr_`, `ggml_`, `core_` | Convencao de namespacing via prefixo (C-style) ao inves de C++ namespaces |
| **PLAN-tagged como tracking de tarefas** | `PLAN #69a`, `PLAN #60e`, `§176f` no codigo | Sistema de rastreamento de implementacao vinculado a documentos de planejamento |
| **Monolito pragmatico** | Arquivos de 7-9k LOC, god objects com 40+ campos | Tradeoff consciente: simplicidade de build vs complexidade de manutencao |

## 3. Fronteiras Arquiteturais

### 3.1 Fronteiras Fortes

| Boundary | Tipo | Mecanismo |
|----------|------|-----------|
| **Public C API ↔ Internal C++** | Hard boundary | `extern "C"` + `CRISPASR_API` export macro; tipos opacos (`whisper_context*`) |
| **GGML ↔ CrispASR** | Library boundary | `ggml_*` API; backend abstraction via `ggml_backend_t` |
| **Backend Interface** | Plugin boundary | `CrispasrBackend` virtual class; `capabilities()` bitmask; factory via `if/else if` |
| **Core Primitives** | Internal library | `core_*` namespace; header-only; zero dependencia circular |

### 3.2 Fronteiras Fracas (acoplamento nao intencional)

| Boundary | Problema | Evidencia |
|----------|----------|-----------|
| **whisper_state ↔ todos os subsistemas** | God object de 40 campos acessado por VAD, gramatica, DTW, decoder, alinhamento | `src/crispasr.cpp:1300-1401` |
| **crispasr_c_api.cpp ↔ 40+ backends** | #if guards para cada backend; adicionar backend = editar arquivo de 7k LOC | `src/crispasr_c_api.cpp:49-276` |
| **cli.cpp ↔ legacy whisper path** | Codigo de 2.5k LOC duplica logica de dispatch | `examples/cli/cli.cpp` |
| **Custom GGML patch** | `GGML_OP_AA_SNAKE_BETA` modifica enum upstream; requer re-aplicacao manual | `ggml/include/ggml.h:582-585` |

## 4. Fluxos de Dados

### 4.1 Fluxo Principal — Transcricao ASR

```
Audio File/Stream
    │
    ▼
crispasr_audio_load_stereo()     ← Decodifica audio (miniaudio + fallbacks)
    │  PCM 16kHz mono/estereo f32
    ▼
VAD (Silero/MarbleNet/FireRed)   ← Detecta segmentos de fala
    │  Lista de timestamps [start, end]
    ▼
CrispasrBackend::transcribe()    ← Dispatched para backend especifico
    │  ┌─ whisper: mel → encoder → cross-attn → decoder → beam/greedy
    │  ├─ parakeet: fastconformer_ctc → transducer decoder
    │  └─ granite: conformer → CTC + LM rescore
    │  std::vector<crispasr_segment>
    ▼
Post-processing                   ← LID, punctuacao, truecasing, alinhamento
    │
    ▼
Output (JSON/SRT/VTT/TXT)        ← Formatted via crispasr_output.cpp
```

### 4.2 Fluxo — TTS (Sintese de Voz)

```
Texto de entrada
    │
    ▼
CrispasrBackend::synthesize()    ← Dispatched para backend TTS
    │  ┌─ kokoro: BPE encode → conformer → HiFi-GAN decoder
    │  ├─ bark_tts: semantic → coarse → fine → EnCodec decode
    │  └─ cosyvoice3: BPE → flow-matching → vocoder
    │  std::vector<float> (audio PCM)
    ▼
Session API                       ← Gerencia estado de sessao, cache de fonemas
    │
    ▼
Output audio (WAV)                ← crispasr_session_synthesize()
```

### 4.3 Fluxo — Streaming (Real-time)

```
Microfone/Captura
    │  Audio chunks (200ms-2s)
    ▼
VAD (voice activity detection)    ← Silero VAD em tempo real
    │  Speech segments
    ▼
CrispasrBackend::transcribe_streaming()  ← Streaming backend (whisper, parakeet)
    │  Resultado parcial a cada chunk
    ▼
WebSocket/HTTP SSE                 ← Server envia resultado incremental
    │
    ▼
LCS dedup                          ← Remove duplicacao entre chunks sobrepostos
```

## 5. Padroes Arquiteturais

### 5.1 Padroes Confirmados (evidencia repetida)

| Padrao | Tipo | Frequencia | Localizacao |
|--------|------|------------|-------------|
| **Facade (C ABI)** | Structural | Unico ponto | `crispasr_c_api.cpp` — unica interface publica |
| **Strategy (Backends)** | Behavioral | 50+ implementacoes | `crispasr_backend_*.cpp` — cada modelo e um strategy |
| **Factory (Backend Creation)** | Creational | Unico ponto | `crispasr_create_backend()` + `if/else if` chain |
| **Registry (Model Lookup)** | Data | Tabela estatica | `crispasr_model_registry.cpp` — 1171 linhas |
| **Computation Graph** | Architectural | Framework inteiro | `ggml` — define-then-execute |
| **Backend vtable** | Structural | 4 camadas | `ggml-backend-impl.h` — device, backend, buffer_type, buffer |
| **Memory Arena** | Structural | Framework inteiro | `ggml_context` — alocacao contigua |
| **Capability Matrix** | Behavioral | Todos os backends | 23-bit bitmask em `crispasr_backend.h` |

### 5.2 Anti-Padroes (violacoes estruturais)

| Anti-Padrao | Severidade | Evidencia | Impacto |
|-------------|-----------|-----------|---------|
| **God Object (whisper_state)** | Critica | 40 campos, `src/crispasr.cpp:1300-1401` | Qualquer mudanca em VAD/grammar/DTW afeta o struct inteiro |
| **God File (crispasr.cpp)** | Critica | 9,550 LOC unico arquivo | Dificil navegacao, merge conflicts, revisao |
| **God File (crispasr_c_api.cpp)** | Alta | 7,752 LOC unico arquivo | 40+ backends em um arquivo, compilacao condicional massiva |
| **Global Mutable State** | Alta | `g_cpu_pools`, `g_state` | Thread safety fragil, dificil testar isoladamente |
| **Dual Code Path (legacy + unified)** | Media | `cli.cpp` | Dupla manutencao, risco de divergencia |
| **Custom Upstream Patch** | Media | `GGML_OP_AA_SNAKE_BETA` | Risco de merge a cada sync do ggml |
| **Platform ifdef soup** | Media | `crispasr_audio.cpp` | 4+ plataformas com `#ifdef` espalhados |
| **Assert-Crash sem recuperacao** | Media | 1100+ `GGML_ASSERT` | Alocacao falha → abort, sem graceful degradation |
| **Circular Dependency (ggml backend)** | Baixa | `ggml-backend.cpp:2404` | Documentado com FIXME, impacta apenas CPU buffer types |

## 6. Dependencias e Acoplamento

### 6.1 Grafo de Dependencias

```
crispasr_c_api.cpp ──► crispasr_backend.h ──► src/*.cpp (modelos)
                                    │
                                    ├──► src/core/* (primitivas)
                                    │
                                    └──► ggml (framework)
                                           │
                                           ├──► ggml-cpu
                                           ├──► ggml-cuda
                                           ├──► ggml-metal
                                           └──► ... (17 backends)
```

### 6.2 Dependencias Externas

| Dependencia | Tipo | Uso |
|-------------|------|-----|
| **ggml** | Fork interno | Tensor computation, GPU dispatch |
| **miniaudio** | Vendored (95,844 LOC) | Audio capture/playback |
| **httplib** | Vendored (10,197 LOC) | HTTP server (examples/server) |
| **nlohmann/json** | Vendored (24,596 LOC) | JSON parsing (examples) |
| **stb_vorbis** | Vendored | OGG Vorbis decoding |
| **libopus** | Optional system lib | Opus audio decoding |
| **FDK-AAC** | Dynamic load (dlopen) | AAC audio decoding |

## 7. Divida Tecnica Arquitetural

| Item | Tipo | Custo Estimado |
|------|------|---------------|
| Quebrar crispasr.cpp (9,550 LOC) em modulos | Refatoracao | Alto — 1-2 semanas |
| Extrair backends do crispasr_c_api.cpp (7,752 LOC) | Refatoracao | Medio — 3-5 dias |
| Migrar legacy code path para unified dispatch | Consolidacao | Medio — 3-5 dias |
| Resolver custom GGML patch (AA_SNAKE_BETA) | Upstream | Baixo — contribuir upstream |
| Adicionar linter Python/JS | Tooling | Baixo — 1 dia |
| Documentar god objects e planos de decomposicao | Documentacao | Baixo — 1-2 dias |
| Substituir ifdef soup por abstracao de plataforma | Refatoracao | Alto — 1-2 semanas |
| Resolver 379 TODOs/FIXMEs no ggml | Divida upstream | Depende do upstream |

## 8. Recomendacoes Arquiteturais

1. **Preservar o boundary C ABI** — e a decisao arquitetural mais saudavel do projeto
2. **Migrar legacy whisper path para unified dispatch** — eliminar dupla manutencao
3. **Decompor crispasr.cpp** — separar VAD, gramatica, DTW, decoder em modulos
4. **Extrair backend wrappers** do `crispasr_c_api.cpp` para arquivos por-backend
5. **Contribuir `GGML_OP_AA_SNAKE_BETA` upstream** para eliminar patch customizado
6. **Migrar de if/else if factory para tabela de registro** — similar ao model registry
7. **Adicionar CI para Python/JS linting**
8. **Documentar todas as decisoes arquiteturais em ARCHITECTURE.md**
