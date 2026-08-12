---
name: th07-re
description: Reconstruct the original Japanese TH07 1.00b executable using exact target evidence, adjacent TH06/TH08 corroboration, and the repository ledger.
---

# TH07 reconstruction

Use this skill only for the target identity in `config/target.toml`.

1. Read `AGENTS.md`, `docs/ARCHITECTURE.md`, `docs/RE_WORKFLOW.md`, the
   selected ledger row, and active claims.
2. Run `scripts/verify-target.py` and `scripts/check-ida-mcp.py` before fresh
   semantic work. IDA is attached to the GUI's current file and has no program
   selector.
3. Reconcile the ledger boundary with target instructions. IDA chunks and
   decompiler sizes are advisory.
4. Run `python3 scripts/typed-re.py ADDRESS --json` when the function is
   instruction-decodable. Treat `exact_observations` as target evidence and
   `inferences` / compiler recommendations only as source-shaping hypotheses.
5. Label conclusions Observed, Inferred, or Hypothesized. TH06/TH08 source and
   clone signatures corroborate TH07; they do not override it.
6. Recover ABI, object layout, side effects, callers/callees, globals, and
   control flow before source shaping.
7. Keep source in the smallest appropriate module. Do not add fake bodies.
8. Use `th07-matching` for compilation and byte-match work. Update shared
   ledger/name/claim state only when acting as coordinator.

Never patch the target. Never commit copyrighted binaries, analysis databases,
toolchains, reports, credentials, or the ignored adjacent-version clones.
