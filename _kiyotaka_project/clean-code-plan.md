# Plano de Codigo Limpo — CrispASR

**Modo:** `project_evolution`
**Data:** 2026-06-25
**Base:** Evidencia coletada nas fases de observacao, escavacao e arquitetura

---

## 1. Estado Atual do Codigo

### 1.1 Niveis de Qualidade por Modulo

| Modulo | Estado | Problemas Principais |
|--------|--------|---------------------|
| `src/core/` (49 arquivos) | **Limpo** | Melhor modulo — zero TODOs, bem documentado, bem fatorado |
| `tests/` (186 arquivos) | **Limpo** | Zero TODOs, boas convencoes, null-handle testing |
| `src/crispasr_audio.cpp` | **Bom** | Zero TODOs, bem documentado, platform ifdefs controlados |
| `src/crispasr.cpp` (9,550 LOC) | **Critico** | God object, monolitico, 9 TODOs, 3 FIXMEs, decode loop de 1600 LOC |
| `src/crispasr_c_api.cpp` (7,752 LOC) | **Alto** | God-file, 40+ backends em 1 arquivo, compilacao condicional massiva |
| `examples/cli/` (102 arquivos) | **Moderado** | Dual code path, 2 TODOs |
| `ggml/` (~250k LOC) | **Moderado** | 379 TODOs/FIXMEs/HACKs, monolitico, custom patch |

### 1.2 Convencoes Atuais

| Elemento | Convencao | Consistencia |
|----------|-----------|-------------|
| Funcoes C | `snake_case` + `whisper_`/`crispasr_`/`ggml_` prefixo | Alta |
| Funcoes C++ | `snake_case` | Alta |
| Classes/Structs | `PascalCase` (WhisperBackend, CrispasrBackend) | Alta |
| Enums | `UPPER_SNAKE_CASE` com prefixo | Alta |
| Macros | `ALL_CAPS` | Alta |
| Constantes | `kCamelCase` (raro, so em audio.cpp) | Baixa |
| Arquivos | `snake_case` (crispasr_backend.cpp, core/mel.h) | Alta |
| Namespaces | `core_*` prefixo, nao C++ namespace | Media |
| Tratamento de erros | Assert-abort + return codes; zero excecoes | Alta |
| Comentarios | PLAN-tagged, sparse-to-moderate, secao dividers `//` | Media |

---

## 2. Plano de Limpeza

### 2.1 Prioridade Critica — Decomposicao de Monolitos

#### Tarefa CLN-001: Decompor `crispasr.cpp` (9,550 LOC)

**Problema:** Arquivo unico contendo model loading, encoding, decoding, VAD, grammar, DTW, alignment, mel spectrograms.

**Acao:** Separar em modulos:

```
src/
├── crispasr.cpp              ← Mantido como orquestrador (~2k LOC)
├── crispasr_model.cpp        ← whisper_model_load, hparams, vocab (~1.5k)
├── crispasr_encode.cpp       ← encode_internal, mel, conv (~1k)
├── crispasr_decode.cpp       ← decode loop, beam/greedy, sampling (~2k)
├── crispasr_kv_cache.cpp     ← KV cache init, free, find_slot (~500)
├── crispasr_vad_internal.cpp ← VAD model, detect, segments (~800)
├── crispasr_grammar.cpp      ← GBNF grammar parser (~700)
├── crispasr_dtw.cpp          ← DTW timestamp alignment (~500)
└── crispasr_align.cpp        ← Word alignment (~300)
```

**Blast radius:** ALTO — requer atualizacao de includes e possivelmente bindings
**Risco:** Regressao em precisao de saida
**Mitigacao:** Suite de testes de regressao existente + bit-exact validation
**Esforco estimado:** 1-2 semanas

#### Tarefa CLN-002: Extrair backend wrappers do `crispasr_c_api.cpp` (7,752 LOC)

**Problema:** 40+ backends com wrappers ABI no mesmo arquivo via `#if __has_include`.

**Acao:** Criar arquivos individuais:

```
src/api/
├── crispasr_api_core.cpp       ← Core API (session, params)
├── crispasr_api_whisper.cpp    ← Whisper ABI wrappers
├── crispasr_api_parakeet.cpp   ← Parakeet ABI wrappers
├── crispasr_api_canary.cpp     ← Canary ABI wrappers
├── ...                         ← Um por backend
└── crispasr_api_register.cpp   ← Tabela de registro
```

