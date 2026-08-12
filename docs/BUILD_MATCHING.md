# Build and exact-matching plan

## Toolchain

The target's PE linker and Rich header identify Visual C++ .NET 2002 (VC7,
build 9466). The TH06 and TH08 reference repositories already contain a
reproducible acquisition/configuration path for that compiler and the DirectX 8
SDK. TH07 will reuse the toolchain design while keeping its own source list,
link order, addresses, globals, and comparison manifests.

The project wrapper pins the audited TH08 tooling commit, installs the VC7 and
DirectX 8 environment below ignored `.tools/`, and applies its
`#pragma var_order` compiler shim:

```bash
scripts/bootstrap-tools.sh
```

System Wine/msitools packages remain host prerequisites and are not modified by
the repository script.

## Staged acceptance

1. Verify the exact target and export the address ledger.
2. Recover one small routine and its ABI.
3. Compile the smallest plausible translation unit with VC7.
4. Compare target instructions and relocations at function scope.
5. Add object-level objdiff configuration once original object partitions are
   supported by evidence.
6. Add executable-level reccmp only after source, globals, link order, and PE
   layout are sufficiently complete.

Every comparison is fail-closed. Unresolved calls, absolute data references,
imports, float literals, and string relocations must be mapped to an exact
symbol or reported as blockers. A standalone object can establish exact
function code generation, but cannot by itself prove the original translation
unit or final linked executable.

## Match-unit graph

`config/match-units.toml` maps a focused source probe to a generated VC7 COFF
object, compiler profile, exact decorated symbols, target addresses, and any
strict relocation mappings:

```bash
python3 scripts/build.py --check
python3 scripts/build.py --list
python3 scripts/build.py --unit text-helper --compare --json
```

Each successful compile emits an ignored provenance sidecar beside the object.
It records the source, manifest, compiler runner, `cl.exe`, patched `C1XX.DLL`,
and object hashes. The JSON comparator has stable `exact`, `mismatch`,
`blocked`, and `error` outcomes and reports the first differing byte.

The first `text-helper` unit is explicitly a focused probe. Its success can
prove function code generation but does not claim the original TH07 object
partition.

## First exact code-generation result

The pinned VC7 build and patched `C1XX.DLL` reproduce all six implemented
`TextHelper` allocation methods:

| Address | Function | Bytes |
| --- | --- | ---: |
| `0x00431A0F` | constructor | 60 / 60 |
| `0x00431A4B` | destructor | 17 / 17 |
| `0x00431A5C` | release buffer | 114 / 114 |
| `0x00431ACE` | allocate with fallback | 95 / 95 |
| `0x00431B2D` | DIB allocation | 447 / 447 |
| `0x00431CEC` | format lookup | 80 / 80 |

The strict comparison resolves member and CRT calls, validates five exact
GDI32 import-table entries, and validates the `g_FormatInfoArray` relocation at
`0x0049ED98`. The accepted command is:

```bash
python3 scripts/build.py --unit text-helper --compare --json
```

## Guardrails

Do not use naked assembly, copied target byte arrays, manual padding, fake
types, or ABI lies solely to force a match. Prefer source expressions that the
original programmer could reasonably have written and keep semantics stable
while changing code shape.
