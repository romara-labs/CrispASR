# Observacao do Repositorio — CrispASR

**Modo:** `project_evolution`
**Fase:** observation
**Data:** 2026-06-25

## Resumo

CrispASR e um framework de IA de fala on-device de larga escala (v0.8.4, MIT), construido em **C++17** sobre a biblioteca de computacao de tensores **ggml**. Suporta **22+ modelos ASR**, **23+ modelos TTS**, alem de LID, VAD, diarizacao, alinhamento, pontuacao, truecasing, watermarking e traducao — tudo por uma API C unificada.

## Stack

| Camada | Tecnologias |
|--------|-------------|
| Linguagem principal | C++17 |
| Build system | CMake 3.10-3.29 |
| Framework core | ggml (tensor computation) |
| Backends GPU | CUDA, Metal, Vulkan, OpenCL, SYCL, ROCm/HIP, CoreML, OpenVINO, WebGPU, +8 |
| Bindings | Python, Rust, Go, Java, JavaScript/WASM, Ruby, Dart/Flutter |
| Testes | Catch2 (C++), pytest (Python), go test, JUnit, minitest, Dart test |
| Linting | clang-format + clang-tidy (138 linhas de checks) |
| CI/CD | 16 workflows GitHub Actions |
| Infra | Docker Compose (CPU/CUDA/Vulkan), HF Space |

## Estrutura de Modulos

- **src/** (269 arquivos) — Engine core e implementacoes de modelos
- **src/core/** (49 arquivos) — Primitivas NN compartilhadas
- **ggml/** — Biblioteca de tensores com 15+ backends GPU
- **examples/cli/** (102 arquivos) — CLI unificada
- **examples/server/** — Servidor HTTP/WebSocket
- **bindings/** — Go, Java, JS/WASM, Ruby
- **python/**, **crispasr/** (Rust), **flutter/** — Pacotes de linguagem
- **tests/** (186 arquivos) — Suite de testes
- **tools/** (88 arquivos) — Benchmarks, kernels Kaggle
- **models/** (102 arquivos) — Conversao HF -> GGUF
- **docs/** (17 arquivos) — Documentacao extensa

## Sinais de Risco

| Sinal | Detalhe |
|-------|---------|
| TODO/FIXME/HACK | 338 ocorrencias (~69 no codigo do projeto) |
| Cobertura de testes | ~60% (161 testes / 269 src) |
| Bus factor | 2 contribuidores = ~99% dos commits |
| Arquivos grandes | crispasr.cpp (9550 LOC), crispasr_c_api.cpp (7752 LOC) |
| Sem linter Python/JS | Apenas C++ tem linting configurado |
| Modelos muito grandes | voxcpm2_tts.cpp (7488 LOC), qwen3_tts.cpp (7164 LOC) |

## Pontos de Entrada

- API C: `include/crispasr.h` (916 linhas)
- Engine: `src/crispasr.cpp` (9550 linhas)
- CLI: `examples/cli/cli.cpp`
- Servidor: `examples/server/server.cpp`
- Python: `python/crispasr/__init__.py`
- Rust: `crispasr/src/lib.rs`
