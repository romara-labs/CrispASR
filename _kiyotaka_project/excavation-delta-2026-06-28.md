# Escavação do Delta — CrispASR (2026-06-28)

**Modo:** `project_evolution`
**Fase:** excavation (delta)
**Escavador:** kiyotaka-excavator
**Data:** 2026-06-28
**Base:** observa `observation-delta-2026-06-28.md`; aprofunda módulo-a-módulo o trabalho novo.

## Resumo executivo

O delta é **majoritariamente código de alta qualidade**, não dívida. OmniVoice é um subsistema production-grade com tratamento de erro exemplar. O Kokoro Full-GGUF é correto no ponto crítico que a análise temia (conversão de precisão) e frágil num ponto que a análise **não** enfatizou (tabela hardcoded de mapeamento de nomes de tensores). Há uma **divergência arquitetural notável**: OmniVoice introduz exceções C++ internas onde o resto do repo tem zero.

**Encontrados: 0 TODO/FIXME em ~13,2K LOC novos. 3 zonas de falha (1 alta, 2 baixas). 2 duplicações intencionais. 3 regras implícitas.**

---

## Module Cards

### M1 — OmniVoice: carregamento de pesos (`gguf-weights.h` + `weight-ctx.h`)
- **Propósito:** loader GGUF format-independente para todos os componentes do modelo.
- **Complexidade:** high (mas bem fatorado).
- **Signature:** naming `gf_*`/`wctx_*`, funções curtas, comentários densos, error handling thorough. Confiança: confirmed.
- **Padrões notáveis:**
  - `gf_load` valida que **todo tensor cabe no mmap** antes do read do backend (gguf-weights.h:140-155) — previne segfault em `cuMemcpyHtoDAsync` em download truncado. **Melhor que o padrão típico do repo.**
  - `gf_load_conv_f16` (gguf-weights.h:296-333): Conv1d → F16 obrigatório em ARM aarch64 (im2col asserta src0=F16), com dequant Q8_0/Q4_K/Q5_K/Q6_K via type traits. Resolve explicitamente a zona de precisão.
  - `WeightCtx` (weight-ctx.h): pending-copies com byte-offset para tensores fundidos; staging buffers em `unique_ptr` para manter endereço estável.
- **Dívida:** nenhuma observada.

### M2 — OmniVoice: fronteira ABI (`omnivoice.cpp`)
- **Propósito:** implementação da ABI pública C99; converte falhas internas em `ov_status`.
- **Complexidade:** medium.
- **Signature:** extern "C", ownership explícita, comentários densos. Confiança: confirmed.
- **Padrões notáveis:**
  - `ov_init` (omnivoice.cpp:212-272): try/catch em torno da cadeia de load; `ov_free` é **idempotente em estado parcial** (unwind correto). Guarda `abi_version`.
  - `ov_synthesize` (omnivoice.cpp:286-333): try/catch defense-in-depth — exceção **nunca** cruza `extern "C"`; vira `OV_STATUS_GENERATE_FAILED`.
  - `g_last_error` thread_local; log callback `std::atomic` (lock-free publish).
- **Dívida:** nenhuma observada.

### M3 — Kokoro Full GGUF (`kokoro.cpp:297-576`)
- **Propósito:** aceitar GGUF "full" do projeto externo `gguf-onnx-migrations/voicer.cpp` (modelo+vozes no mesmo arquivo).
- **Complexidade:** very-high (na região nova).
- **Padrões:**
  - `tensor_to_f32` (kokoro.cpp:488-510): copy-then-convert — `ggml_backend_tensor_get` → `traits->to_float`. **CORRETO.**
  - `init_embedded_voices` (kokoro.cpp:512-576): valida shape (256,510); F32 usa alias direto no buffer do modelo, não-F32 converte para F32 num contexto dedicado; auto-seleciona a primeira voz.
  - `map_full_gguf_tensor_name` (kokoro.cpp:312-407): **tabela hardcoded de 90+ regras `replace_all`** mapeando nomes externos → internos.
  - `add_full_gguf_tensor_aliases` (kokoro.cpp:418-486): detecta 2D não-embedding não-conv e **transpõe em CPU no load** via loop manual.
