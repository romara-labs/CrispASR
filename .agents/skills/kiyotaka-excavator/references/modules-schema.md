# Modules Schema — `.kiyotaka/context/modules.json`

```json
{
  "modules": [
    {
      "name": "string",
      "path": "string",
      "purpose": "string",
      "files": 0,
      "lines_of_code": 0,
      "languages": ["string"],
      "complexity": "low | medium | high | very-high",
      "developer_signature": {
        "naming": "string",
        "function_length": "short | medium | long | mixed",
        "comment_style": "dense | moderate | sparse | absent",
        "error_handling": "thorough | moderate | minimal | absent",
        "confidence": "confirmed | inferred | gap"
      },
      "technical_debt": {
        "todos": 0,
        "fixmes": 0,
        "hacks": 0,
        "dead_code": "none | minimal | moderate | significant",
        "refactoring_habits": "proactive | reactive | absent"
      },
      "patterns": [
        {
          "name": "string",
          "type": "structural | behavioral | architectural | anti-pattern",
          "frequency": "rare | occasional | frequent | ubiquitous",
          "evidence": ["file:line"]
        }
      ],
      "data_structures": [
        {
          "name": "string",
          "type": "model | interface | type | schema | enum",
          "file": "string",
          "fields": 0
        }
      ]
    }
  ],
  "summary": {
    "total_modules": 0,
    "total_lines": 0,
    "average_complexity": "low | medium | high",
    "most_complex_module": "string",
    "cleanest_module": "string"
  },
  "metadata": {
    "analyzed_at": "ISO timestamp",
    "analyzer": "kiyotaka-excavator"
  }
}
```
