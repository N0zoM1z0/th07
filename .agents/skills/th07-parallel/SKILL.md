---
name: th07-parallel
description: Coordinate non-overlapping TH07 reconstruction workers while protecting shared IDA, ledger, ABI, and Git state.
---

# TH07 parallel reconstruction

Use together with `th07-re` for two or more bounded lanes.

## Coordinator ownership

The coordinator alone writes IDA names/types/comments, claims, ledgers,
known-symbol/global manifests, shared headers/layouts, compiler profiles,
relocation allowlists, typed-RE helper code/rules/regressions, generated
progress, commits, and pushes. Workers never commit or push. They may execute
typed-RE helpers and propose improvements, but never edit `scripts/typed_re/`,
its CLI wrappers, `config/typed-re-rules.toml`, or `th07-typed-re`.

Before dispatch, the coordinator:

1. verifies target and IDA metadata;
2. reconciles exact function boundaries;
3. records every address with `scripts/claim.py`;
4. captures fresh work packets and creates canonical match units;
5. assigns non-overlapping addresses and exclusive source files.

Workers use `scripts/work-packet.py ADDRESS --cached`; they do not refresh
packets or mutate IDA. Source work is allowed only in the files explicitly
named in the brief. Shared-state changes are returned as proposals.

## Worker brief

Every brief states:

- exact included addresses and excluded neighboring ranges;
- analysis-only or the exclusive writable source file(s);
- unit name and canonical build/compare command;
- prohibition on IDA, ledger, claim, shared ABI, Git, and allowlist writes;
- expected record per address: Observed/Inferred/Hypothesized evidence,
  candidate ABI/body, exact JSON result or first mismatch/blocker, and unknowns.

Use one coordinator plus up to three `gpt-5.6-terra` workers at `high`
reasoning in the current four-slot environment. Prefer reusing a worker for a
follow-up in the same class/module so recovered layout context is retained.
If the user requests low CPU usage, keep parallel work analysis-heavy and
serialize compiler/comparator invocations through the coordinator. Workspace
search must exclude `_references`, `.tools`, and `build`, and must not follow
symlinks. Keep the Wine prefix outside the workspace because it exposes `/`
through `dosdevices/z:`.

The coordinator reviews diffs, reruns every comparison, applies durable IDA
and ledger changes, releases claims, validates progress, commits, and pushes.
