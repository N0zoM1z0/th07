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
   For a zero-addend function pointer, `--dir32-target SYMBOL=0xADDRESS` is
   accepted only when the address is a canonical function start in the
   target-attested ledger. Globals and literals still require the relocation
   allowlist. Use `validation=literal` for constants and uniformly initialized
   storage. Use `validation=address` only for directly evidenced non-uniform
   tables: enumerate every accepted addend, retain an identifying base sample,
   and require every resolved address to map inside the hash-attested target.
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

If no-GS fixes the frame but 16-bit arithmetic still differs as
`mov`+`and 0xffff` versus `movzx`, A/B `/G5` against `/G6` without changing
source. The canonical `vc7-debug-od-no-gs-g6` profile exists for this case.
Promote it only after all previously exact functions in the focused unit remain
exact; TH07's DirectSound unit is the reference proof for this discriminator.

If an `/Od` probe has the right ABI and source semantics but emits immediate or
member-wise stores where the target uses pooled FPU constants, `movsd` aggregate
copies, and inlined source helpers, A/B the source unchanged under
`vc7-size-ob1-no-gs-g6` (`/Os /Ob1 /Op /G6`). Do not simulate the target with
volatile fields, fake locals, or manual copies. Promote the size-oriented
profile only after strict relocation-aware equality; `spellcard-gui` is the
reference proof for this discriminator.

## Resource-aware iteration

All compilation runs headlessly through one shared `flock` slot, pinned to one
CPU with low process priority. Preserve the native master PDB, remove unused
per-object PDB/PCH files, and keep Wine's prefix outside the checkout. Never run
an unbounded search following Wine's `dosdevices/z:` symlink to `/`.

Current acceptance uses `scripts/replay-exact-units.py` and full native PDB or
COFF extents. A ledger-sized probe prefix cannot grant exact credit. See
`docs/ORACLE.md` for source policy and independent relocation rules.

Do not force bytes with naked assembly, copied byte arrays, manual padding,
fake types, ABI lies, or target patches. Only the coordinator promotes a row to
`matching`, and only after independently reproducing a 100% strict result.
