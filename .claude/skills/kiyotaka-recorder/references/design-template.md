# Design Template

```markdown
# Design Patterns

> How this developer designs solutions — their architectural DNA.

---

## Architectural Preferences

### Primary Paradigm
[OOP / Functional / Hybrid / Event-driven]

### Structural Pattern
[MVC / Layered / Clean Architecture / Feature-based / ...]

### Communication Pattern
[REST / GraphQL / gRPC / Message Queue / ...]

---

## Signature Design Moves

### Move 1: [Name]
**Description**: [What this pattern does]
**When used**: [Trigger condition]
**Structure**:
```
[pseudo-code or description of the pattern]
```
**Evidence**: [file paths]
**Consistency**: [always/mostly/sometimes]

### Move 2: [Name]
[Same structure as above]

---

## Abstraction Threshold

| Trigger | Threshold | Evidence |
|---------|-----------|----------|
| Parameters before options object | [N] | [files] |
| Nesting levels before refactor | [N] | [files] |
| Repetitions before extraction | [N] | [files] |
| Files before module creation | [N] | [files] |

---

## State Management

[How the developer manages state — local, global, server, etc.]

---

## Dependency Management

- **Selection criteria**: [How they choose libraries]
- **Version strategy**: [bleeding edge / pinned / conservative]
- **Custom vs. library**: [When they build vs. install]

---

## Error Design

[How errors flow through the system they design]

---

## Test Design

[How they structure their testing approach]

---

## Anti-Patterns (What This Developer Avoids)

1. [Anti-pattern they never use]
2. [Anti-pattern they never use]

---

## Confidence Assessment

| Design Decision | Confidence | Evidence |
|----------------|------------|----------|
| [decision] | 🟢/🟡/🔴 | [evidence] |
```
