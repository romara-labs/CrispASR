# Plano de Arquitetura Limpa — CrispASR

**Modo:** `project_evolution`
**Data:** 2026-06-25
**Base:** Leitura arquitetural + escavacao profunda

---

## 1. Estado Atual da Arquitetura

### 1.1 Avaliacao

| Dimensao | Nota | Comentario |
|----------|------|-----------|
| Separacao de camadas | 7/10 | Boa separacao C API / internal, mas monolitos internos |
| Gerenciamento de dependencias | 6/10 | Dependencia circular em ggml; custom patch fragil |
| Boundary enforcement | 8/10 | C ABI boundary forte; backend interface bem definida |
| Testabilidade | 6/10 | God objects e globals dificultam testes isolados |
| Extensibilidade | 7/10 | Strategy pattern para backends; mas factory e if/else chain |
| Documentacao arquitetural | 6/10 | ARCHITECTURE.md existe mas nao cobre god objects/planos |
| Tratamento de erros | 5/10 | Assert-abort sem graceful degradation |
| Gerenciamento de estado | 4/10 | Global mutable state + god object de 40 campos |

### 1.2 Anti-Padroes Criticos Identificados

| Anti-Padrao | Impacto | Urgencia |
|-------------|---------|----------|
| **God Object** (`whisper_state`, 40 campos) | Alta | Media |
| **God Files** (crispasr.cpp 9.5k, crispasr_c_api.cpp 7.7k) | Alta | Alta |
| **Global Mutable State** (g_cpu_pools, g_state) | Alta | Media |
| **Dual Code Path** (legacy + unified) | Media | Alta |
| **Custom Upstream Patch** (GGML_OP_AA_SNAKE_BETA) | Media | Alta |
| **Platform ifdef soup** (crispasr_audio.cpp) | Media | Baixa |
| **Factory if/else chain** (50 backends) | Baixa | Baixa |
| **Assert-Crash sem recuperacao** | Media | Baixa |

---

## 2. Visao Alvo — Arquitetura Limpa

### 2.1 Arquitetura em Camadas (Target State)

```
┌──────────────────────────────────────────────────────────────────┐
│  LAYER 1: INTERFACES PUBLICAS                                     │
│  ┌────────────┐ ┌──────────┐ ┌──────────┐ ┌──────────────────┐  │
│  │ C ABI      │ │ Python   │ │ Rust FFI │ │ Dart/Go/Java/... │  │
│  │ (include/) │ │ (ctypes) │ │ (sys)    │ │                  │  │
│  └─────┬──────┘ └────┬─────┘ └────┬─────┘ └────────┬─────────┘  │
│        └──────────────┴────────────┴───────────────┘             │
│                           │                                       │
├───────────────────────────┼───────────────────────────────────────┤
│  LAYER 2: API UNIFICADA   │                                       │
│  ┌────────────────────────▼──────────────────────────────────┐   │
│  │              Session API (session.cpp)                      │   │
│  │  • Unified session lifecycle                                │   │
│  │  • Model-agnostic params                                    │   │
│  │  • Progress/event callbacks                                 │   │
│  └────────────────────────┬──────────────────────────────────┘   │
│                           │                                       │
├───────────────────────────┼───────────────────────────────────────┤
│  LAYER 3: CORE SERVICES    │                                       │
│  ┌────────────────────────┼──────────────────────────────────┐   │
│  │  ┌──────────┐ ┌────────▼─────┐ ┌──────────────────────┐   │   │
│  │  │ Model    │ │ Capability   │ │ Backend Factory      │   │   │
│  │  │ Registry │ │ Matrix       │ │ (table-driven)       │   │   │
│  │  └──────────┘ └──────────────┘ └──────────┬───────────┘   │   │
│  │                                           │                │   │
│  │  ┌────────────────────────────────────────┼───────────┐   │   │
│  │  │         Backend Interface              │           │   │   │
│  │  │  • name(), capabilities()             │           │   │   │
│  │  │  • init(), transcribe(), synthesize() │           │   │   │
│  │  │  • shutdown()                         │           │   │   │
│  │  └────────────────────────────────────────┼───────────┘   │   │
│  └───────────────────────────────────────────┼───────────────┘   │
│                                              │                    │
├──────────────────────────────────────────────┼────────────────────┤
│  LAYER 4: BACKENDS                           │                    │
│  ┌──────────┐ ┌──────────┐ ┌──────────┐      │                    │
│  │ Whisper  │ │ Parakeet │ │ Canary   │ ...  │  (50 backends)    │
│  │ Backend  │ │ Backend  │ │ Backend  │      │                    │
│  └────┬─────┘ └────┬─────┘ └────┬─────┘      │                    │
│       └─────────────┴────────────┘            │                    │
│                           │                   │                    │
├───────────────────────────┼───────────────────┼────────────────────┤
│  LAYER 5: CORE PRIMITIVES │                   │                    │
│  ┌────────────────────────┼───────────────────┼────────────────┐   │
│  │  mel │ attention │ bpe │ fft │ rvq │ gguf_loader │ ...     │   │
│  └────────────────────────┼───────────────────┼────────────────┘   │
│                           │                   │                    │
├───────────────────────────┼───────────────────┼────────────────────┤
│  LAYER 6: INFRASTRUCTURE  │                   │                    │
│  ┌────────────────────────┼───────────────────┼────────────────┐   │
│  │           GGML Framework (tensor compute)                    │   │
│  │  CPU │ CUDA │ Metal │ Vulkan │ OpenCL │ ... (17 backends)   │   │
│  │  Memory Arena │ Graph Allocator │ Backend Scheduler         │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                           │                                       │
├───────────────────────────┼───────────────────────────────────────┤
│  LAYER 7: PLATFORM        │                                       │
│  ┌────────────────────────┼──────────────────────────────────┐   │
│  │  Audio I/O │ HTTP Server │ File System │ GPU Drivers       │   │
│  └────────────────────────────────────────────────────────────┘   │
└──────────────────────────────────────────────────────────────────┘
```

