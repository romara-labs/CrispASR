# ADR 0001 — OmniVoice: exceções C++ internas capturadas na fronteira da ABI C

- **Status:** Aceito
- **Data:** 2026-06-28
- **Modo Kiyotaka:** project_evolution → PATCH_PLAN (TK-0009)
- **Decisor:** mantenedor (Rodrigo)

## Contexto

O subsistema **OmniVoice** (`src/omnivoice/`, ~13,2K LOC, 37 arquivos) foi importado como um
runtime GGML isolado a partir de um projeto externo (`omnivoice.cpp-master`). Seu caminho de
carga (leitor GGUF, carga do codec de áudio, carga dos pesos do LM) tem da ordem de ~97 sítios
de chamada que podem falhar; propagar um `bool` de erro por todos eles exigiria uma cascata
de tratamento de erro em cada nível.

Para manter esse caminho de carga legível, o OmniVoice adota internamente **exceções C++**
(`ov_throw(...)` em `src/omnivoice/ov-error.h`, que monta uma mensagem e dispara
`std::runtime_error`).

Isto **diverge** da convenção do restante do CrispASR, registrada pela verificação de 25/06
(claim #4, `_kiyotaka_project/verification.md`) como **"assert-abort + return codes, zero
exceções"**. Esta divergência é intencional e este ADR a documenta.

## Decisão

Manter o uso de exceções **confinado ao subsistema OmniVoice**, com a fronteira `extern "C"`
como garantia:

1. `ov_throw` (e qualquer `std::runtime_error` / `std::bad_alloc` oriundo do backend GGML
   profundo) é capturado **somente** nas entradas públicas da ABI C.
2. Cada entrada pública que pode falhar envolve o caminho de carga/síntese em `try { ... }
   catch (const std::exception&)` (e `catch (const std::bad_alloc&)` onde há alocação),
   convertendo a falha em:
   - um código negativo de `enum ov_status` (retorno), e
   - uma mensagem gravada no `ov_last_error()` thread-local via `ov_set_error`.
3. **Nenhuma exceção cruza a fronteira `extern "C".`** A ABI pública permanece C pura
   (consumível de C, Rust bindgen, Go cgo, Python ctypes).

## Consequências

- **Positivas:** caminho de carga com tratamento de erro limpo (sem cascata de `bool`);
  diagnóstico de erro estruturado (`ov_status` + `ov_last_error`) para os bindings;
  robustez a falhas profundas (ex.: `std::bad_alloc` no backend GGML) sem abortar o processo.
- **Custo / divergência:** o subsistema OmniVoice usa um idiom diferente do resto do repo.
  Isto é aceitável porque o subsistema é **isolado** (runtime importado, com seus próprios
  `audio-io`/`wav`/`resample`/`weight-ctx`) e a fronteira catch é a garantia de contenção.
- **Regra operacional:** **não propagar** este padrão de exceções para fora de `src/omnivoice/`.
  Código novo em outros módulos deve continuar com a convenção "return codes, zero exceções".

## Evidência (auditoria da fronteira — TK-0010)

Toda entrada `extern "C"` pública de `src/omnivoice/omnivoice.cpp` que pode disparar uma
exceção está envolta em `try/catch`:

| Entrada | try/catch | Converter para |
|---|---|---|
| `ov_init` | `omnivoice.cpp:240-269` | `NULL` + `ov_set_error` |
| `ov_synthesize` | `omnivoice.cpp:323-332` | `OV_STATUS_GENERATE_FAILED` |
| `ov_extract_voice_ref` | `omnivoice.cpp:385-433` (catch `bad_alloc` + `exception`) | `OV_STATUS_*` |

As demais entradas (`ov_version`, `ov_last_error`, `ov_log_set`, `ov_audio_free`, `ov_free`,
`ov_voice_ref_free`, `ov_init_default_params`, `ov_tts_default_params`,
`ov_duration_sec_to_tokens`, `ov_num_codebooks`) não percorrem caminho de carga e não
disparam exceções. **Fronteira confirmada como segura: nenhuma exceção escapa da ABI C.**

Referências de código: `src/omnivoice/ov-error.h:14-20` (racional do `ov_throw`),
`src/omnivoice/omnivoice.cpp:240-269`, `323-332`, `385-433`.

## Referências
- `_kiyotaka_project/excavation-delta-2026-06-28.md` (R1 — regra implícita)
- `_kiyotaka_project/verification.md` (claim #4: repo "zero exceções")
