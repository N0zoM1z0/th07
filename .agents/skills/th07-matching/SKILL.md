---
name: th07-matching
description: Compile and tune TH07 VC7 functions against strict relocation-aware target-byte comparisons.
---

# TH07 exact matching

Use together with `th07-re` after function semantics and boundaries are known.

1. Start from a coordinator-defined `config/match-units.toml` unit and cached
   work packet.
2. Run `python3 scripts/build.py --unit UNIT --compare --json`.
3. Treat `exact`, `mismatch`, `blocked`, and `error` as evidence. Classify the
   first mismatch before changing source: ABI/layout, compiler profile, COFF
   symbol selection, REL32/DIR32 mapping, control-flow/source order, or a
   translation-unit/link effect.
4. Add a relocation mapping only after proving the exact COFF symbol, target
   address, addend, and target bytes. Unknown relocations remain blocked.
5. Change one source-plausible expression or declaration at a time and rerun
   the canonical unit.
6. A focused probe does not prove the original object partition. Record that
   limitation even when its function bytes are exact.

Do not force bytes with naked assembly, copied byte arrays, manual padding,
fake types, ABI lies, or target patches. Only the coordinator promotes a row to
`matching`, and only after independently reproducing a 100% strict result.