### 2.2 Regras Arquiteturais (Target)

| Regra | Descricao |
|-------|-----------|
| **Dependency Rule** | Dependencias apontam para baixo: Layer N so depende de Layer N+1 |
| **C ABI Boundary** | Unica interface publica; nada interno vaza para consumidores |
| **Backend Isolation** | Backends nao se comunicam diretamente; toda orquestracao via Layer 3 |
| **No Global Mutable State** | Estado pertence a contextos; globals somente read-only apos init |
| **Single Source of Truth** | Registro de modelos e unica fonte de metadados |
| **Error Propagation** | Erros propagam via return codes; assert somente para invariantes de programacao |

---

## 3. Iniciativas Arquiteturais

### 3.1 ARC-001: Decompor God Object `whisper_state`

**Estado atual:** 40 campos em struct unico (`src/crispasr.cpp:1300-1401`)

**Estado alvo:** Composicao de contextos especializados:

```cpp
struct whisper_state {
    // Core state (essencial para todas as operacoes)
    whisper_decoder_state  decoder;   // decoder state, sequences
    whisper_kv_cache_state kv_cache;  // KV cache
    whisper_model_state    model;     // model reference

    // Optional subsystems (inicializados sob demanda)
    whisper_vad_state *    vad;       // VAD state (lazy init)
    whisper_grammar_state * grammar;  // Grammar state (lazy init)
    whisper_dtw_state *    dtw;       // DTW alignment (lazy init)
    whisper_align_state *  align;     // Word alignment (lazy init)
};
```

**Beneficios:**
- Cada subsistema pode ser testado isoladamente
- Memoria alocada apenas para features usadas
- Mudancas em VAD nao afetam grammar/DTW

**Esforco:** 1-2 semanas (depende de CLN-001)

### 3.2 ARC-002: Table-Driven Backend Factory

**Estado atual:** `if/else if` chain com 50 branches em `crispasr_create_backend()`

**Estado alvo:** Tabela de registro similar ao model registry:

```cpp
struct BackendEntry {
    const char * name;
    uint32_t capabilities;
    std::function<std::unique_ptr<CrispasrBackend>()> factory;
};

const BackendEntry k_backends[] = {
    {"whisper",   CAP_WHISPER,   []() { return std::make_unique<WhisperBackend>(); }},
    {"parakeet",  CAP_PARAKEET,  []() { return std::make_unique<ParakeetBackend>(); }},
    // ...
};

std::unique_ptr<CrispasrBackend> crispasr_create_backend(const char * name) {
    for (auto & e : k_backends)
        if (strcmp(e.name, name) == 0) return e.factory();
    return nullptr;
}
```

