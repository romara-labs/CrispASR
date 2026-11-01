# Romara Labs upstream merge — 2026-10-31

Target branch: `kokoro-omnivoice-realtime`.

- Previous Romara head: `a8efefec1`.
- Integrated upstream: `CrispStrobe/CrispASR` main at `1dd0bf6fb`.
- Local recovery branch: `backup/kokoro-omnivoice-pre-crispstrobe-20261031`.
- Integration uses a merge commit; both histories remain intact.

## Preserved fork behavior

The complete `src/omnivoice/` runtime remains the production implementation,
including the two-GGUF pipeline, VoiceDesign, cancellation, streaming callbacks,
language aliases, reusable voice references and explicit duration controls.
The CLI adapter and C ABI codec/reference setters remain wired to that runtime.
Kokoro embedded voices, full-GGUF support, LSTM diagnostics and per-request G2P
language/reset behavior remain present. Realtime server and turn buffering remain
available. Existing local, untracked configuration directories were not staged.

An audit against the pre-merge backup found 39 fork-specific files byte-identical
and 10 combined with upstream changes; no fork-specific file was missing.

## Conflict resolution

- Combine upstream repository guidance with the existing Kiyotaka instructions.
- Retain the complete OmniVoice README description alongside upstream backend
  additions, model licensing metadata and generated documentation counts.
- Keep the compatibility header and add upstream's audio-processing setter
  without changing the by-value context-parameter ABI.
- Port the audio-processing setter/environment options to the active fork facade:
  reference preprocessing policy, silence removal, reference loudness restoration,
  unconditioned peak normalization and independently configurable fade/padding.
- Retain upstream `src/omnivoice.cpp` exclusively for its direct-include reference
  tests and diagnostic tools. Production CMake continues to compile only the
  complete runtime under `src/omnivoice/`. Those reference tests do not qualify
  the fork codec's real-model numerical parity.
- Add `test-omnivoice-facade` to exercise the active facade's audio policy,
  invalid values, independent fade/padding, reference gain, silent output and
  environment-duration reset without downloading a model.
- Update the ggml checkout to the merged gitlink `e2ae679a5`.

## Validation

- Release CPU CLI and shared-library build: passed, version 0.8.49.
- 146 focused CTests: all passed. Targets cover Kokoro parameters/G2P/wiring,
  OmniVoice language/instructions/duration/prompt/facade, upstream reference codec
  tests, realtime turn buffering, architecture dispatch and session C ABI guards.
- Real server realtime test with cached Whisper tiny: 9 passed, 0 failed;
  includes RFC 6455 handshake/ping, transcription, timing and turn reset.
- Backend wiring with the built CLI and required shared C ABI library: passed.
- Go cgo link flags: passed against an independently configured static graph;
  the complete wiring check also passed with dependency access enabled.
- Ruby generated context guards: passed (157 callable operations).
- LEARNINGS index: passed (338 sections).
- Documentation counts checked against the committed feature matrix: passed
  (143 backends). The CPU build omits optional ONNX adapters.
- Upstream allocator regression: normal output and quiet-reference gain passed;
  allocator-failure mutations and the pre-fix baseline failed as expected.

The existing build cache had `BUILD_TESTING=OFF`; it was enabled for actual CTest
discovery and execution. No GPU execution or real-model Kokoro/OmniVoice synthesis
was qualified in this merge. Full cross-platform/model acceptance remains CI work.
