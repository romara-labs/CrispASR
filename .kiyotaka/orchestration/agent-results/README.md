# Agent Results

This directory is a **staging area** for output artifacts produced by dynamic agents.

## How it works

- Dynamic agents created via `kiyotaka-shadow-orchestrate` may write their output files here.
- Each agent's contract defines its expected outputs and evidence requirements.
- Results placed here are **not** automatically consumed by the framework — they serve as the agent's deliverable for review by the orchestrator or developer.

## Current status

Automatic result writing by dynamic agents is **not yet implemented**. This directory exists as a reserved location for future agent executor integration.

When an agent completes its work, its outputs should be placed in this directory or in the location specified by the agent's contract `outputs` field.

## What belongs here

- Analysis reports (`*-report.md`)
- Coverage matrices (`*-matrix.md`)
- Feasibility assessments (`*-feasibility.md`)
- Traceability documents (`*-trace.md`)
- Any artifact defined in a dynamic agent's contract `outputs`

## What does NOT belong here

- Agent contracts → `.kiyotaka/orchestration/agent-contracts/`
- Agent state → `.kiyotaka/dynamic-agents/active/` or `archived/`
- Orchestration documents → `.kiyotaka/orchestration/` (roster, delegation map, shadow plan)
