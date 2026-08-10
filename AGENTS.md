# Repository Instructions

- When asked to check a GitHub issue, read the whole issue before acting: the description, all comments, linked comments, and recent owner/reporter follow-ups. Treat issue comments as part of the requirements, not optional context.

## Read first, if you have it

Maintainer checkouts keep a development guide **beside** this repository, at
`../crispasr-crispembed-dev.md` — a sibling of the repo root, not a file inside
it, and deliberately not tracked here (it spans CrispASR *and* CrispEmbed).

**If that file exists, read it in full before writing any code.** It is the
method, and it is not summarised anywhere: the hard rules, the
convert → quantize → dump-reference → diff → parity port pipeline, the ggml and
GPU-portability gotchas, the A/B discipline, and the storage layout. Where it
and this file disagree, it wins.

If it is not there you are on an ordinary clone — nothing is missing from the
build or the tests. Use the map below, `README.md` and `docs/`.

## Where things live

Read this before opening any of them: together they are >2 MB, and only the
development guide above is meant to be read end to end.

| Need | Read |
|---|---|
| A specific past lesson ("has this bitten us?") | `docs/LEARNINGS-INDEX.md` first — 272 lessons by topic and by model — then grep the heading in `LEARNINGS.md` |
| Is this backend fast / which quant ships | `PERFORMANCE.md` |
| What is in flight right now | `PLAN.md` — and claim your task there before starting |
| What already shipped | `HISTORY.md` — archive; consult to confirm a claim, never as a plan |
| Adding a backend | `docs/contributing.md` (12-point checklist) |
| Build, test, lint commands | `README.md`, and the development guide above |

Two habits that repeatedly cost time here:

- **`PLAN.md` and `HISTORY.md` prose goes stale.** Items marked OPEN are often
  already shipped. Audit against the CODE, never the note.
- **Auto-detection working in the CLI proves nothing about the bindings.** The
  CLI has a filename pass that short-circuits the GGUF-architecture table the C
  ABI depends on; test through the C ABI. (Issue #335.)

## Regenerated files — do not hand-edit

- `docs/LEARNINGS-INDEX.md` → `python tools/gen-learnings-index.py` (CI gates it
  with `--check`; adding a `##` section to `LEARNINGS.md` shifts every line
  number it cites).
- `bindings/go/whisper.go` cgo `LDFLAGS` → `python tools/sync_go_cgo_ldflags.py`.

---

# Kiyotaka

> The White Room Framework installed in this project.

## How to use

Type `kiyotaka` to activate Kiyotaka and begin or resume the selected operational mode.

## Behavior when activated

When the user types `kiyotaka`:

1. Read `.kiyotaka/config.toml`
2. Read `.kiyotaka/state.json`
3. Identify `selected_mode`
4. Activate the `kiyotaka` skill available in `.agents/skills/kiyotaka/SKILL.md`
5. Read the SKILL.md in full and follow Kiyotaka's instructions exactly

## Kiyotaka Commands

### Planner Mode — `kiyotaka planner-mode`

Convert a plan into tracked BK/TK tasks before execution.

When the user invokes planner mode (by saying "kiyotaka planner-mode", "planner mode", or passing a plan):

1. Read `.kiyotaka/state.json` and `.kiyotaka/config.toml`
2. Receive the plan from user input (text, file path, or existing plan artifact)
3. Save the plan to `.kiyotaka/planner-mode/current-plan.md`
4. Classify the plan: NEW_PLAN, CONTINUATION_PLAN, PATCH_PLAN, RECOVERY_PLAN, MIGRATION_PLAN, CLEANROOM_HANDOFF_PLAN, SECURITY_REMEDIATION_PLAN
5. Analyze: extract phases, tasks, dependencies, risks, blockers, affected files
6. Generate tracked TK tasks with IDs (TK-XXXX), acceptance criteria, required tests, blast radius, affected files
7. Write `.kiyotaka/planner-mode/generated-tks.md` and individual task files in `generated-tasks/`
8. Generate `.kiyotaka/planner-mode/backlog-patch.md` and apply to backlog if appropriate
9. Generate `.kiyotaka/planner-mode/execution-queue.md` with dependency order
10. Update `.kiyotaka/state.json` planner_mode section
11. Report: READY_TO_EXECUTE or BLOCKED with reason

**Rule:** Never execute plan tasks without tracked TK/BK tasks.

### Shadow Orchestration — `kiyotaka shadow-orchestrate`

Create and manage temporary dynamic agents for scoped parallel work.

When the user invokes shadow orchestration:

1. Read `.kiyotaka/state.json` and `.kiyotaka/dynamic-agents/registry.json`
2. Verify a base plan exists (populated `.kiyotaka/plan.md` or mode output)
3. If no base plan: BLOCK and ask user to run a mode first
4. Create agent contract: trigger, scope, non_scope, inputs, outputs, evidence_required, stop_conditions, failure_conditions, parent_task, disposal
5. Register in `.kiyotaka/dynamic-agents/active/<agent-id>.json` and update registry
6. Write contract to `.kiyotaka/orchestration/agent-contracts/<agent-id>.md`
7. Regenerate: agent-roster.md, delegation-map.md, shadow-plan.md
8. Update state.json shadow_orchestration counts

Sub-commands:
- **spawn / create** — create a new dynamic agent (default)
- **list** — show all registered agents
- **status** — show shadow orchestration metrics
- **archive <agent-id>** — move agent to archived

### Aliases

- `kiyotaka spawn-agent` = `kiyotaka shadow-orchestrate`
- `kiyotaka delegate` = `kiyotaka shadow-orchestrate`
- `kiyotaka synthesize-agents` = `kiyotaka shadow-orchestrate`

## Non-negotiable rule

Never delete, modify, or overwrite pre-existing project files.
Observe before planning. Plan before editing. Test before declaring done.