**Beneficios:** Adicionar backend = 1 linha na tabela, sem editar factory function.

### 3.3 ARC-003: Migrar Custom GGML Patch Upstream

**Estado atual:** `GGML_OP_AA_SNAKE_BETA` em `ggml/include/ggml.h:582` exige re-aplicacao manual.

**Acao:** Contribuir a operacao anti-aliased SnakeBeta para upstream ggml.
**Se rejeitado:** Implementar como callback/extensao sem modificar enum upstream.
**Esforco:** 2-3 dias (contribuicao) ou 1 semana (workaround)

### 3.4 ARC-004: Unificar Code Paths

**Estado atual:** `cli.cpp` tem dois caminhos — legacy `whisper_full_parallel` e unified `CrispasrBackend::transcribe`.

**Acao:** Fazer `WhisperBackend::transcribe()` chamar `whisper_full_parallel` internamente; remover dispatch condicional do CLI.
**Validacao:** Output byte-identico nos testes de regressao.
**Esforco:** 3-5 dias

### 3.5 ARC-005: Substituir Platform ifdef por Abstracao

**Estado atual:** `crispasr_audio.cpp` com `#if defined(__APPLE__)`, `#ifdef _WIN32`, etc.

**Estado alvo:** Interface de audio platform-abstraction:

```cpp
class AudioDecoder {
    virtual bool decode(const char * path, std::vector<float> & pcm) = 0;
};

// Implementacoes:
class MiniaudioDecoder : public AudioDecoder { ... };
class CoreAudioDecoder : public AudioDecoder { ... };   // macOS
class MediaFoundationDecoder : public AudioDecoder { ... }; // Windows
class NDKDecoder : public AudioDecoder { ... };          // Android
```

**Esforco:** 1-2 semanas

### 3.6 ARC-006: Error Handling Strategy

**Estado atual:** Mix de assert-abort, return codes, nullptr, fprintf(stderr).

**Estado alvo:** Sistema em camadas:
- **Invariantes de programacao:** `CRISPASR_ASSERT` (mantido, crash-only)
- **Erros recuperaveis:** `crispasr_status` enum + propagacao
- **Alocacao:** `std::optional` ou fallback alocacao
- **GPU:** Status code + log + fallback para CPU

---

## 4. Guardas de Regressao

### 4.1 Estrategia de Testes para Refatoracoes

| Iniciativa | Tipo de Teste | Validacao |
|------------|---------------|-----------|
| ARC-001 (decompor god object) | Unit + Integration | Cada subsistema testado isoladamente |
| ARC-004 (unificar code paths) | Regression | Output byte-identico com referencia |
| ARC-002 (table factory) | Unit | Todos os backends instanciaveis |
| ARC-005 (platform abstraction) | Integration | Audio decode identico em cada plataforma |
| ARC-003 (ggml patch) | Performance + Correctness | Bit-exact output + benchmark |

### 4.2 Testes de Regressao Existentes

- 186 arquivos de teste (Catch2 C++, pytest Python, shell)
- Testes tagged por modulo: `[unit][mel]`, `[lcs][c-abi]`
- Testes de regressao por issue: `test-issue-79.cpp`, `test-issue-150-*.cpp`
- Python null-handle validation para cada funcao ABI
- CI regression workflow dedicado

### 4.3 Novos Testes Necessarios

| Teste | Cobertura |
|-------|-----------|
| Testes unitarios para cada modulo extraido de crispasr.cpp | Unit |
| Teste de regressao byte-exact apos unificar code paths | Regression |
| Teste de factory table-driven (tenta criar cada backend) | Integration |
| Teste de platform abstraction (mock decoder) | Unit |
| Benchmark comparativo pre/pos refatoracao | Performance |

---

## 5. Roadmap

### Short-term (1-2 sprints)
- ARC-004: Unificar code paths (menor risco, alto impacto)
- ARC-002: Table-driven factory (baixo risco)
- CLN-003: Resolver FIXMEs criticos

### Mid-term (2-4 sprints)
- ARC-001 + CLN-001: Decompor god object + crispasr.cpp
- CLN-002: Extrair backend wrappers
- ARC-006: Error handling strategy

### Long-term (4+ sprints)
- ARC-003: Contribuir ggml patch upstream
- ARC-005: Platform abstraction para audio
- CLN-009/010: Melhorias continuas (namespaces, rope types)
