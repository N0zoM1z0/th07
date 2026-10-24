# TH07 reconstruction agent rules

This repository reconstructs one exact binary: the original Japanese TH07
version 1.00b executable whose identity is recorded in `config/target.toml`.
Do not analyze or substitute a localized, patched, or earlier executable.

## Before changing reconstruction state

1. Read `docs/ARCHITECTURE.md`, `docs/RE_WORKFLOW.md`, and the relevant module
   notes.
2. Run `python3 scripts/verify-target.py` and
   `python3 scripts/ghidra.py check`. All tools run headless; do not use REA
   or launch a GUI/game process.
3. Inspect `config/functions.csv` and `config/claims.csv`; claim a small,
   address-bounded unit before implementation.
4. Reconcile Ghidra and historical IDA extents with complete target control
   flow and compiler/linker extents. Analysis inventories are provisional.

## Evidence and status rules

- Keep target observations, adjacent-version inferences, and unknowns distinct.
- TH06/TH08 names and source are supporting evidence only. Confirm every name,
  ABI, field offset, side effect, and boundary against TH07.
- Never mechanically paste decompiler output as source.
- Never add fake-return or empty behavioral bodies merely to make a build link.
- Never use a naked function, whole-function inline assembly, or a
  target-instruction transcription as a substitute for reconstructed source.
  A function may be marked `matching` only when its real C/C++ implementation
  is independently derived from target evidence and strictly compared. Inline
  assembly is allowed only for a narrowly evidenced ABI boundary within an
  otherwise real implementation; it must not encode the function's control
  flow, field accesses, or behavior merely to force a byte match.
- `config/functions.csv` is the source of truth. Use the lifecycle
  `unclassified`, `identified`, `decompiled`, `implemented`, `compiles`,
  `matching`, `library`, or `blocked`.
- `matching` requires `match_percent=100.00` and a reproducible comparison
  command/report in `evidence`. Visual similarity and normalized clone hashes
  cannot grant matching status.

## Analysis database safety

Ghidra headless is the semantic backend. Use `scripts/ghidra.py`, which
checks target identity, PE mapping and six mapped-code samples before every
query. Historical IDA ledgers remain useful evidence but do not authorize GUI
or IDA MCP calls. Never patch target bytes. Keep databases private and mirror
durable discoveries into tracked ledgers and documentation.

Use one compiler process at a time, pin expensive analysis/build processes to
one CPU, lower process priority and keep Ghidra's heap at or below 2 GiB.
Preserve linker PDBs needed for complete LTCG function extents; delete temporary
compiler PDB/PCH files after they are no longer needed.

## ABI and implementation

- Preserve the VC7 x86 ABI: calling conventions, field widths, object layout,
  vtable order, exception behavior, static initialization, and translation-unit
  boundaries.
- Build and compare the smallest affected function or object first.
- Keep generated reports below `build/` and local analysis below `.analysis/`.
- Do not commit original executables, game data, IDA databases, toolchains,
  third-party reference clones, generated reports, or credentials.

## Shared reconstruction state

Only start concurrent agent lanes when explicitly requested. Keep address claims
and source ownership disjoint. One coordinator reviews ABI, compiler profiles,
relocation maps, origins, acceptance ledgers, progress and Git integration.
Even with concurrent analysis, compilation remains serial. Use Ghidra through
the attested headless wrapper; no worker may launch a GUI or use REA/IDA MCP.

## Handoff

Run `python3 scripts/validate-tracking.py`, `python3 scripts/build.py --check`,
`python3 scripts/progress.py --check`, and `git diff --check`. Report addresses,
evidence, old/new status, exact comparison result, and remaining uncertainty.

Commit messages use `gpt-6.1-sol: ...`. Reference source is pinned at
`_reference/some100-th07`; it earns exact credit only through this project's
strict target oracle. Preserve existing accepted source and provenance.
