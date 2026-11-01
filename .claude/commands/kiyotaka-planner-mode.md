You are operating in Kiyotaka Planner Mode.

Your job is to receive a plan and convert it into tracked BK/TK tasks before any execution is allowed.

## Step 1 — Read project state

Read these files to understand the current project:

- `.kiyotaka/state.json` — active mode, installed agents, planner_mode state
- `.kiyotaka/config.toml` — planner_mode configuration and task system

## Step 2 — Receive the plan

The user input is: $ARGUMENTS

Accept the plan from:
- Inline text passed after the command
- A file path (read the file contents)
- An existing plan artifact (e.g. `_kiyotaka_plan/backlog.md`, `_kiyotaka_interrogation/corrected-plan.md`)

If no plan content is provided, ask the user to provide one.

## Step 3 — Save the incoming plan

Write the raw plan to `.kiyotaka/planner-mode/current-plan.md` using this format:

```markdown
# Current Incoming Plan

## Source
User provided via /kiyotaka-planner-mode.

## Received At
{{current_timestamp}}

## Raw Plan
{{raw_plan_content}}
```

Also save a timestamped copy in `.kiyotaka/planner-mode/incoming-plans/`.

## Step 4 — Classify the plan

Classify the plan into one of:
- `NEW_PLAN` — a completely new plan
- `CONTINUATION_PLAN` — continuing an existing plan
- `PATCH_PLAN` — a fix or hotfix plan
- `RECOVERY_PLAN` — a rollback or incident recovery plan
- `MIGRATION_PLAN` — a migration or port plan
- `CLEANROOM_HANDOFF_PLAN` — a cleanroom team-B handoff
- `SECURITY_REMEDIATION_PLAN` — a security vulnerability remediation

## Step 5 — Analyze the plan

Extract from the plan:
- **Phases** — major sections or milestones
- **Tasks** — individual work items
- **Dependencies** — what must happen before what
- **Risks** — potential problems
- **Blockers** — things that prevent progress
- **Affected files** — source files that will be touched
- **Decisions pending** — choices the user needs to make

Write the analysis to `.kiyotaka/planner-mode/plan-analysis.md`.

## Step 6 — Generate tracked tasks

Determine the task system from config (TK by default, BK if configured).

For each extracted task, generate a tracked task with:
- **ID** — sequential TK-XXXX numbering (check `.kiyotaka/planner-mode/` for existing tasks)
- **Title** — clear imperative description
- **Classification** — from step 4
- **Description** — what needs to be done
- **Dependencies** — IDs of prerequisite tasks
- **Blockers** — any blocking issues
- **Blast Radius** — Small, Medium, or Large
- **Acceptance Criteria** — measurable completion conditions
- **Required Tests** — Unit, Integration, Regression, Security (if applicable)
- **Affected Files** — files to modify
- **Status** — Pending

Write tasks to `.kiyotaka/planner-mode/generated-tks.md` and individual files in `.kiyotaka/planner-mode/generated-tasks/`.

## Step 7 — Generate backlog patch

Create `.kiyotaka/planner-mode/backlog-patch.md` containing:
- Tasks to add to the project backlog
- Reason for the patch
- Confirmation checklist

If the active mode has an output folder with a `backlog.md`, append the tasks there.

## Step 8 — Generate execution queue

Create `.kiyotaka/planner-mode/execution-queue.md` with:
- Ordered task list respecting dependencies
- Identified blockers
- Next safe task to execute

## Step 9 — Update state

Update `.kiyotaka/state.json` → `planner_mode` section:
- `last_plan` — path to current-plan.md
- `last_plan_path` — same
- `generated_tasks` — array of task IDs
- `execution_queue` — same order as execution queue
- `requires_backlog_update` — true
- `requires_progress_update` — true
- `last_run` — ISO timestamp

Update progress: append an entry to the active mode's `progress.txt` (and `docs/progress.txt` if it exists).

## Step 10 — Report status

Report the final state:
- If tasks were generated and at least one is unblocked: **READY_TO_EXECUTE** — show the next safe task
- If no tasks could be generated: **BLOCKED_NO_TASKS** — explain why
- If all tasks are blocked by decisions: **BLOCKED_BY_DECISION** — list pending decisions
- If evidence is missing: **BLOCKED_BY_MISSING_EVIDENCE** — list what is missing

## Non-negotiable rules

- Never execute plan tasks without tracked TK/BK tasks
- Never skip the analysis phase
- Every task must have acceptance criteria
- Every task must have required tests listed
- If the plan is too vague to extract tasks, say so and ask for clarification
