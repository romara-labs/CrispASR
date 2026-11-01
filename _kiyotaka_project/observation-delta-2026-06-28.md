# Observação do Delta — CrispASR (2026-06-28)

**Modo:** `project_evolution`
**Fase:** observation (delta re-observação)
**Observador:** kiyotaka-observer
**Data:** 2026-06-28
**Base de comparação:** `_kiyotaka_project/observation.md` (2026-06-25)

## Resumo

Trabalho novo, não capturado pela observação de 25/06. Duas frentes distintas:

1. **Novo subsistema OmniVoice TTS** — `src/omnivoice/` (37 arquivos, ~13,2K LOC), completamente integrado (build + factory + CLI adapter). Limpo: **0 TODO/FIXME**.
2. **Kokoro "Full GGUF"** — migração do runtime Kokoro para aceitar vozes embutidas (`voices.<id>.style`) no GGUF principal, documentada em `kokoro-full-gguf-analysis.md` (hoje, 21:28). Implementação presente na working tree, não commitada.
3. **Instrumentação de debug** — shims em `core/lstm.h` e `kokoro.cpp` gateados por `KOKORO_DEBUG_SHAPES`, consistentes com a trilha de commits recentes sobre o grafo do vocoder MI-LSTM (32K → 65K → 131K nós).

Magnitude do delta: **7 arquivos modificados (+542/-38) + 38 arquivos novos (~13,4K LOC)**.

---

## Stack adicionada (fatos)

| Item | Valor | Evidência |
|---|---|---|
| Novo modelo TTS | OmniVoice (Qwen3 backbone + RVQ codec, 24 kHz mono) | `omnivoice.h:1-15`, `src/CMakeLists.txt:1427-1445` |
| Paradigma | MaskGIT-TTS sampler + DAC codec + voice cloning | `omnivoice.h:181-250` (struct `ov_tts_params` com 7 campos `mg_*`) |
| Componentes do pipeline | HuBERT/qwen3/semantic encoders, BPE tokenizer, RVQ, DAC enc/dec, voice-design, duration-estimator, lang-map, text-chunker (buffered + streaming) | `find src/omnivoice -type f` (37 arquivos) |
| Multilinguismo | lang-map + `lang` aceita "" (auto), "en", "zh" | `omnivoice.h:184-187`, `lang-map.h` (698 LOC) |
| ABI pública | Pure C99, prefixo `ov_`, `OV_API` visibility, `OV_ABI_VERSION 3` | `omnivoice.h:31-57` |
| Estilo | Segue `whisper.h` / `llama.h`: extern "C", POD structs, const char* UTF-8, status enum | `omnivoice.h:1-15` |

## Estrutura — `src/omnivoice/` (novo, 37 arquivos)

Arquivos de alto valor (LOC):

| Arquivo | LOC | Papel |
|---|---|---|
| `pipeline-tts.cpp` | 1447 | Pipeline TTS (maior) |
| `duration-estimator.h` | 1233 | Estimativa de duração |
| `lang-map.h` | 698 | Mapa de idiomas |
| `hubert-enc.h` | 691 | Encoder HuBERT |
| `bpe.h` | 633 | Tokenizador BPE |
| `omnivoice.cpp` | 441 | Implementação da ABI pública |
| `audio-io.h` | 487 | I/O de áudio |
| `voice-design.h` | 460 | Design de voz por prompt |
| `dac-decoder.h` | 428 | Decoder DAC |
| `gguf-weights.h` | 445 | Carregamento de pesos GGUF |
| `pipeline-codec.cpp` | 799 | Pipeline do codec |
| `omnivoice.h` | 313 | **ABI pública** (entry point) |

(Remainder: `audio-postproc{,-stream}.h`, `maskgit-tts.h`, `prompt-tts.h`, `qwen3-enc.h`, `rvq-{codec,file}.h`, `text-chunker{,-stream}.h`, `wav.h`, `srt.h`, `philox.h`, `utf8.h`, `backend.h`, `weight-ctx.h`, `debug.h`, `ov-error.h`, `semantic-enc.h`, `dac-encoder.h`, `audio-resample.h`, `omnivoice-llm.h`, `timer.h`, `version.h`.)

## Pontos de entrada / integração

