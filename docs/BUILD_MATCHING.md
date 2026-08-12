# Build and exact-matching plan

## Toolchain

The target's PE linker and Rich header identify Visual C++ .NET 2002 (VC7,
build 9466). The TH06 and TH08 reference repositories already contain a
reproducible acquisition/configuration path for that compiler and the DirectX 8
SDK. TH07 will reuse the toolchain design while keeping its own source list,
link order, addresses, globals, and comparison manifests.

The project wrapper pins the audited TH08 tooling commit, installs the VC7 and
DirectX 8 environment below ignored `.tools/`, keeps the Wine prefix in the
workspace's sibling `.th07-wine-vc7` directory, and applies its
`#pragma var_order` compiler shim. Keeping Wine outside the workspace prevents
forced symlink-following file searches from escaping through Wine's `z:` drive:

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

The match units are explicitly focused probes.
Their success can prove function code generation but does not claim the
original TH07 object partitions.

## Exact code-generation results

The pinned VC7 build and patched `C1XX.DLL` currently reproduce 70 functions
across eleven probes:

| Probe | Functions | Exact bytes |
| --- | ---: | ---: |
| `text-helper` | 11 | 2,992 / 2,992 |
| `midi` | 11 | 725 / 725 |
| `zwave` | 19 | 3,316 / 3,316 |
| `ecl-manager` | 2 | 178 / 178 |
| `bullet-render` | 2 | 1,085 / 1,085 |
| `controller` | 3 | 587 / 587 |
| `screen-effect` | 4 | 528 / 528 |
| `bullet-manager` | 4 | 576 / 576 |
| `player-core` | 3 | 892 / 892 |
| `player-bomb` | 3 | 834 / 834 |
| `chain` | 8 | 961 / 961 |
| **Total** | **70** | **12,674 / 12,674** |

The strict comparison resolves member and CRT calls; validates GDI32, WINMM,
and KERNEL32 IAT entries; checks global, string, and vtable target bytes; and
recognizes VC7's compiler-generated `FS:[0]` SEH-chain operands. Local EH thunk
addresses remain explicit allowlisted evidence rather than wildcard
relocations. Zero-addend function pointers may map directly to an address only
when that address is a canonical function start in the target-attested ledger;
globals and literals still require an explicit relocation allowlist entry.
Uniform constants/storage use literal validation at every permitted addend.
Directly evidenced non-uniform tables use address validation with an explicit
addend set, an identifying base-byte sample, and a mapped-range check against
the hash-attested target image.
Accepted commands include:

```bash
python3 scripts/build.py --unit text-helper --compare --json
python3 scripts/build.py --unit midi --compare --json
python3 scripts/build.py --unit zwave --compare --json
python3 scripts/build.py --unit screen-effect --compare --json
python3 scripts/build.py --unit bullet-manager --compare --json
python3 scripts/build.py --unit player-core --compare --json
python3 scripts/build.py --unit chain --compare --json
```

The `vc7-debug-od-no-gs` profile is an evidence-controlled variant for target
functions whose prologues lack VC7 security-cookie frames. VC7 does not accept
`/GS-`; the profile omits `/GS` while holding the remaining `/Od /Ob1 /Op /G5`
switches constant.

## Guardrails

Do not use naked assembly, copied target byte arrays, manual padding, fake
types, or ABI lies solely to force a match. Prefer source expressions that the
original programmer could reasonably have written and keep semantics stable
while changing code shape.
