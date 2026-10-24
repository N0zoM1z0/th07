# Tool entry points

Run commands from the repository root. The build and Ghidra wrappers configure
headless execution, CPU limits and the pinned private/shared tools.

| Task | Entry point |
| --- | --- |
| Verify the original executable | `verify-target.py` |
| Prepare pinned tools | `bootstrap-tools.py` |
| Query the attested target database | `ghidra.py` |
| Build the maintained executable | `build-whole.py` |
| Replay every admitted full extent | `replay-exact-units.py` |
| Verify linking, globals and complete source | `verify-linked-product.py --require-all-exact` |
| Reproduce the unmodified reference | `build-reference.py`, then `reference-oracle.py` |
| Validate public accounting | `validate-tracking.py`, `build.py --check` |
| Regenerate/check Markdown and SVG | `progress.py`, `progress.py --check` |
| Preview/remove obsolete diagnostics | `clean-build.py`, `clean-build.py --apply` |

`build-reference.py` also supplies the maintained build engine. Its receipts bind
the source tree, compiler/linker, objects, paired compiler PDBs and linked image.
`reference-oracle.py`, `compiler_eh.py`, `compiler_statics.py`,
`compiler_thunks.py`, `pdb_symbols.py` and `pe_image.py` provide native evidence
and strict comparison. The synthetic controls are in `tests/`.

Cleanup defaults to a preview and removes only named diagnostic paths. It refuses
to run while the compiler lock is held. Maintained/reference builds, their paired
PDBs, canonical reports, private inputs, tools and target analysis are preserved.
Keep reusable diagnostic scripts and compact evidence under `.analysis/` before
discarding scratch work in `build/`.

`build.py --unit ...`, `compare-function.py`, `typed-re.py`, the library scanners
and `replay-legacy-units.py` support historical focused probes. They do not
replace full maintained-product replay. `build-exe.py` belongs to that older
layout graph. IDA export/client scripts are retained for provenance and are
outside the current workflow; see [IDA_MCP.md](../docs/IDA_MCP.md).

`import-reconstruction.py` is a reviewed snapshot importer, not an automatic
acceptance command for new source changes. Review its changes to origins,
boundaries, provenance and acceptance before committing.
