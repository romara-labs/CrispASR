# TK-0004: Integrar CLI com `--voice` path ou id

## Objetivo

Fazer o backend Kokoro do CLI aceitar GGUF full sem exigir voice pack separado.

## Acceptance Criteria

- `--voice` como arquivo usa `kokoro_load_voice_pack`.
- `--voice` como id usa voz embutida quando o modelo full fornece esse id.
- Sem `--voice`, modelo full usa default deterministico.
- Modelo antigo continua exigindo ou resolvendo voice pack separado.

## Required Tests

- Smoke com GGUF full.
- Smoke/regressao com modelo antigo + `kokoro-voice-af_heart.gguf`.

## Affected Files

- `examples/cli/crispasr_backend_kokoro.cpp`
- `docs/cli.md`
- `docs/tts.md`

