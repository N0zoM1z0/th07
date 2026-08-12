# Workflow evolution decision

## Accepted order

TH07 uses the mature TH10.5 control-plane ideas while removing its VC8/LTCG and
Ghidra-specific complexity:

```text
exact target + ledger gate
→ adjacent-version candidate map
→ structured strict VC7 comparator
→ compiler-aware typed instruction facts and probe recommendations
→ match-unit graph + build provenance
→ fresh coordinator / cached worker packets
→ non-overlapping parallel matching lanes
→ original-object/TU evidence where available
→ partial link and executable-level reccmp
```

The first five gates must be demonstrated on `TextHelper` before parallel
source shaping starts. This prevents visually similar TH06/TH08 source from
being mistaken for reconstructed TH07 bytes.

## Evidence classes

- Target PE/IDA observations establish identity, boundaries, ABI, data, and
  behavior.
- Adjacent-version source and relocation-normalized clones prioritize and
  corroborate candidates; they do not grant status.
- A focused VC7 probe proves function code generation only. It does not claim
  the original translation-unit/object boundary.
- A 100% fail-closed function comparison grants `matching` for that function.
- A future linked executable and reccmp report remain the final authority for
  link order, folding, layout, imports, resources, and whole-program identity.

## Parallel invariant

The coordinator owns semantic database writes and all shared state. Workers get
preclaimed addresses, fresh-then-cached packets, an exclusive source file, and
a canonical unit command. They return source plus structured comparison output;
the coordinator reproduces and integrates it. Shared layouts, relocation
allowlists, toolchain profiles, ledger status, commits, and pushes stay serial.

## Scheduling policy

Each wave should combine:

- an exact adjacent-version clone lane for rapid accepted bytes;
- a class/layout lane that unlocks several dependent methods;
- a subsystem/hard-path lane so progress does not become a trivial-accessor
  counter.

Breadth-first semantic recovery is preferred when a function is blocked by a
translation-unit or compiler-shaping question. The mismatch packet is retained
and a worker moves to another bounded function; only strict 100% results count
in the progress badge.

## Typed reconstruction helper

`scripts/typed-re.py` is the first executable layer of accumulated ZUN/VC7
knowledge. It reads only the attested target and tracked manifests, then emits
machine-readable facts for stack slots, operand widths, register homes, saved
registers, absolute globals/constants, direct calls, cleanup ABI, and known
compiler-shaping patterns. With `--compare` it also runs the canonical unit and
classifies the current frame/tail mismatch:

```bash
python3 scripts/typed-re.py 0x0043E0A0 --compare --json \
  > build/typed-re-0x0043E0A0.json
```

The helper deliberately separates `exact_observations` from `inferences` and
never rewrites source or promotes ledger status. New exact matches can add
small target-pinned rules to `config/typed-re-rules.toml`; over time those rules
become a compiler-aware reconstruction model that workers consume directly,
without turning decompiler guesses into evidence.