**Blast radius:** MEDIO
**Risco:** Build matrix — cada backend precisa ser compilado condicionalmente
**Mitigacao:** Padronizar macro de inclusao; testes verificam symbol export
**Esforco estimado:** 3-5 dias

### 2.2 Prioridade Alta — Divida Tecnica

#### Tarefa CLN-003: Resolver TODOs/FIXMEs criticos

| ID | Localizacao | Descricao | Prioridade |
|----|-------------|-----------|------------|
| `crispasr.cpp:8191` | `whisper_full_with_state` | "FIXME: will timestamp offsets be correct?" | ALTA |
| `crispasr.cpp:9302` | DTW | "FIXME: manual clip/transpose might not be most efficient?" | MEDIA |
| `crispasr.cpp:9366` | median_filter | "FIXME: Allocating mem everytime we call this func" | MEDIA |
| `crispasr.cpp:5045` | GPU VAD | "TODO: GPU VAD is forced disabled" | BAIXA |
| `crispasr.cpp:7525` | prompt | "TODO: do not recompute the prompt if same as previous" | BAIXA |

#### Tarefa CLN-004: Migrar legacy whisper path para unified dispatch

**Problema:** `cli.cpp` tem dois caminhos de codigo para whisper (legacy + backend).
**Acao:** Remover `whisper_full_parallel` path, usar apenas `WhisperBackend::transcribe`.
**Validacao:** Output byte-identico nos testes de regressao.
**Esforco:** 3-5 dias

#### Tarefa CLN-005: Padronizar `g_` prefix para globals

**Problema:** Globals usam `g_` prefixo inconsistente; `g_state`, `g_cpu_pools`, `g_progress`.
**Acao:** Auditar todos os globais e padronizar; documentar no cabecalho do arquivo.
**Esforco:** 1 dia

### 2.3 Prioridade Media — Higiene

#### Tarefa CLN-006: Adicionar linter Python e JavaScript

**Problema:** Apenas C++ tem linting configurado (clang-format + clang-tidy).
**Acao:**
- Python: Adicionar `ruff.toml` ou `.flake8`
- JavaScript: Adicionar `.eslintrc` ou usar biome
- CI: Adicionar workflow de lint para Python/JS
**Esforco:** 1-2 dias

#### Tarefa CLN-007: Normalizar nomenclatura de arquivos de teste

**Problema:** Tests usam `test-*.cpp`, `test_*.cpp`, `test-*.sh`, `test_*.py` — inconsistente.
**Acao:** Escolher um padrao (`test_*.ext`) e renomear.
**Esforco:** 1 dia

#### Tarefa CLN-008: Documentar god objects

**Problema:** `whisper_state` com 40 campos sem documentacao de decomposicao futura.
**Acao:** Adicionar comentario no struct documentando o plano de decomposicao e dependencias.
**Esforco:** 2 horas

### 2.4 Prioridade Baixa — Melhorias Continuas

#### Tarefa CLN-009: Converter `#define` rope types para enum

**Contexto:** `ggml/include/ggml.h:249-254` — comentario "TODO: convert to enum".
**Acao:** Contribuir a mudanca para upstream ggml.
**Esforco:** Desconhecido (depende do upstream)

#### Tarefa CLN-010: Migrar `core_*` prefix para C++ namespaces

**Problema:** Primitivas usam `core_mel::`, `core_bpe::` como prefixos de namespace manual.
**Acao:** Migrar para `namespace crispasr::core::mel {}` se compilador permitir.
**Risco:** Pode quebrar compatibilidade C; avaliar antes.

---

## 3. Plano de Execucao

### Fase 1 — Estabilizacao (1-2 semanas)
- CLN-003: Resolver FIXMEs criticos (8191, 9302, 9366)
- CLN-005: Padronizar globals
- CLN-006: Adicionar linting Python/JS
- CLN-007: Normalizar nomes de teste
- CLN-008: Documentar god objects

### Fase 2 — Refatoracao Estrutural (2-3 semanas)
- CLN-001: Decompor crispasr.cpp
- CLN-002: Extrair backend wrappers
- CLN-004: Migrar legacy whisper path

### Fase 3 — Melhorias Continuas (ongoing)
- CLN-009, CLN-010
