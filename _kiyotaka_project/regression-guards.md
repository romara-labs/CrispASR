# Guardas de Regressao — CrispASR

**Modo:** `project_evolution`
**Data:** 2026-06-25

---

## 1. Estado Atual da Suite de Testes

| Metrica | Valor |
|---------|-------|
| Arquivos de teste | 186 |
| Tipos de teste | Catch2 C++ (130), pytest Python (31), shell (22), JS (3) |
| Cobertura de arquivos | ~60% (161 testes / 269 src) |
| CI workflows com testes | regression.yml, bindings-go.yml, bindings-ruby.yml |
| Benchmarks | 4 (corpus, metrics, asr, asr_engines) |
| TODO/FIXME nos testes | **Zero** |

## 2. Estrategia de Regressao

### 2.1 Pipeline de Validacao

```
Mudanca de codigo
    │
    ▼
1. Lint (clang-format, clang-tidy)     ← CI: lint.yml
    │
    ▼
2. Build (todas plataformas)           ← CI: build.yml
    │
    ▼
3. Unit Tests (Catch2, pytest)         ← CI: regression.yml
    │
    ▼
4. Integration Tests (shell)           ← CI: regression.yml
    │
    ▼
5. Binding Tests (Go, Ruby)            ← CI: bindings-*.yml
    │
    ▼
6. Regression Tests (bit-exact)        ← CI: regression.yml
    │
    ▼
7. Benchmark Comparison                ← tools/ (manual)
```

### 2.2 Regras de Ouro

1. **Nenhuma refatoracao sem teste de regressao equivalente**
2. **Output de transcricao deve ser byte-identico** antes/depois de refatoracao
3. **Novo codigo requer novo teste** (manter razao de cobertura)
4. **FIXME resolvido = teste adicionado** para evitar regressao
5. **God object decomposto = testes isolados por modulo**

### 2.3 Categorias de Teste

| Categoria | O que testa | Exemplos |
|-----------|-------------|----------|
| **Unit** | Funcoes puras, algoritmos | mel, BPE, sentencepiece, LCS, FFT, ISTFT |
| **Parameter Validation** | Todos os backends aceitam params | `test-<backend>-params.cpp` |
| **C ABI Surface** | Symbol export + null-handle safety | `test_python_session.py` |
| **Integration** | CLI binary com audio real | `test-*.sh` |
| **Regression** | Output identico a referencia | `test_python_session.py:389-429` |
| **Issue-specific** | Regressao de bugs corrigidos | `test-issue-79.cpp`, `test-issue-150-*.cpp` |
| **Performance** | Benchmarks comparativos | `benchmark_asr.py` |

---

## 3. Testes Necessarios para Iniciativas de Evolucao

### 3.1 Decomposicao de crispasr.cpp (CLN-001, ARC-001)

| Teste | Tipo | Validacao |
|-------|------|-----------|
| `test-decomposed-encode` | Unit | Output do encoder identico pre/pos refatoracao |
| `test-decomposed-decode` | Unit | Output do decoder identico pre/pos refatoracao |
| `test-decomposed-vad` | Unit | Segmentos VAD identicos |
| `test-decomposed-grammar` | Unit | Parsing GBNF identico |
| `test-decomposed-dtw` | Unit | Alinhamento DTW identico |
| `test-full-pipeline` | Integration | Transcricao completa byte-identica |

### 3.2 Unificacao de Code Paths (ARC-004)

| Teste | Tipo | Validacao |
|-------|------|-----------|
| `test-legacy-vs-unified` | Regression | Output de whisper identico via ambos os paths |
| `test-all-params-legacy` | Parameter | Todos os whisper_params produzem mesmo resultado |

### 3.3 Table-Driven Factory (ARC-002)

| Teste | Tipo | Validacao |
|-------|------|-----------|
| `test-backend-factory-all` | Integration | Todo backend na tabela e instanciavel |
| `test-backend-capabilities` | Unit | Capabilities bitmask da tabela == backend->capabilities() |

---

## 4. Ferramentas de Regressao Existentes

| Ferramenta | Proposito | Localizacao |
|------------|-----------|-------------|
| `benchmark_asr.py` | Comparacao de performance ASR | `tools/` |
| `benchmark_corpus.py` | Benchmark em corpus completo | `tools/` |
| Reference backends | Validacao contra implementacoes de referencia | `tools/reference_backends/` |
| Diff system (`crispasr_diff`) | Comparacao de saida entre backends | `examples/cli/crispasr_diff.cpp` |
| Kaggle kernels | Validacao em datasets publicos | `tools/kaggle/` |

---

## 5. Plano de Acao Imediato

1. **Adicionar `test-full-pipeline`** — transcricao completa byte-exact como baseline antes de qualquer refatoracao
2. **Adicionar `test-decomposed-*`** — um para cada modulo extraido de crispasr.cpp
3. **Adicionar `test-legacy-vs-unified`** — validar que ambos os paths produzem saida identica
4. **Adicionar `test-backend-factory-all`** — criar todo backend da tabela
