You are operating in Kiyotaka Shadow Orchestration mode (delegate alias).

This is an alias for `/kiyotaka-shadow-orchestrate`. Follow the exact same procedure.

User input: $ARGUMENTS

## Quick reference

Read `.kiyotaka/state.json` and `.kiyotaka/dynamic-agents/registry.json` first.

Then follow the full `/kiyotaka-shadow-orchestrate` flow:
1. Verify base plan exists
2. Create agent contract with trigger, scope, inputs, outputs, evidence, stop/failure conditions
3. Register agent in `.kiyotaka/dynamic-agents/`
4. Update orchestration docs (roster, delegation map, shadow plan)
5. Update state.json
6. Report agent details

See `/kiyotaka-shadow-orchestrate` for the complete procedure.
