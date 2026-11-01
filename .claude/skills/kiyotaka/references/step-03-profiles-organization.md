# Mode Output Organization

## Overview

Kiyotaka no longer asks for ad-hoc profile organization during first run. Output layout is determined by the selected mode.

## Output Folders

- `developer_reverse` → `_kiyotaka_profiles/`
- `atomic_plan` → `_kiyotaka_plan/`
- `security_analysis` → `_kiyotaka_security/`
- `project_evolution` → `_kiyotaka_project/`
- `plan_interrogation` → `_kiyotaka_interrogation/`
- `legacy_cleanroom` → `_kiyotaka_cleanroom/`

## Rule

When a mode is active, all primary deliverables for that mode belong in its dedicated output folder. Shared runtime data belongs in `.kiyotaka/context/`, `.kiyotaka/cache/` or `.kiyotaka/_config/`.
