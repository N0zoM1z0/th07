# Architecture and source layout

The target is original Japanese TH07 1.00b, PE32 i386, image base `0x00400000`,
entry `0x0047EA7D`. `.text` occupies virtual `0x00401000`–`0x0048CD37`
inclusively (572,728 bytes). The original image has stripped relocations.
VC7 compiler 13.00.9466 and LINK 7.0 reproduce the source build profile.

`src/th07/` contains the maintained upstream translation-unit graph: supervisor,
window/input, callback chains, ANM/text/rendering, stage/ECL, enemies, bullets,
items, player, GUI/effects, score/replay, DirectSound/MIDI, menus/results/endings
and PBG4 archive handling. `resources/csv/i18n.csv` generates the Japanese header
inside ignored build directories. Headers retain target layout assertions and
source function/global annotations.

The existing `src/*.cpp` and associated headers preserve focused reconstruction
probes and their history. Four independently replayed functions supplement the
native build; they are selected explicitly in `config/exact-units.json`, rather
than silently replacing upstream source bodies or being linked twice.

`config/functions.csv` holds 1,546 reviewed entries. Explicit origins distinguish
authored source, libraries and compiler code. Application closure is reviewed in
[BOUNDARY_REVIEW.md](BOUNDARY_REVIEW.md); complete-extent acceptance and the eight
remaining authored routines are documented in [ORACLE.md](ORACLE.md).

The 686 authored functions cover 380,493 non-overlapping function bytes. This
source scope includes the game's PBG archive code while retaining its original
LZSS copyright notice. Prebuilt SDK routines and compiler wrappers stay outside
authored progress. Typed reverse-engineering tools and historical cross-version
clone maps remain supporting evidence; only strict target replay grants exact
acceptance.
