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
5. When `--compare` emits `comparison.instruction_shape`, use
   `shared_shape_prefix` to locate the first structural divergence. If
   `topology_exact` is true, inspect `stack_slot_pairs` and instruction sizes
   before changing behavior: the remaining problem is usually constants,
   relocations, local order, or lifetime shaping.
6. Accept a result only through the canonical unit's strict 100% comparison.

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
- Instruction topology is a diagnostic, never acceptance evidence. It
  deliberately abstracts immediate values and displacements; always return to
  the strict byte/relocation report before changing status.
- A target jump table can establish emitted case-destination order and reveal
  shared labels. Treat the recovered source case order as an inference and
  verify every case's fields, signs, side effects, and fallthrough against TH07.
- Read `exact_observations.unaccessed_frame_slots` before shaping a receiver
  home. These are target frame dwords with no decoded memory access; they can
  corroborate a legacy reserved local, but do not by themselves justify adding
  a source local. Require an exact or adjacent-source-supported A/B.
- If target materializes a condition as 0/1 (with `setcc` or a branch diamond)
  and then tests it while a named source flag shifts the receiver home, probe
  the direct compound condition or explicit `condition ? 1 : 0` form. This is
  a controlled VC7 source-shaping A/B, not permission to change branch
  semantics or accept a non-exact result.
- A target frame hole may represent a legacy ZUN source local, but retain one
  only when exact stack evidence and adjacent source identify its semantic role.
  Use a named scalar and the smallest `var_order` probe; anonymous filler,
  padding arrays, and frame-only guesses remain forbidden.
- If target loads a fixed global address into ECX before a call and performs no
  caller cleanup, model the callee as a member of that global overlay. A known
  destination address does not justify a cdecl/free-function prototype that
  changes the target call shape.
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
