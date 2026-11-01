# Surface Schema — `.kiyotaka/context/surface.json`

```json
{
  "project": {
    "name": "string",
    "type": "web-app | cli | library | monorepo | api | mobile | desktop | other",
    "primary_language": "string",
    "stack": ["string"],
    "entry_points": ["string"]
  },
  "structure": {
    "directories": ["string"],
    "depth": 3,
    "total_files": 0,
    "total_dirs": 0
  },
  "contributors": [
    {
      "name": "string",
      "email": "string",
      "commits": 0,
      "percentage": 0,
      "primary_areas": ["string"],
      "first_commit": "ISO date",
      "last_commit": "ISO date"
    }
  ],
  "primary_developer": {
    "name": "string",
    "email": "string",
    "confidence": "confirmed | inferred | gap"
  },
  "commit_patterns": {
    "total_commits": 0,
    "frequency": "daily | weekly | burst | sporadic",
    "message_style": "conventional | descriptive | terse | mixed",
    "average_message_length": 0,
    "scope_usage": "always | sometimes | never",
    "body_inclusion_rate": 0
  },
  "signals": {
    "formatting": "strict | relaxed | enforced-by-tool | absent",
    "ci_cd": "present | absent",
    "ci_complexity": "simple | moderate | complex",
    "documentation_quality": "high | medium | low | absent",
    "gitignore_thoroughness": "thorough | basic | minimal"
  },
  "metadata": {
    "analyzed_at": "ISO timestamp",
    "analyzer": "kiyotaka-observer"
  }
}
```

## Fields Description

- `project.type`: Best-fit classification of the project
- `contributors`: Ordered by commit count (descending)
- `primary_developer.confidence`: How confident we are in the identification
- `commit_patterns.frequency`: Overall rhythm of development
- `signals`: Quick-read indicators of developer habits
