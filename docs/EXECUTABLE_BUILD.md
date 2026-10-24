# Native executable build and evidence

`python3 scripts/build-whole.py` cold-builds all 32 maintained translation units
serially with pinned VC7 under headless Wine, then performs faithful and metadata
links. `python3 scripts/build-reference.py` reproduces the unmodified pinned
reference separately. The reference uses 29 `/GL` units and `/LTCG`. The maintained product adds three
linked native COFF units for menu pre-input, result registration and constructor
emission. All are part of the same executable. The result constructor uses a
VC7 dllexport emission attribute; the resulting PE has one additional constructor
export. Both profiles omit original icon resources. Whole-file identity is not
claimed.

Every currently accepted function comes from the maintained product; separate
focused probes are retained as historical work. Full native measurement is
694/694 named functions. Authored acceptance is 686/686, including the explicitly
user-approved 32-byte x87 primitive. The strict full-source completion gate passes;
see [completion requirements](GOAL.md).

Each native build directory retains faithful and metadata executables, master
`th07.pdb`, `th07.map`, bounded `pdb-symbols.json`, all intermediate objects and
compiler PDBs, generated Japanese headers, logs and a source/tool/command receipt.
The receipt records the pinned C2 backend as well as the driver/frontend/linker.
Both C2 load paths are verified before building; legacy receipts without the
backend hash must be rebuilt, and a backend change invalidates every cached TU.
Compiler PDBs are necessary when reusing VC7 `/GL` objects and cannot be discarded
while retaining an incremental cache. The cache verifies both object and PDB
hashes, tool/source/header state and exact compiler command. Missing debug pairs
force recompilation. `--incremental` still performs both complete links.

```bash
python3 scripts/build-whole.py
python3 scripts/replay-exact-units.py
python3 scripts/verify-linked-product.py
# Complete authored-source gate: passes for the current product.
python3 scripts/verify-linked-product.py --require-all-exact
```

Verification binds the PE entry to `WinMainCRTStartup`, checks imports and
unsuppressed link status, verifies native PDB GUID/age and complete procedures,
and compares 132 named application globals/vtables through independent
relocations. Twelve CRT placeholders and two SDK descriptors are reported
separately from that application-data check.
Metadata linking must preserve faithful `.text` and the source-global bytes.
These checks establish a valid build/link artifact and measured function/data
agreement. No game process was executed; playability and whole-file identity
remain unverified.

The older `scripts/build-exe.py` and `config/executable.toml` retain the historical
layout/resource plan. Their disabled link gate belongs to that older probe graph
and is not the current native-product build entry.

## Artifact retention and cleanup

| Path | Retention |
| --- | --- |
| `build/authored/` | Keep the maintained images, objects, paired compiler PDBs, master PDB/map, logs and receipt |
| `build/reference/` | Keep the pinned reference build and the same evidence/cache pairs |
| `build/authored-comparison.json`, `build/reference-baseline.json` | Keep the current full native measurements |
| `build/exact-replay.json`, `build/link-verification.json` | Keep canonical replay and product/global verification |
| `build/frame-probes/`, compiler traces, trig probes and old match-unit outputs | Disposable after compact evidence/scripts are retained under `.analysis/` |
| Named superseded trial reports | Disposable; the cleaner lists their exact paths |
| `.analysis/`, `.private/`, `.tools/`, `_reference/`, `ghidra-project/` | Preserved by the cleaner |

```bash
python3 scripts/clean-build.py             # preview only
python3 scripts/clean-build.py --apply     # remove named disposable paths
```

The allowlist excludes maintained/reference caches and unknown build paths.
Cleanup refuses to run while the shared compiler lock is held, rejects a
symlinked build root and never follows diagnostic directory symlinks. A duplicate
`reference-baseline-v3.json` is removed only if its complete bytes equal the
canonical `reference-baseline.json`. The preview reports the paths and logical
byte total; it does not modify files. Match-unit outputs are removed only when
the canonical manifest proves every accepted unit uses the native product;
accepted legacy COFF supplements remain protected if present.

After cleanup, full replay and the all-exact product gate remain runnable without
rebuilding. Historical focused probes can regenerate their discarded outputs
through `build.py --unit ...`. Avoid deleting compiler PDBs from a retained
`/GL` cache; missing pairs require recompilation.
