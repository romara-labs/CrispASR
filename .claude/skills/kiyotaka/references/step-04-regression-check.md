# Regression Check

## Overview

Before resuming a mode, confirm the repository and generated artifacts have not drifted enough to invalidate prior analysis.

## Procedure

1. Compare relevant source files against the latest checkpoint or timestamp.
2. Compare mode artifacts in the active output folder for partial rewrites or missing files.
3. If source drift is major, restart from observation for the affected mode.
4. If only generated artifacts drifted, regenerate or revalidate them before continuing.

## Decision Matrix

| Drift | Action |
|------|--------|
| Minimal | Continue and note the delta |
| Moderate | Re-run affected agents |
| High | Restart the mode from observation |

## Special Rule for `plan_interrogation`

If the original plan or the original code changed after the audit started, mark readiness as stale and re-run source trace and coverage before allowing finalize.