- **Factory registration:** `examples/cli/crispasr_backend.cpp:182` — nomes `"omnivoice"` / `"omnivoice-tts"` / `"ov-tts"` → `crispasr_make_omnivoice_backend()`
- **Capabilities:** `CAP_TTS | CAP_VOICE_CLONING | CAP_STREAMING` (`crispasr_backend_omnivoice.cpp:44`)
- **CLI adapter:** `examples/cli/crispasr_backend_omnivoice.cpp` (173 LOC) — mapeia `--model` (LM GGUF), `--codec-model` (codec GGUF), `--voice` (WAV ref), `--ref-text`, `--instruct` → VoiceDesign
- **Build:** `add_library(omnivoice STATIC ...)` com `POSITION_INDEPENDENT_CODE`, `OMNIVOICE_STATIC`, links `ggml ggml-base ggml-cpu`, `-O3`; linkado em `crispasr-lib`, `crispasr-diff` e `${TARGET}` (CLI)

## Kokoro "Full GGUF" (working tree, não commitado)

Implementa a hipótese de migração do `kokoro-full-gguf-analysis.md`:

- **Nova ABI (`kokoro.h` +7):** `kokoro_embedded_voice_count`, `kokoro_embedded_voice_id`, `kokoro_select_embedded_voice`
- **Novos internals (`kokoro.cpp` +461):** structs `kokoro_embedded_voices`, `kokoro_alias_copies`; funções `init_embedded_voices`, `map_full_gguf_tensor_name`, `add_full_gguf_tensor_aliases`, `tensor_to_f32`, `is_full_gguf_conv_alias`
- **Semântica:** vozes embutidas `voices.<id>.style` (shape `(256,510)`) convivem com o path legado `voice.pack` (`(256,1,510)`); conversão BF16/F16/quant → F32 para views por byte offset
- **CLI (`crispasr_backend_kokoro.cpp` +56):** nova ordem de resolução de voz — `--voice` path existente → pack separado; `--voice` não-path → id embutido; sem `--voice` + embutidas → default embutida; fallbacks antigos → pack separado
- **Teste (`test-kokoro-params.cpp` +6):** null-guards da nova ABI

## Sinais de risco / observação (fatos e inferências)

| # | Sinal | Tipo | Evidência |
|---|---|---|---|
| D1 | **Debug scaffolding em `core/`** — shims `lstm_debug_mul_mat` / `kokoro_debug_mul_mat` / `kokoro_debug_conv_1d` gateados por `KOKORO_DEBUG_SHAPES` (env var) | Fato | `src/core/lstm.h:45-67`, `kokoro.cpp` diff |
| D2 | `core/` era registrado (25/06) como "módulo mais limpo, zero TODOs"; o delta introduz paths de debug env-gated | Fato + inferência | `verification.md` claim #24; diff `core/lstm.h` |
| D3 | Trilha de commits recentes itera o orçamento de nós do grafo do vocoder MI-LSTM (32K→65K→131K) — debugging ativo | Fato | `git log` (6b50f76e, f3aba41c, 9877c883) |
| D4 | `kokoro-full-gguf-analysis.md` (hoje 21:28) declara "cria plano/documentação, **não aplica patch no runtime**"; mas a working tree **contém** a implementação embedded-voice | **Inferência — requer confirmação de ordenação** | Análise §Riscos; `git diff src/kokoro.cpp` |
| D5 | OmniVoice: 0 TODO/FIXME em ~13K LOC, ABI versionada, headers densamente documentados — alinhado ao padrão de qualidade do `core/` | Fato | `grep -rEc "TODO\|FIXME\|HACK" src/omnivoice/` = 0 |
| D6 | Novo modelo exposto na factory; nenhum teste de smoke/fixture para OmniVoice observado nesta passagem | Fato (ausência) | nenhum `test-omnivoice*` encontrado no escopo do delta |
| D7 | Bus factor e cobertura geral de testes do repo — não reavaliados neste delta | Gap | fora do escopo da re-observação |

## Pontos que exigem decisão (não resolvidos pela observação)

- **D2/D3:** a instrumentação de debug em `core/lstm.h` é intencional/permanente ou scaffolding temporário do debug do vocoder? (Decisão de remoção vs. retenção.)
- **D4:** a implementação Kokoro Full GGUF está completa e pronta para commit, ou é trabalho em andamento? A análise diz "não patche"; o código diz "já patcheado". Precisa de confirmação do mantenedor antes de tratar como pronto.
- **D6:** OmniVoice precisa de teste de smoke com GGUF real (análogo ao recomendado para Kokoro Full) antes de tratar como integrado.

## Notas metodológicas

- Esta passagem **não re-escaneou** o repo inteiro (já mapeado em 25/06). Mapeou apenas o delta visível em `git status` / `git diff`.
- Não editou código (regra de observação before action).
- Não sobrescreveu `repository.json` / `modules.json` (regra non-negotiable: preservar artefatos preexistentes). Criou este artefato delta novo.
