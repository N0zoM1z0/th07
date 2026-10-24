# Native executable build and evidence

`python3 scripts/build-whole.py` cold-builds all 29 maintained translation units
serially with pinned VC7 under headless Wine, then performs faithful and metadata
links. `python3 scripts/build-reference.py` reproduces the unmodified pinned
reference separately. Both use `/GL /LTCG` and omit original icon resources.

Every currently accepted function comes from the maintained product; separate
focused probes are retained as historical work. Full native measurement is
692/694 named functions. Authored acceptance is 683/686 because the assembler-only
helper is withheld. [Completion requirements](GOAL.md) remain unmet.

Each native build directory retains faithful and metadata executables, master
`th07.pdb`, `th07.map`, bounded `pdb-symbols.json`, all intermediate objects and
compiler PDBs, generated Japanese headers, logs and a source/tool/command receipt.
Compiler PDBs are necessary when reusing VC7 `/GL` objects and cannot be discarded
while retaining an incremental cache. The cache verifies both object and PDB
hashes, tool/source/header state and exact compiler command. Missing debug pairs
force recompilation. `--incremental` still performs both complete links.

```bash
python3 scripts/build-whole.py
python3 scripts/replay-exact-units.py
python3 scripts/verify-linked-product.py
# Completion gate: currently fails because three authored routines remain.
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