- **Dívida:** ver Z1/Z2 abaixo.

### M4 — Debug scaffolding (`core/lstm.h` + `kokoro.cpp kokoro_debug_*`)
- **Propósito:** instrumentação para debugar o grafo do vocoder MI-LSTM.
- **Padrão:** shims (`lstm_debug_mul_mat`, `kokoro_debug_mul_mat`, `kokoro_debug_conv_1d`) gateados por env var `KOKORO_DEBUG_SHAPES`, logam shapes de mul_mat/conv.
- **Avaliação:** env-gated (custo zero quando desligado), nomeados `_debug_`. **Dívida temporária de severidade baixa**, mas introduz surface nova em `core/` (o módulo mais limpo per claim #24 da verificação 25/06).

---

## Zonas de falha (ranqueadas)

| # | Zona | Severidade | Evidência | Impacto provável |
|---|---|---|---|---|
| **Z1** | `map_full_gguf_tensor_name` — mapeamento hardcoded de nomes | **ALTA** | kokoro.cpp:312-407 (90+ `replace_all`) | Se o exporter externo mudar convenção de naming (nova versão Kokoro, outro conversor), aliases são produzidos errados/ausentes **silenciosamente**. `sanity_check_weights` (kokoro.cpp:581+) mitiga com subset, mas não garante cobertura total. Hazard de **ordenação** entre `replace_all` encadeados (ex.: `.norm1.fc.`→`.adain1.` antes de `.adain1.0.fc.`→`.adain1.0.`). |
| **Z2** | transpose+convert no load (`add_full_gguf_tensor_aliases`) | baixa | kokoro.cpp:460-464, 443-481 | Correto, mas O(nelements) de memória CPU extra por tensor transposto, pico simultâneo em `transposed_data`. Heurística `is_full_gguf_conv_alias` (substring) decide o que pular — nome que coincida com `.conv`/`F0_proj` etc. é erroneamente não-transposto. |
| **Z3** | debug shims em `core/lstm.h` | baixa | core/lstm.h:45-67 | Permanece em `core/`; se esquecido, polui o módulo mais limpo. Env-gated → sem custo de runtime. |
| **Z4** | aliasing de ponteiro F32 voice → buffer do modelo | baixa | kokoro.cpp:536-537 (`c->embedded.styles[id] = t`) | Lifetime implícito: `embedded.styles` aponta para `c->tensors`. Seguro enquanto ambos vivem em `c`, mas é um acoplamento de lifetime não-documentado. |

**Resolução positiva de risco:** o `kokoro-full-gguf-analysis.md` §Riscos temia "views diretas em BF16/F16 quebram offsets baseados em sizeof(float)". **A implementação evita isso** — `tensor_to_f32` copia para buffer próprio antes de converter (kokoro.cpp:500-508). O medo era hipótese; o código é seguro neste ponto.

---

## Duplicação (intencional, não dívida acidental)

| # | Duplicação | Evidência | Avaliação |
|---|---|---|---|
| **DPL1** | OmniVoice traz próprio `audio-io.h`, `audio-resample.h`, `wav.h`, `audio-postproc.h` embora `core/` já tenha `wav_reader.h`, `audio_resample.{h,cpp}`, `audio_chunking.h` | `ls src/omnivoice/` vs `ls src/core/`; adapter (crispasr_backend_omnivoice.cpp:13-14) usa `core/` para o WAV de referência | **Isolamento deliberado** — o adapter diz "imported as an isolated native GGML runtime". Há 2 wav readers e 2 resamplers no repo. Aceitável como boundary, mas vale registrar. |
| **DPL2** | OmniVoice tem próprio idiom de load de pesos (`WeightCtx`/`wctx_*` em `weight-ctx.h`, usado por semantic-enc/dac-enc/dac-dec/hubert-enc), separado do loader do repo em `crispasr.cpp` | `grep WeightCtx` | Mesma filosofia de isolamento. Kokoro Full-GGUF (M3) reimplementa convert-at-load **ad-hoc** em vez de reusar qualquer um dos dois. |

---

## Regras implícitas descobertas

| # | Regra | Evidência | Observação |
|---|---|---|---|
| **R1** | **OmniVoice usa exceções C++ internas (`ov_throw`)** capturadas na fronteira extern "C" | ov-error.h:14-20; omnivoice.cpp:240-269, 323-332 | **Divergência arquitetural**: a verificação 25/06 (claim #4) registrou o repo como "assert-abort + return codes, **zero exceções**". OmniVoice introduz `std::runtime_error` interno. A fronteira catch torna seguro externamente, mas é um idiom diferente convivendo no mesmo repo. |
| **R2** | Kokoro Full-GGUF faz convert/transpose **no load** (CPU) ao invés de nós de cast no runtime | kokoro.cpp:443-481, 547-569 | Espelha a filosofia do `gf_load_tensor_f32` do OmniVoice ("eliminates runtime cast nodes"), mas reimplementada ad-hoc. |
| **R3** | Estratégias de versão de ABI **diferentes** dentro do repo | OmniVoice: `OV_ABI_VERSION` + campo `abi_version` + reject no runtime (omnivoice.h:57, omnivoice.cpp:218). Kokoro: novas funções C aditivas sem gate de versão (kokoro.h:74-80) | Aceitável (C aditivo é backward-compat), mas não-uniforme. |

---

## Status do D4 (análise diz "não patche", código está "patcheado")

- **Fato:** `kokoro-full-gguf-analysis.md` (hoje 21:28) §Riscos afirma "esta passagem cria plano e documentação, mas **não aplica patch no runtime**".
- **Fato:** a working tree **contém** a implementação (`kokoro_embedded_voice_*`, `init_embedded_voices`, etc.), não commitada (`git diff`).
- **Inferência:** o plano foi **aplicado após** a escrita da análise (ou em paralelo). A implementação cobre os 7 passos da "hipótese de migração mínima" do documento (detecção, indexação, compat legada, seleção de origem, convert F32, ABI pequena, ajuste do CLI) — parece **completa em escopo**, mas:
- **Bloqueador para tratar como pronto:** falta confirmação de (a) ordenagem temporal código↔análise, (b) smoke test com GGUF full real (recomendação §"Verificação recomendada" 1-4 do doc), (c) teste de regressão com modelo antigo + voice pack separado.

---

## Separação fato / inferência / hipótese / bloqueador

- **Fato:** OmniVoice é código limpo (0 TODO), ABI sólida, loader com guards de truncamento e precisão. Kokoro `tensor_to_f32` é seguro (copy-then-convert).
- **Fato:** `map_full_gguf_tensor_name` é uma tabela hardcoded de 90+ regras com hazard de ordenação.
- **Fato:** OmniVoice usa exceções internas; resto do repo não.
- **Inferência:** DPL1/DPL2 são isolamento deliberado, não dívida acidental.
- **Hipótese:** a implementação Kokoro Full-GGUF está completa em escopo (cobre os 7 passos do plano).
- **Bloqueador:** D4 não pode virar "pronto p/ commit" sem smoke test + regressão + confirmação de ordenagem.

## Notas metodológicas

- Não re-escavou o repo inteiro (25/06). Foco no delta.
- Não editou código. Não sobrescreveu `modules.json` (regra non-negotiable) — criou artefato delta novo.
- Cada finding aponta módulo + evidência (file:line) + impacto provável, conforme critério de qualidade do excavator.
