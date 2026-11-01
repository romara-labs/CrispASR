# Checkpoint Guide

## Overview

Checkpoints are optional runtime snapshots that help resume a mode safely after interruption or repository drift.

## Location

`.kiyotaka/_config/checkpoint-<mode>-<phase>.json`

## Example

```json
{
  "mode": "plan_interrogation",
  "phase": "source-trace",
  "completed_at": "2026-06-06T12:00:00Z",
  "files_generated": [
    "_kiyotaka_interrogation/source-trace.md"
  ],
  "summary": "Traced 14 claims, 4 still blocked",
  "stats": {
    "claims_traced": 14,
    "blocked_claims": 4
  }
}
```

## Guidance

- Use checkpoints for resuming analysis, not for hiding unresolved contradictions.
- A checkpoint can record blocked status; it does not imply readiness.
- For `plan_interrogation`, checkpoint counts should align with `implementation-readiness.md`.
