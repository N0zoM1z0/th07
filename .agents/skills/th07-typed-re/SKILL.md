---
name: th07-typed-re
description: Generate, interpret, and improve target-pinned compiler-aware reconstruction packets with scripts/typed-re.py. Use for TH07 function typing, ABI recovery, stack/local shaping, VC7 codegen diagnosis, canonical mismatch triage, or when adding reusable ZUN/VC7 inference rules and extractor regressions.
---

# TH07 typed reconstruction

Use the helper as a fact extractor below the semantic decompiler layer. Keep
target observations, compiler inferences, and source hypotheses separate.

## Generate a packet

1. Follow `th07-re` preflight and claim rules.
2. Run:

   ```bash
   python3 scripts/typed-re.py ADDRESS --compare --json \
     > build/typed-re-ADDRESS.json
   ```

3. Read `exact_observations` first. Use stack offsets, access widths, register
   homes, saved registers, direct calls, image references, x87 widths, and
   return cleanup as hard constraints.
4. Treat everything in `inferences`, including calling-convention hints and
   compiler recommendations, only as the next probe to test.
5. Accept a result only through the canonical unit's strict 100% comparison.

If the address has no match-unit entry, use the packet for ABI recovery and ask
the coordinator to create the canonical entry before matching iterations.

## Shape source

- Prefer a type or lifetime change that explains several observed instructions.
- Preserve explicit `u8`/`u16` or signed narrow types when `movzx`/`movsx`
  requires them.
- Reproduce register homes and saved-register use through real parameters,
  locals, or side effects; do not add inert padding or fake behavior.
- Diagnose frame mismatches by declaration order and lifetime before changing
  semantics. Diagnose tail mismatches by missing or redundant behavior first.
- Treat adjacent-version `#pragma optimize` regions as source evidence. Probe
  them in the smallest bounded region before adding a new translation-unit
  profile, and retain them only when strict target comparison proves the shape.
- Confirm every TH06/TH08-derived type and name against the packet and target.

## Improve the model

The stable CLIs are `scripts/typed-re.py` and
`scripts/scan-vc7-library.py`; implementation modules live under
`scripts/typed_re/` so extractors and solvers can grow independently.

Only the coordinator may change this automation, its rules, regressions, or
skill. A worker must execute it read-only and return proposed fields/rules plus
raw evidence in its handoff. The coordinator changes the model only when the
knowledge is reusable.

For a new extractor:

1. Define the output as an exact observation only if target bytes determine it
   without a semantic guess; otherwise emit it under `inferences`.
2. Fail closed on target SHA, PE mapping, ledger extent, or incomplete decode.
3. Add a target-pinned assertion to `--check` covering the new field.
4. Run `python3 -m compileall -q scripts/typed_re scripts/typed-re.py
   scripts/scan-vc7-library.py` and
   `python3 scripts/typed-re.py --check`.

For a new compiler rule:

1. Give it a stable ID, explicit feature prerequisites, confidence, actionable
   recommendation, and address-specific evidence.
2. Base `observed_pattern` on at least one strict exact function plus a
   controlled failed/successful probe. Use `hypothesis` when evidence is weaker.
3. Never let a rule rewrite source, mutate IDA, update the ledger, or grant
   `matching` automatically.

## Handoff

Report the address and target size; exact observations used; recommendation
tested; canonical unit command; object/target sizes and first mismatch; and
whether the helper or rule set changed. The coordinator reruns comparisons and
reviews shared rule changes.
