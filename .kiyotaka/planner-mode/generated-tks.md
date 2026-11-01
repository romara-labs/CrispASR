# TKs Gerados: Kokoro Full GGUF

## TK-0001: Validar contrato dos GGUFs full reais

Status: COMPLETED nesta passagem.

Aceitacao:
- Dump dos metadados dos tres GGUFs full BF16/F16/F32 registrado.
- Confirmacao de `kokoro.precision`, `kokoro.voices.ids`, `kokoro.vocab.*` e shapes de `voices.<id>.style`.
- Diferencas entre loader externo e arquivos reais documentadas.

Testes:
- Comando de inspecao GGUF executado para cada arquivo disponivel em `/home/rodrigo/Documentos/gguf-onnx-migrations/voicer.cpp/convert-gguf`.

Blast radius:
- Documentacao e decisao de contrato.

## TK-0002: Adicionar loader de vozes embutidas no contexto Kokoro

Status: COMPLETED nesta passagem.

Aceitacao:
- `kokoro_context` indexa vozes embutidas do GGUF principal.
- F32/BF16/F16 sao convertidos para representacao F32 usada pelo grafo.
- Runtime antigo sem vozes embutidas continua funcionando.

Testes:
- `cmake --build build --target crispasr-cli`.
- Smoke CLI com `Kokoro-82M-v1.0-F32.full.gguf` sem `--voice`.
- Smoke CLI com `Kokoro-82M-v1.0-F16.full.gguf` e `--voice af_heart`.

Blast radius:
- `src/kokoro.cpp`, `src/kokoro.h`.

## TK-0003: Selecionar voz embutida por ABI e fallback

Status: COMPLETED nesta passagem.

Aceitacao:
- Nova ABI lista vozes embutidas ou retorna contagem/ids.
- Nova ABI seleciona voz por id.
- Sem selecao explicita, runtime usa default deterministico quando o modelo full tem vozes.
- `kokoro_load_voice_pack` continua funcionando.

Testes:
- Smoke CLI sem `--voice` validou default deterministico.
- Smoke CLI com `--voice af_heart` validou selecao por id.
- Smoke CLI com `--voice no_such_voice` validou erro especifico.

Blast radius:
- ABI C de Kokoro e wrappers futuros.

## TK-0004: Integrar CLI com `--voice` path ou id

Status: COMPLETED nesta passagem.

Aceitacao:
- `--voice /path/file.gguf` usa o fluxo antigo.
- `--voice af_heart` seleciona voz embutida quando disponivel.
- Sem `--voice`, GGUF full seleciona default sem exigir arquivo separado.
- Mensagens de erro diferenciam "voice id inexistente" e "path de voz falhou".

Testes:
- Smoke CLI com GGUF full F32 e F16.
- `cmake --build build --target crispasr-cli`.

Blast radius:
- `examples/cli/crispasr_backend_kokoro.cpp`, docs CLI/TTS.

## TK-0005: Atualizar conversor/documentacao do formato full

Status: PENDING parcial; documentacao Kiyotaka criada, documentacao publica/conversor ainda pendentes.

Aceitacao:
- Contrato do GGUF full documentado em `docs/tts.md` ou doc dedicada.
- Conversor atual declara claramente se gera formato separado ou full.
- Se o conversor full for importado, ele escreve metadados exigidos pelo loader.

Testes:
- Validacao de metadata do GGUF gerado.

Blast radius:
- `models/convert-kokoro-to-gguf.py`, docs.

## Nota de regressao 2026-06-29: ruido no Kokoro full-GGUF

Status: COMPLETED nesta passagem.

Achado:
- A validacao anterior por "nao-silencio" era insuficiente. O caminho full-GGUF gerava audio com perfil de ruido para `pf_dora`/`pt-br`.
- Causa corrigida em `src/kokoro.cpp`: `kokoro_make_har` lia `dec.gen.m_source.weight` como F16 fixo, mas o GGUF full F32 armazena esse tensor como F32. A leitura agora usa `tensor_to_f32`.

Evidencia:
- Antes: `/tmp/crispasr-kokoro-pf-dora-curto.wav` `peak=1196 rms=300.4 zcr=0.4002`.
- Depois: `/tmp/crispasr-kokoro-pf-dora-curto-fixed.wav` `peak=15218 rms=1658.5 zcr=0.131`.
- Referencia `voicer-cli`: `/tmp/voicer-kokoro-pf-dora-curto.wav` `peak=13749 rms=1833.9 zcr=0.0866`.
- Arquivo solicitado regenerado: `/tmp/kokoro-pf-dora-pt-br.wav` `duration=4.62s peak=12581 rms=1449.0 zcr=0.1048`, tocado com `aplay`.

## Nota de performance 2026-06-29: Kokoro full-GGUF realtime CPU

Status: COMPLETED nesta passagem.

Achado:
- O caminho de audio recalculava o generator duas vezes (`mag` e `phase`) e recalculava BERT/predictor/F0N dentro de cada chamada.
- O caminho de sintese final agora executa `mag`/`phase` no mesmo grafo, extrai `dur_enc_out`/`durations` numa unica passagem do predictor, extrai `f0_curve`/`n_curve` numa unica passagem F0N e reutiliza o F0 calculado pelo decoder para o harmonico do generator.

Evidencia:
- Antes da otimizacao: `/tmp/kokoro-bench-cpu.wav` levou ~10.44s para 4.62s de audio, RTF ~2.26.
- Apos unir `mag`/`phase`: `/tmp/kokoro-bench-cpu-opt1.wav` levou ~5.55s, RTF ~1.20.
- Apos reuso predictor/F0N/F0: `/tmp/kokoro-bench-cpu-opt2.wav` levou ~4.48s com bench, RTF ~0.97.
- Sem bench: `/tmp/kokoro-pf-dora-pt-br.wav` gerado em ~4.26s para 4.62s de audio, RTF ~0.92.
