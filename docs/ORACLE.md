# Strict function oracle

The original Japanese executable is identified by SHA-256 and size before every
measurement. The reference is some100/th07 commit
`4878bf0e42a9280ef44735eb0517d4fda7cb6e04`, retained privately under
`_reference/some100-th07`. Similarity percentages do not grant exact acceptance.

## Native evidence

`scripts/build-reference.py` reproduces the upstream 29-TU VC7 13.00.9466 profile
with pinned `var_order`, Japanese localization generation and `/GL /LTCG`.
The maintained product adds the recorded `MainMenuPreInput.cpp` native COFF
unit to that graph, preserving a constructor call with a TU-local compiler
attribute. All accepted code is in the same linked product.
The faithful profile omits the original icon. `/GL` objects contain intermediate
representation; native sizes must come from the completed link, not apparent
COFF prefixes.

The bounded MSF/CodeView reader supplies full procedure extents, types and
source-object ownership. It binds the master PDB GUID/age to the generated PE.
A same-path metadata link adds `/FIXED:NO`; its `.text` must remain byte-identical
to the faithful link. DIR32 fields come from that image's relocation directory.
Reachable generated x86 instructions supply relative fields and traverse switch
tables. Replay compares every byte through the complete procedure/table extent.

Destinations come from original symbol/global annotations, import identities,
typed literal content, same-procedure offsets and independently validated compiler
metadata. Unknown destinations remain unaccepted. No target instruction operand
selects an expected destination and no relocation byte is masked. Linked-address
raw equality can also establish a complete exact range.

Oracle v2 additionally handles:

- const f32/f64 objects whose compiler TPI type establishes width and constness,
  with a unique aligned typed read-only original literal;
- offsets bounded inside a complete independently mapped string literal;
- uint32 local-static guards adjacent to typed, named BSS objects, constrained by
  original object sizes and non-overlap with other globals;
- unique complete five-byte CRT forwarding entries to independently mapped callees;
- compiler EH graphs matched by state shape and complete cleanup/handler replay,
  with unique one-to-one assignments, rather than contribution order.

Synthetic negative controls reject ambiguous literals/thunks, wrong independent
addresses, missing type/size evidence, nonadjacent guards, truncated procedures,
trailing table corruption, wrong PDB GUID/age and stale build artifacts.

Receipts bind source/header state, driver/frontend/backend/linker hashes, commands,
objects, compiler PDBs and final native image/PDB/map metadata. Both the private
compiler backend and the stock LTCG backend must match the pinned `c2.dll` hash.
Native replay rejects receipts without that backend identity. Missing compiler
PDBs invalidate an incremental object pair. Native PDB metadata is parsed afresh
on replay; cached JSON cannot override it. Reports remain private in `build/`.
Public manifests retain source ownership, coverage/full comparison sizes and
SHA-256 of complete target ranges.

## Reference and maintained measurements

| Current oracle v2 scope | Exact functions | Exact coverage bytes |
| --- | ---: | ---: |
| Unmodified reference, named scope | 686 / 694 | 345,019 / 381,170 (90.5158%) |
| Maintained linked product, named scope | 693 / 694 | 380,831 / 381,170 (99.9111%) |
| Reviewed authored functions in that product | 685 / 686 | 380,154 / 380,493 (99.9109%) |

The initial v1 reference measurement was 682/694 and 89.6149%. It is preserved
in `config/legacy/reference-baseline-v1.csv`. V2 removes four independently
resolved metadata/literal false negatives without changing reference source or
its target denominator. No relaxation of byte comparison is involved.

The maintained signed score correction recovers ECL's complete 30,156-byte
procedure, including 692 table bytes beyond the 29,464 coverage bytes.
Further fixes recover initialization call boundaries, sample alignment, unsigned
array comparison and CRT parser identity. [Matching notes](MATCHING_NOTES.md)
record target evidence and rejected experiments. The menu pre-input loop now
replays all 3,184 bytes, including its 32-byte table, through the isolated native
profile described in [frame diagnostics](FRAME_DIAGNOSTICS.md).

Every accepted function now comes from **one maintained linked product**.
The four former standalone supplements—ending parameter parsing, enemy chain
registration, bullet setup and replay recording setup—also pass there. Old focused
probes and their original ledgers remain for provenance, and are no longer
canonical supplements.

## Authored scope and source policy

Authored means reconstructed source-owned application functions, including
adapted DirectSound utilities and PBG codecs. It describes reconstruction
ownership, rather than original copyright authorship, following TH06's convention.

The denominator remains **686 functions / 380,493 bytes**. Three D3DX inline
routines and five compiler deleting wrappers are excluded from the 694 named
functions. Prebuilt CRT/SDK code, compiler EH/startup code, alignment and embedded
DInput data are outside authored scope. [Boundary review](BOUNDARY_REVIEW.md)
records closure without unreviewed application gaps.

`Float2`/`Float3::FromAngleMagnitude` at `0x004325C0` is a complete 32-byte
match. On 2026-10-24 the user explicitly approved its FSINCOS/scaled-output
implementation as one narrow x87 mathematical-primitive exception.
`config/source-policy.toml` binds that approval to the target SHA-256, address,
complete extent and both reviewed source bodies. Admission and replay reject a
changed scope or body; removing/revoking approval withholds acceptance. All other
whole-function assembly substitutes remain prohibited. The denominator is
unchanged.

## Remaining authored work

| Address | Function | Coverage bytes | Blocker |
| --- | --- | ---: | --- |
| `0x0044A302` | `ResultScreen::RegisterChain` | 339 | Stack frame 0x38 versus 0x2C |

`config/functions.csv`, `function-origins.csv`, `matches.csv` and
`exact-units.json` jointly define current acceptance. `config/remaining.csv`
records unfinished source work. The historical ledger/claims remain verbatim
under `config/legacy/`; the admission importer repairs five unquoted historical
CSV fields while loading that archive.

## Build and link verification limits

A fresh 30-TU cold build and both links pass. The product verifier checks startup
binding, imports, unsuppressed linker status and complete native measurements.
It also replays **132 named application globals/vtables** through independently
resolved relocations, and checks their faithful/metadata byte agreement.
Twelve historical CRT placeholder globals and the two SDK object-format
descriptors are separately reported outside that application-data check.

The active completion gate is `verify-linked-product.py --require-all-exact`;
it deliberately fails while any authored work remains. Current build/link
verification passes, and exact-source completion is false.

The percentage measures non-overlapping function coverage, not `.text` or file
identity. Table tails are checked without counting them twice. No game was run;
playability remains unverified. Upstream reports an integrity-check crash after
3,999 update cycles in its matching build; no check was suppressed here.
