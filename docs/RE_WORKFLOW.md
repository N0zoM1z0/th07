# Headless reconstruction workflow

Start with `AGENTS.md`, the target manifest and the relevant source/ledger rows.
Use Ghidra through the attested headless wrapper. REA, graphical Wine processes
and GUI-attached IDA MCP are prohibited. Historical IDA files and scripts are
retained for provenance and are outside the active workflow.

```bash
python3 scripts/verify-target.py
python3 scripts/ghidra.py check
python3 scripts/ghidra.py --help
```

`scripts/ghidra.py initialize` creates the ignored `ghidra-project/TH07` database.
Before a query, the wrapper checks file identity, PE mappings and six mapped
code samples. It uses one CPU, low process priority and a 2 GiB maximum heap.
Inventory exports remain separate from the reviewed acceptance ledger.

For a source change, first reconcile ABI, semantics, neighbors, switch tables
and complete function boundaries. Claim the bounded address if working with
other contributors. Preserve original evidence, distinguish observations from
inferences and keep unknown destinations blocked.

```bash
# Maintained native product and complete accepted-source replay
python3 scripts/build-whole.py
python3 scripts/replay-exact-units.py
python3 scripts/verify-linked-product.py --require-all-exact

# Rebuild the corresponding product/probe and replay one accepted routine
python3 scripts/replay-exact-units.py --unit f_00410520 --rebuild

# Preserved focused probe workflow
python3 scripts/build.py --unit enemy-lifecycle --compare --json
```

The probe command remains useful for iteration, but promotion requires complete
extent replay rather than an old ledger-sized prefix. Never crop a longer
procedure to the target length. Never infer a relocation destination from the
operand being compared, mask bytes, add fake locals/padding, patch the target,
or transcribe whole functions into assembly.

Run one compiler process at a time; both build paths share a `flock` lock.
Retain compiler PDBs with cached `/GL` objects; missing debug pairs force a
recompile. Wine is configured with empty `DISPLAY`/`WAYLAND_DISPLAY`, a private
32-bit prefix outside the checkout, and menu integration disabled. Keep master
linker PDBs, compiler PDBs used by cached objects and receipts. Do not follow
Wine's `dosdevices/z:` symlink during file searches.

Keep reusable diagnostic scripts and compact evidence under `.analysis/`.
Preview disposable build paths with `python3 scripts/clean-build.py`; use
`--apply` to remove them after the relevant experiment is finished. The cleaner
preserves native builds and their PDB pairs, canonical reports and target inputs.
See [artifact retention](EXECUTABLE_BUILD.md#artifact-retention-and-cleanup).

After new evidence, update `config/functions.csv`, explicit origins, full-extent
units, remaining work and source provenance together. The admission script is a
snapshot importer for the pinned reference, not an automatic way to approve
new source changes. Review its output before committing. Regenerate progress:

```bash
python3 scripts/validate-tracking.py
python3 scripts/build.py --check
python3 scripts/report-reconstruction-status.py --summary
python3 scripts/progress.py
python3 -m unittest discover -s tests -v
python3 scripts/progress.py --check
git diff --check
```

Public CI needs neither the game nor Wine. Local byte replay additionally
requires the original executable, pinned tools/reference and fresh private build
artifacts. A source or header change invalidates the associated receipt and
requires rebuilding. Commit messages use `gpt-6.1-sol: ...`.
