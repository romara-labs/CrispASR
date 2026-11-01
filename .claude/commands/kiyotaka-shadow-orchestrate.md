You are operating in Kiyotaka Shadow Orchestration mode.

Your job is to create, manage, or inspect temporary dynamic agents that work on scoped tasks in parallel with the main workflow.

## Step 1 — Read project state

Read these files to understand the current orchestration state:

- `.kiyotaka/state.json` — shadow_orchestration status, agent counts
- `.kiyotaka/dynamic-agents/registry.json` — active and archived agents
- `.kiyotaka/orchestration/agent-roster.md` — current agent roster
- `.kiyotaka/orchestration/delegation-map.md` — task delegation overview

## Step 2 — Parse the user input

User input: $ARGUMENTS

Detect sub-commands:
- **No args / `create`** — create a new dynamic agent (default)
- **`list`** — show all registered agents
- **`status`** — show shadow orchestration state
- **`archive <agent-id>`** — archive a completed agent

## Step 3 — For `create`: verify prerequisites

Shadow orchestration requires a **base plan** to exist. Check for populated plan files:
- `.kiyotaka/plan.md`
- Any mode output folder with a populated `architecture.md`, `bootstrap.md`, or `plan.md`

If no base plan exists, **BLOCK** with:
```
BLOCKED — Shadow orchestration requires a base plan.
Run a Kiyotaka mode first to generate plan artifacts, or populate .kiyotaka/plan.md.
```

## Step 4 — Create agent contract

If creating an agent, define the contract:

```markdown
## Agent Contract

- **agent_id:** auto-generated from trigger + scope (slugified)
- **trigger:** What initiated this agent (from user input or --trigger flag)
- **scope:** What the agent should analyze or produce
- **non_scope:** What is explicitly out of scope
- **inputs:** What the agent receives (plan files, source code, etc.)
- **outputs:** What the agent must produce
- **evidence_required:** What counts as proof of completion
- **stop_conditions:** When the agent should stop
- **failure_conditions:** When the agent should report failure
- **parent_task:** TK-XXXX linking to the main task
- **disposal:** archive (default)
```

## Step 5 — Register the agent

Write the agent JSON to `.kiyotaka/dynamic-agents/active/<agent-id>.json`.
Update `.kiyotaka/dynamic-agents/registry.json` with the new agent.
Write the contract markdown to `.kiyotaka/orchestration/agent-contracts/<agent-id>.md`.
Regenerate:
- `.kiyotaka/orchestration/agent-roster.md`
- `.kiyotaka/orchestration/delegation-map.md`
- `.kiyotaka/orchestration/shadow-plan.md`

Update `.kiyotaka/state.json` → `global_skills.shadow_orchestration`:
- Increment `agents_created` and `agents_active`
- Set `last_agent_created` to the agent ID

## Step 6 — Report

Show the created agent details:
- Agent ID
- Trigger, scope, parent task, disposal
- File locations

## For `list`: display all agents with status (active/archived).

## For `status`: show orchestration metrics (created, active, archived, last created).

## For `archive`:
1. Move agent JSON from `active/` to `archived/`
2. Update registry status to `archived`
3. Regenerate roster, delegation map, shadow plan
4. Decrement `agents_active`, increment `agents_archived` in state

## Rules

- Maximum 10 active agents (configurable in config.toml)
- Each agent must have a clear scope and stop conditions
- Agents without a parent task link should warn the user
- Never create agents without a populated base plan
