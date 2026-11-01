# State Schema — `.kiyotaka/state.json`

```json
{
  "framework": "kiyotaka",
  "version": "1.0.0",
  "selected_mode": "developer_reverse",
  "installed_modes": ["developer_reverse"],
  "installed_agents": [],
  "installed_engines": [],
  "optional_capabilities": [],
  "output": {
    "profiles": "_kiyotaka_profiles",
    "plan": "_kiyotaka_plan",
    "security": "_kiyotaka_security",
    "project": "_kiyotaka_project",
    "interrogation": "_kiyotaka_interrogation",
    "cleanroom": "_kiyotaka_cleanroom",
    "replication": "_kiyotaka_replication",
    "active": "_kiyotaka_profiles"
  },
  "output_folders": [],
  "pending": [],
  "current_phase": "observation",
  "developer_profile": null,
  "domain_model": null,
  "security_model": null,
  "project_model": null,
  "legacy_cleanroom_model": null,
  "cleanroom": {
    "target_project": null,
    "discovery_status": "not_started",
    "validation_status": "not_started",
    "contamination_status": "clean",
    "team_b_handoff_status": "not_ready",
    "parity_status": "not_started",
    "critical_gaps": [],
    "approved_specs": [],
    "blocked_specs": [],
    "current_phase_index": 0,
    "current_phase_key": "isolation",
    "current_phase_label": "Prepare isolation",
    "overall_status": "DISCOVERY_IN_PROGRESS",
    "phase_statuses": {},
    "completed_phases": 0
  },
  "patterns": {},
  "decisions": [],
  "risks": [],
  "last_run": null
}
```

## Rules

- `selected_mode` drives the active pipeline.
- `output.active` must match the active mode output folder.
- `installed_modes`, `installed_agents` and `installed_engines` are append-only unless the user explicitly removes support.
- `pending`, `current_phase` and `last_run` reflect runtime progress, not installation intent.
- `cleanroom` tracks legacy discovery, validation, contamination, parity and Team B handoff state.
