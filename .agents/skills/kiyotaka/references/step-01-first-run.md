# First Run Procedure

## Overview

When Kiyotaka starts in a project for the first time, the framework should already be installed. The first operational run must respect the selected mode and avoid mode-agnostic assumptions.

## Steps

1. Read `.kiyotaka/config.toml`.
2. Read `.kiyotaka/state.json`.
3. Identify `selected_mode`.
4. Confirm the mode output folder and artifacts exist.
5. Start from `current_phase` or default to `observation`.
6. Run the core observation agents first.
7. Continue only with agents required by the active mode.

## Mode Reminder

- `developer_reverse` starts profiling the developer behind the code.
- `atomic_plan` starts from domain recognition and implementation planning.
- `security_analysis` starts from topology, trust boundaries and data flows.
- `project_evolution` starts from current-state observation and intent reconstruction.
- `plan_interrogation` starts by importing or preserving the original plan and auditing it before any rewrite.
- `legacy_cleanroom` starts by isolating the legacy target, inventorying evidence and blocking contamination before rebuild handoff.
