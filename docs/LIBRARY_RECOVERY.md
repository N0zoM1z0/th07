# Prebuilt library recovery

TH07 statically links substantial VC7-era D3DX8 and C runtime code. The
repository's pinned toolchain contains the original `d3dx8.lib`, `LIBC.LIB`,
and `LIBCMT.LIB` archives, so these functions should be replayed from their
original i386 COFF members before anyone attempts a manual rewrite.

The extractor fails closed on the archive SHA-256, archive structure, member
identity, output location, and i386 COFF machine type:

```bash
python3 scripts/extract-vc7-library-object.py \
  --library d3dx8 --object obj/i386/d3dxmath.obj \
  --output build/match-units/vc7_d3dx8_d3dxmath.obj
```

Archive symbols, TH06/TH08 clone names, and target sizes are candidate-finding
evidence only. A row remains non-exact until its canonical match unit returns
`result=exact` after every REL32 and DIR32 destination is independently
verified. Ambiguous identical symbols are excluded rather than guessed.

Accepted prebuilt rows retain `status=library`; their exact bytes contribute
to reproducible-library and combined progress, never authored-game matching.
The proprietary archives and extracted objects remain ignored below `.tools/`
and `build/` and are not redistributed by this repository.

## Embedded open-source versions

The pinned D3DX8 archive retains compiler paths and version strings that make
its codec ancestry unusually strong target-adjacent evidence:

- `zutil.obj` names Microsoft's `zlib113` build directory and contains the
  exact `1.1.3` version string. The official historical release is preserved
  in the [zlib fossil archive](https://zlib.net/fossils/).
- `jerror.obj` contains `6a  7-Feb-96` and the 1996 Thomas G. Lane copyright
  string, identifying IJG JPEG release 6a. The release remains available in
  historical source archives; its provenance agrees with the target error
  strings and object symbols.
- The D3DX8 image objects contain `libpng version 1.0.5` and `@1.0.5`, fixing
  the embedded libpng line to 1.0.5.

This enables a second recovery route for functions that cannot be replayed
directly: vendor an unmodified, license-compatible official release under
`third_party/`, apply only a tracked D3DX namespace/configuration adapter, and
compile with VC7. Archive replay remains preferred because it preserves
Microsoft's exact wrappers, macros, flags, and object partition automatically.

## Accepted first wave (2026-08-12)

Four archive members reproduce 29 functions and 9,678 function bytes exactly:

```bash
python3 scripts/build.py --unit vc7-d3dx8-d3dxmath --compare --json
python3 scripts/build.py --unit vc7-d3dx8-cd3dxblt --compare --json
python3 scripts/build.py --unit vc7-d3dx8-jdmarker --compare --json
python3 scripts/build.py --unit vc7-d3dx8-jidctred --compare --json
```

The accepted rows cover matrix primitives, CD3DXBlt pixel-format paths, the
JPEG marker parser, and reduced JPEG IDCTs. Their match units re-extract the
objects and re-audit all declared relocations on every canonical run.

## Relocation-free breadth wave (2026-08-12)

An archive-wide scan matched COFF function sections with zero relocations
against unique complete ledger bodies. Ambiguous symbols, duplicate aliases,
one-byte tails, and every relocated body were excluded from automatic
acceptance. The accepted manifest is `config/vc7-prebuilt-library.csv`.

Thirty-seven pinned D3DX8 members reproduce another 121 functions and 18,710
bytes exactly. They cover substantial libpng 1.0.5, IJG JPEG 6a, zlib 1.1.3,
S3TC, and D3DX image conversion code. `scripts/build.py` materializes the CSV
as ordinary canonical match units, re-extracts the SHA-pinned member, and runs
the same strict target-byte comparator. Reproduce one member or the complete
wave with:

```bash
python3 scripts/build.py --unit vc7-d3dx8-raw-pngrtran --compare --json
for unit in $(python3 scripts/build.py --list | awk -F: '/vc7-d3dx8-raw/{print $1}'); do
  python3 scripts/build.py --unit "$unit" --compare --json || exit
done
```

Together, these first D3DX8 archive waves account for 150 exact functions and
28,388 bytes.

## Relocation-aware archive scan (2026-08-12)

`scripts/scan-vc7-library.py` performs a second fail-closed pass. It requires a
unique target function with identical bytes outside COFF relocation fields,
solves each REL32 destination from the target ledger and each DIR32 destination
from the audited relocation allowlist, then invokes the canonical comparator.
It never edits tracking state; blocked and ambiguous candidates remain separate
in its generated JSON report.

```bash
python3 scripts/scan-vc7-library.py --check
python3 scripts/scan-vc7-library.py --library d3dx8 --json \
  > build/library-scan-d3dx8.json
python3 scripts/scan-vc7-library.py --library libcmt --json \
  > build/library-scan-libcmt.json
```

The accepted manifest is `config/vc7-relocated-library.csv`. Thirty-four
D3DX8 members add 141 functions / 33,653 bytes; sixty-six VC7 LIBCMT members
add 123 functions / 10,403 bytes. Every generated match unit re-extracts the
SHA-pinned archive member and replays all solved relocations through the strict
comparator. The LIBCMT provenance also agrees with the reconstruction compiler
profile's explicit `/MT` selection.

The scanner also proposes initialized-data relocations when the COFF symbol is
in an initialized non-code section, the target address is mapped, and up to 16
bytes at the exact symbol-plus-addend location agree with the target. These are
coordinator-reviewed proposals, never automatic allowlist writes. The first
accepted initialized-data wave unlocks another 89 D3DX8 functions / 30,004
bytes. All 23 affected match units were rebuilt and all 162 functions in those
units remained strict exact after the new allowlist entries were applied.

Across the raw and relocation-aware D3DX8 waves, 380 functions / 92,045 bytes
are now reproducibly exact. Together with 123 LIBCMT functions / 10,403 bytes,
the library total is 503 functions / 102,448 bytes.

Normalized identity is only a candidate filter. It cannot promote a row unless
the final relocation-applied target bytes are independently 100% exact.
