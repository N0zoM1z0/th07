# Architecture and binary inventory

## Confirmed target facts

The target is the original Japanese TH07 v1.00b executable identified in
`config/target.toml`. These facts are direct PE or IDA observations:

| Property | Value |
| --- | --- |
| Architecture | 32-bit x86 PE GUI executable |
| Image base | `0x00400000` |
| Entry point | `0x0047EA7D` |
| `.text` virtual range | `0x00401000`–`0x0048CD37` |
| `.text` virtual bytes | 572,728 |
| IDA seed inventory | 1,527 functions |
| Linker | 7.0 |
| Compiler family | Visual C++ .NET 2002 (VC7), build 9466 |
| Relocations | stripped |

The current function ledger is seeded from IDA's analyzed entries. Its ranges
must be reconciled per function before comparison because tail chunks, shared
tails, alignment bytes, and missed code can make an IDA extent differ from the
original compiler function.

## Engine relationship

TH07 sits between TH06 and TH08 and retains the same broad engine families:

- supervisor/window/input and callback chains;
- ANM loading, virtual machines, text, and Direct3D 8 rendering;
- stage/background, enemy ECL, bullets, items, player, GUI, and effects;
- PBG archives, replay/score data, sound, MIDI, menus, endings, and results;
- bundled VC7 runtime plus image/compression libraries.

This module list is an adjacent-version hypothesis until TH07 addresses are
confirmed. Exact normalized instruction clones are useful for prioritization,
but target callers, callees, strings, globals, and data layouts decide names.

## Confirmed source lanes

### TextHelper buffer allocation (`0x00431A0F`–`0x00431D3B`)

Six contiguous methods are implemented from target-side IDA decompilation and
disassembly, with TH06/TH08 used as cross-version corroboration:

- `TextHelper` is `0x24` bytes and owns a DIB section, compatible DC, selected
  GDI objects, dimensions, pitch, image size, and Direct3D 8 format;
- the constructor, destructor, release path, fallback allocator, DIB allocator,
  and format lookup are instruction-identical or uniquely normalized against
  one or both adjacent games;
- `g_FormatInfoArray` is a seven-entry table at `0x0049ED98`, observed directly
  in the TH07 image;
- TH07 uses alpha mask `0x00008000` for `D3DFMT_A1R5G5B5`, agreeing with TH08
  and differing from the current TH06 reference source.

These functions remain `implemented` until a VC7 object build and exact
address-bounded comparison promote them to `matching`.

## Planned source layout

Source will be split by original engine responsibility under `src/`, following
the stable TH06/TH08 module names where TH07 evidence agrees. Unknown routines
remain ledger entries rather than being forced into a speculative catch-all
translation unit.
