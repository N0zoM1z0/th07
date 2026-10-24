# Strict function oracle

The original Japanese executable is identified by SHA-256 and size before every
native measurement. The reference is some100/th07 commit
`4878bf0e42a9280ef44735eb0517d4fda7cb6e04`, retained privately under
`_reference/some100-th07`. Its upstream similarity percentage is not an exact
acceptance result under this oracle.

## Native evidence

`scripts/build-reference.py` reproduces the upstream 29-translation-unit
VC7 13.00.9466 matching profile. It generates the Japanese header, uses the
pinned `var_order` frontend, and links with `/GL /LTCG`. The faithful baseline
omits the original icon resource, following upstream's no-icon build option.
Raw `/GL` objects contain intermediate representation, so their apparent COFF
symbols cannot establish native procedure sizes.

The native linker PDB supplies complete procedure sizes and source-object
ownership. `pdb_symbols.py` parses bounded MSF/CodeView streams and checks that
the PDB GUID and age match the generated PE. A second link at the same output
paths requests `/FIXED:NO` metadata. Its `.text` must be byte-identical to the
faithful link; its base relocation directory enumerates DIR32 fields. Generated
reachable x86 control flow supplies relative call/branch fields and traverses
switch tables. Full procedure extents, including table bytes, are compared.

Destinations come from original symbol/global annotations, PE import identities,
typed literal content, same-procedure offsets and independently validated compiler
EH graphs. EH graphs are matched by state structure and complete cleanup/handler
replay, with unique one-to-one assignments. Descriptor ordering is not assumed.
Unknown or ambiguous destinations remain unaccepted. No target instruction
operand is used to infer an expected destination, and no relocation byte is
masked. An identical linked-address procedure can also pass by direct full-byte
comparison. All accepted functions require every byte to match.

Receipts retain tool hashes, source/header hashes, commands, object hashes and
native image/PDB/map hashes. Replay checks current source and artifact hashes,
freshly parses PDB metadata and verifies image binding. Reports stay in ignored
`build/`; public ledgers retain coverage, full comparison size, source ownership
and the SHA-256 of the complete target range.

## Reference measurement and maintained changes

The unmodified reference yielded **682 / 694 exact named functions**, or
**341,585 / 381,170 coverage bytes (89.6149%)**. Its large ECL procedure was
longer than the original function and overlapped the next original entry.

The target ECL uses `FILD DWORD` to convert `g_SpellcardScore`. Upstream declared
that array `u32`; the maintained source uses `i32` in its definition and extern
declaration. Recompiling all 29 translation units gives a **30,156-byte ECL
procedure**, comprising 29,464 coverage bytes and 692 trailing switch-table
bytes, all exact. No padding, fabricated locals or instruction transcription was
introduced. The maintained native build reaches **683 / 694**, or
**371,049 / 381,170 bytes (97.3448%)**, in the same named scope.

Four retained original-repository probes supplement that build:

| Target | Function | Coverage / full compared bytes | Preserved source |
| --- | --- | ---: | --- |
| `0x0041D410` | `Ending::ReadEndFileParameter` | 115 / 115 | `src/EnemyLifecycle.cpp` |
| `0x00422F40` | `EnemyManager::RegisterChain` | 267 / 267 | `src/EnemyLifecycle.cpp` |
| `0x00426F60` | `BulletManager::AddedCallback` | 1,703 / 1,723 | `src/BulletManager.cpp` |
| `0x00443040` | `ReplayManager::AddedCallbackRecord` | 870 / 870 | `src/ReplayAdded.cpp` |

All three probe units were rebuilt headlessly; their 14 previously accepted
comparisons passed. The four canonical supplements additionally replay their
complete native COFF auxiliary-symbol extent, including BulletManager's 20-byte
table, using independently evidenced relocation mappings. These source units
are separate from the maintained LTCG product; they have not been linked into
a single product with the native implementations.

## Authored scope and exclusions

Authored here means reconstructed source-owned application functions, including
game-adapted DirectSound utilities and PBG codecs. It describes reconstruction
ownership, rather than original copyright authorship. This follows the TH06
accounting convention.

The reviewed authored denominator is **686 functions / 380,493 bytes**. It
excludes three D3DX inline routines and five compiler deleting wrappers from
the 694-function named set. Linked CRT/SDK code, compiler EH/startup code,
DInput object-format data and alignment are also outside authored scope.
[Boundary review](BOUNDARY_REVIEW.md) records closure of the application region.

`Float2::FromAngleMagnitude` at `0x004325C0` is byte-identical but its entire
behavior is an inline assembler field-writing routine. Repository policy rejects
whole-function assembler substitutes. It remains a build dependency and counts
as unaccepted authored source until recovered as admissible C++.

After those exclusions and the four probe supplements, accepted authored source
covers **678 / 686 functions; 373,295 / 380,493 bytes (98.1082%)**.
`config/exact-units.json`, `matches.csv`, `function-origins.csv` and
`functions.csv` jointly define the canonical replay and accounting.
`config/legacy/` preserves the prior ledger and claim snapshot verbatim; the
importer repairs five historical unquoted CSV fields when loading that archive.

## Remaining authored functions

| Address | Function | Coverage bytes | Current blocker |
| --- | --- | ---: | --- |
| `0x0041EE70` | `EnemyManager::EnemyManager` | 256 | Native stack frame differs |
| `0x004325C0` | `Float2::FromAngleMagnitude` | 32 | Whole-function assembler source policy |
| `0x004346E0` | `GameWindow::Render` | 855 | Temporary stack offsets differ |
| `0x004390A5` | `Supervisor::DrawFpsCounter` | 860 | Static initialization guard unresolved |
| `0x0043B4DB` | `MusicRoom::RegisterChain` | 223 | Compiler EH/guard destination unresolved |
| `0x0044A302` | `ResultScreen::RegisterChain` | 339 | Native stack frame differs |
| `0x0044EFB0` | `AnmManager::DrawInner` | 1,481 | Static half constant address ambiguous |
| `0x004555DD` | `MainMenu::OnUpdatePreInput` | 3,152 | Native stack frame differs |

The public percentage is function coverage, not `.text` coverage or whole-file
identity. Trailing tables are checked but are not added to numerator/denominator
a second time. The linked image exists as a comparison artifact; no game was
executed and playability has not been established. Upstream documents a matching
build integrity-check crash after 3,999 update cycles; that remains a runtime
validation concern and has not been suppressed or tested here.
