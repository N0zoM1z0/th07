# Native executable build and evidence

`python3 scripts/build-whole.py` builds the maintained 29-translation-unit graph
serially with VC7 under headless Wine. `python3 scripts/build-reference.py`
reproduces the unmodified pinned reference separately. Both use the upstream
`/GL /LTCG` profile and omit original icon resources. The resulting executables
are private comparison artifacts. Whole-file identity and runtime playability
have not been verified.

Each `build/authored/` or `build/reference/` directory retains:

- `th07.faithful.exe`: faithful no-icon profile output;
- `th07.exe`: same-path metadata link with an identical `.text` section;
- `th07.pdb`, `th07.map`, `pdb-symbols.json`: full native procedure evidence;
- the 29 intermediate objects and `build-receipt.json`: source/tool/command and
  artifact digests;
- generated Japanese headers and compiler/linker logs.

Temporary per-object compiler PDB/PCH files are removed after a successful
build. Master linker PDBs remain necessary evidence. The generated PE's PDB
GUID/age binding is verified during build and replay. No LLVM PDB utility is
required; the bounded reader handles the VC7 MSF/CodeView format directly.

```bash
python3 scripts/build-whole.py
python3 scripts/reference-oracle.py --build-dir build/authored --output build/authored-comparison.json
python3 scripts/replay-legacy-units.py --units enemy-lifecycle bullet-manager replay-added
python3 scripts/replay-exact-units.py
```

The canonical accepted source pool includes four focused COFF implementations
outside this native product. Combining those implementations into one matching
link is outstanding work, as are the eight unaccepted authored routines.

The older `scripts/build-exe.py` and `config/executable.toml` preserve the
historical layout/resource-link plan. Their disabled link gate describes that
older probe-only graph; they are not the current native-product build entry.
Use the commands above and [ORACLE.md](ORACLE.md) for current acceptance.
