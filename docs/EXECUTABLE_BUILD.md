# Complete executable build

The end state of this repository is a VC7 build that links a complete TH07
executable from reconstructed game source, legally redistributable upstream
third-party source, and documented SDK/runtime dependencies. Focused function
matching is an intermediate acceptance gate, not the final deliverable.

`config/executable.toml` records the target PE contract separately from linker
inferences and unknowns. `scripts/build-exe.py` validates that contract directly
against the hash-attested target, compiles every currently accepted source
probe, regenerates the Japanese icon resource from the user's own executable,
and refuses the final link until source/library ownership is complete and an
explicit final object list has been reviewed.

```bash
python3 scripts/build-exe.py --check
python3 scripts/build-exe.py --plan --json
python3 scripts/build-exe.py --compile-known -j 2
python3 scripts/build-exe.py --resources
```

The resource extractor writes only below ignored `build/`; the copyrighted icon
is not stored in Git. The committed RC description records the target's numeric
group ID and Japanese language only.

## Confirmed target layout

- PE32 i386, VC7 LINK 7.0, Windows GUI 4.0, image base `0x00400000`;
- section/file alignment `0x1000` / `0x200`, fixed image with stripped relocs;
- `.text`, `.rdata`, `.data`, `.rsrc` in that order;
- `.data` virtual size `0xEC9258` but raw size only `0x3800`, explaining the
  natural `.rsrc` RVA of `0xF66000` once the large global/BSS layout is restored;
- static LIBCMT and D3DX8, with runtime imports from DirectInput 8, DirectSound,
  Direct3D 8, WinMM, Kernel32, User32, GDI32, and Ole32;
- one RT_ICON and one RT_GROUP_ICON resource, language `0x0411`.

## Link gate

`build.link_enabled` is deliberately false. Enabling it requires all of:

1. every target function owned by reconstructed source or an accepted library;
2. reviewed original translation-unit partitions and an explicit object list;
3. complete global/static-object declarations and initialization order;
4. target-backed object/archive order and CRT entry selection;
5. a documented decision for remaining LINK optimization and PE timestamp
   inputs.

No empty bodies or bulk fake stubs are generated to cross this gate. When the
closure is real, `python3 scripts/build-exe.py --link` invokes the pinned VC7
linker and writes `build/exe/th07.exe`.
