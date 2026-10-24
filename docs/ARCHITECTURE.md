# Architecture and source layout

The target is original Japanese TH07 1.00b, PE32 i386, image base `0x00400000`,
entry `0x0047EA7D`. `.text` occupies virtual `0x00401000`–`0x0048CD37`
inclusively (572,728 bytes). The original image has stripped relocations.
VC7 compiler 13.00.9466 and LINK 7.0 reproduce the source build profile.

`src/` contains the maintained upstream translation-unit graph: supervisor,
window/input, callback chains, ANM/text/rendering, stage/ECL, enemies, bullets,
items, player, GUI/effects, score/replay, DirectSound/MIDI, menus/results/endings
and PBG4 archive handling. `resources/csv/i18n.csv` generates the Japanese header
inside ignored build directories. Headers retain target layout assertions and
source function/global annotations.

The maintained product has 29 upstream LTCG units plus three native COFF units:
`MainMenuPreInput.cpp`, `ResultScreenCtor.cpp` and `ResultRegister.cpp`. TU-local
optimization hints preserve observed call boundaries and argument-home storage.
The shared C++ bodies and object layouts do not vary across units. The result
constructor emitter adds one PE export to retain its required implementation.
See [frame diagnostics](FRAME_DIAGNOSTICS.md).

The files under `src/legacy/` preserve focused reconstruction
probes and their history. The earlier four standalone supplements now replay
from the maintained native product. The [source guide](../src/README.md)
distinguishes these two graphs; existing probes are not linked twice.

The private pinned upstream checkout retains its original `src/th07/` layout;
its provenance paths describe upstream files. Maintained paths use `src/`.

`config/functions.csv` holds 1,546 reviewed entries. Explicit origins distinguish
authored source, libraries and compiler code. Application closure is reviewed in
[BOUNDARY_REVIEW.md](BOUNDARY_REVIEW.md); complete-extent measurements and source
policy are documented in [ORACLE.md](ORACLE.md).

The 686 authored functions cover 380,493 non-overlapping function bytes. This
source scope includes the game's PBG archive code while retaining its original
LZSS copyright notice. Prebuilt SDK routines and compiler wrappers stay outside
authored progress. Typed reverse-engineering tools and historical cross-version
clone maps remain supporting evidence; only strict target replay grants exact
acceptance.
