# Confidence Rules

## Evidence Thresholds

### 🟢 CONFIRMED (Green)
A claim receives CONFIRMED status when:
- **3+ independent sources** confirm the claim, OR
- **Direct code evidence** exists in 3+ files with consistent patterns, OR
- **Git history** shows consistent behavior over time, AND
- **No contradictory evidence** exists

### 🟡 INFERRED (Yellow)
A claim receives INFERRED status when:
- **1-2 sources** confirm the claim, OR
- **Pattern exists but with exceptions**, OR
- **Deduction is logical but lacks direct evidence**, OR
- **Minor contradictions** exist but don't invalidate the claim

### 🔴 GAP (Red)
A claim receives GAP status when:
- **No direct evidence** exists, OR
- **Major contradictions** are found, OR
- **Insufficient data** to make any determination

## Contradiction Resolution

When two claims contradict:

1. **Count evidence**: Which claim has more supporting evidence?
2. **Check recency**: Which evidence is more recent?
3. **Check scope**: Is one claim about an older part of the codebase?
4. **Flag both**: Never silently resolve — present both claims with confidence levels

## Overall Confidence Calculation

```
Overall Confidence = (Confirmed × 1.0 + Inferred × 0.5 + Gaps × 0.0) / Total Claims × 100%
```

### Reliability Thresholds

| Score | Reliability | Action |
|-------|------------|--------|
| 80-100% | High | Safe for replication |
| 60-79% | Moderate | Use with caveats, verify key claims |
| 40-59% | Low | Re-analysis recommended |
| < 40% | Unreliable | Full re-analysis required |

## Minimum Standards

- A profile must have at least **5 CONFIRMED claims** to be usable for replication
- Any claim about security practices must be **CONFIRMED** before being relied upon
- Emotional signatures must have **temporal evidence** (git history) — never infer emotions from code alone
