# TH07 reconstruction agent rules

This repository reconstructs one exact binary: the original Japanese TH07
version 1.00b executable whose identity is recorded in `config/target.toml`.
Do not analyze or substitute a localized, patched, or earlier executable.

## Before changing reconstruction state

1. Read `docs/ARCHITECTURE.md`, `docs/RE_WORKFLOW.md`, and the relevant module
   notes.
2. Run `python3 scripts/verify-target.py` and
   `python3 scripts/check-ida-mcp.py`.
3. Inspect `config/functions.csv` and `config/claims.csv`; claim a small,
   address-bounded unit before implementation.
4. Reconcile IDA's function extent with target instructions and the ledger.
   IDA chunks and decompiler output are useful evidence, not accepted boundaries.

## Evidence and status rules

- Keep target observations, adjacent-version inferences, and unknowns distinct.
- TH06/TH08 names and source are supporting evidence only. Confirm every name,
  ABI, field offset, side effect, and boundary against TH07.
- Never mechanically paste decompiler output as source.
- Never add fake-return or empty behavioral bodies merely to make a build link.
- `config/functions.csv` is the source of truth. Use the lifecycle
  `unclassified`, `identified`, `decompiled`, `implemented`, `compiles`,
  `matching`, `library`, or `blocked`.
- `matching` requires `match_percent=100.00` and a reproducible comparison
  command/report in `evidence`. Visual similarity and normalized clone hashes
  cannot grant matching status.

## Analysis database safety

IDA MCP is attached to the file currently open in the GUI and has no program
selector. Re-run the exact metadata preflight for every bounded work unit.
Never call `patch_address_assembles` or otherwise patch target bytes. Preserve
durable discoveries in tracked CSV, source, or documentation rather than only
inside an IDB.

## ABI and implementation

- Preserve the VC7 x86 ABI: calling conventions, field widths, object layout,
  vtable order, exception behavior, static initialization, and translation-unit
  boundaries.
- Build and compare the smallest affected function or object first.
- Keep generated reports below `build/` and local analysis below `.analysis/`.
- Do not commit original executables, game data, IDA databases, toolchains,
  third-party reference clones, generated reports, or credentials.

## Handoff

Run `python3 scripts/validate-tracking.py`,
`python3 scripts/progress.py --check`, and `git diff --check`. Report addresses,
evidence, old/new status, exact comparison result, and remaining uncertainty.

