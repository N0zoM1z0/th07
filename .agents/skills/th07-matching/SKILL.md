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

## VC7 security-cookie discrimination

When an unoptimized object is blocked by `___security_cookie` or
`@__security_check_cookie@4`, first inspect the target prologue/epilogue. If
the target has no cookie frame, rerun the whole focused unit with
`vc7-debug-od-no-gs` before reshaping source. VC7 does not accept `/GS-`; this
profile disables the feature by omitting `/GS` while keeping `/Od /Ob1 /Op
/G5` identical. Promote the profile only when the no-cookie object reproduces
target bytes and the unit's previously exact functions stay exact.

## Resource-aware iteration

When host CPU is constrained, serialize VC7 builds and strict comparison
batches; workers may continue packet analysis and source drafting in parallel.
Use `nice -n 10` for coordinator integration builds. Never run an unbounded
`rg --follow --no-ignore` from a workspace containing a Wine prefix: Wine's
`dosdevices/z:` symlink points to `/`. Keep `TH07_WINEPREFIX` outside the
workspace, as the canonical scripts do.

Do not force bytes with naked assembly, copied byte arrays, manual padding,
fake types, ABI lies, or target patches. Only the coordinator promotes a row to
`matching`, and only after independently reproducing a 100% strict result.
