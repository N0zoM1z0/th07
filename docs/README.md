# Documentation guide

Start with the current maintained product:

| Document | Purpose |
| --- | --- |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Source layout and native/LTCG TU graph |
| [EXECUTABLE_BUILD.md](EXECUTABLE_BUILD.md) | Build, replay, retained artifacts and cleanup |
| [ORACLE.md](ORACLE.md) | Strict comparison rules, scope and limitations |
| [PROGRESS.md](PROGRESS.md) | Generated acceptance totals |
| [GOAL.md](GOAL.md) | Completion requirements and verified scope |
| [MATCHING_NOTES.md](MATCHING_NOTES.md) | Accepted source corrections |
| [FRAME_DIAGNOSTICS.md](FRAME_DIAGNOSTICS.md) | Compiler experiments and recovered frames |
| [BOUNDARY_REVIEW.md](BOUNDARY_REVIEW.md) | Application, library, data and alignment boundaries |
| [RE_WORKFLOW.md](RE_WORKFLOW.md) | Headless Ghidra and source-change workflow |

[BUILD_MATCHING.md](BUILD_MATCHING.md), [ENEMY.md](ENEMY.md),
[LIBRARY_RECOVERY.md](LIBRARY_RECOVERY.md) and
[WORKFLOW_EVOLUTION.md](WORKFLOW_EVOLUTION.md) preserve the earlier focused-probe
work. Their intermediate status and planned steps do not describe current
acceptance. [IDA_MCP.md](IDA_MCP.md) documents retired tooling; current semantic
analysis uses headless Ghidra.

The authoritative state lives in `config/functions.csv`, explicit function
origins and `config/exact-units.json`. Regenerate progress with
`python3 scripts/progress.py`; do not edit its Markdown/SVG outputs manually.
The [source guide](../src/README.md) and [tool guide](../scripts/README.md) identify
the current build and the retained historical probes.
