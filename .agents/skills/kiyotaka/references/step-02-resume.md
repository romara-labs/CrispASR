# Resume Procedure

## Overview

When `.kiyotaka/state.json` exists, resume according to `selected_mode`, `current_phase` and the artifacts already generated.

## Steps

1. Read `state.json`.
2. Read the active output folder.
3. Run the regression check if there are checkpoints or timestamps to compare.
4. Resume from `current_phase`.
5. Only activate agents that belong to the current mode.
6. Update `last_run` and any mode-specific status artifacts after each meaningful step.

## Mode Routing

| Mode | Typical next agents |
|------|---------------------|
| `developer_reverse` | observer, excavator, architect-reader, profiler, pattern-reader, emotion-reader, forecaster, verifier |
| `atomic_plan` | observer, domain-modeler, contract-designer, atomic-planner, task-slicer, test-strategist, adr-writer, implementation-sequencer |
| `security_analysis` | observer, threat-modeler, data-flow-mapper, boundary-auditor, auth-auditor, secrets-auditor, security-test-designer, remediation-planner |
| `project_evolution` | observer, excavator, project-doctor, clean-architect, clean-code-planner, documentation-builder, continuation-planner, regression-guardian |
| `plan_interrogation` | observer, interrogator, assumption-hunter, source-tracer, coverage-mapper, hallucination-detector, blocker-judge, plan-corrector |
| `legacy_cleanroom` | observer, excavator, legacy-discovery, inventory-cartographer, behavior-cartographer, domain-rule-extractor, architecture-reconstructor, operational-spec-writer, cleanroom-validator, contamination-guard, parity-test-designer, handoff-builder, migration-strategist |
