# Reverse-engineering workflow

## Sources of truth

- `config/target.toml`: immutable target identity.
- the hash-attested IDA IDB: semantic working database, never committed.
- `config/functions.csv`: function inventory and reconstruction lifecycle.
- `config/known-symbols.csv` and `config/known-globals.csv`: supported names.
- `config/claims.csv`: active address ownership.
- function/object comparison reports: exact-match acceptance evidence.

The ignored TH06/TH08 checkouts are supporting evidence. Their layouts and
source do not override TH07 instructions.

## Function lifecycle

1. Verify the local executable and the active IDA session.
2. Select and record a small unclaimed address range.
3. Inspect function metadata, disassembly, decompilation, callers, callees,
   xrefs, strings, globals, RTTI/vtables, and neighbors.
4. Reconcile the IDA extent with target control flow and the ledger.
5. Record a supported name, module, ABI, and evidence.
6. Recover object layout and behavior before source-shaping work.
7. Implement in a bounded translation unit.
8. Compile with the VC7 toolchain and compare the function/object.
9. Record exact results and promote status only as far as the evidence permits.
10. Regenerate progress and release the claim.

## Adjacent-version acceleration

`scripts/cross-version-clones.py` compares TH07 functions to the exact TH06
1.02h and TH08 1.00d binaries using relocation-aware instruction signatures.
Its output is a candidate map, not source truth. A candidate becomes
`identified` only after target-side semantic corroboration. It becomes
`matching` only after reconstructed TH07 source compiles to exact target bytes.

## Status meanings

| Status | Meaning |
| --- | --- |
| `unclassified` | inventory only |
| `identified` | role/name supported by evidence |
| `decompiled` | control flow and types documented; no source yet |
| `implemented` | source exists but is not integrated in the target build |
| `compiles` | included in the target build but differs |
| `matching` | byte-identical under the accepted comparison |
| `library` | verified compiler/runtime/third-party code |
| `blocked` | a concrete blocker is documented |

Decompiler resemblance, identical adjacent-version source, and normalized
binary clones are insufficient for `matching`.

