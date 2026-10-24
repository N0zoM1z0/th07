# Matching evidence and preserved diagnostics

## Recovered in the maintained linked product

- `EnemyManager::Initialize` (`0x0041EA60`): preserve the 1,037-byte body and move
  its definition out of the class into `EnemyManager.cpp`. Both callers keep
  actual calls in the target. Removing the rejected inline candidate eliminates
  52 unused VC7 stack bytes in the constructor (`0x0041EE70`, 256 bytes) and
  `RegisterChain` (`0x00422F40`, 267 bytes). All three complete bodies are exact.
- `GameWindow::Render` (`0x004346E0`, 855 bytes): the performance sample is two
  DWORD fields at EBP-12 and EBP-8. The SDK union instead places it at EBP-16.
  Expressing the observed 8-byte, DWORD-aligned sample preserves the x86 API
  buffer layout and yields complete equality. No spare storage was introduced.
- `BulletManager::AddedCallback` (`0x00426F60`): the template bound uses unsigned
  `ARRAY_SIZE`, matching JAE. The signed helper emitted JGE. Full replay checks
  all 1,723 bytes, including 20 table bytes. The earlier opcode-level note about
  LEA versus ADD was incorrect; disassembly establishes the branch signedness.
- `Ending::ReadEndFileParameter` (`0x0041D410`, 115 bytes): use `atoi`. The original
  friendly symbol CSV called both the 136-byte `atol` implementation and its
  5-byte `atoi` forwarding entry `atol`. Native VC7 symbols distinguish them;
  complete unique thunk replay identifies the latter independently of the
  caller's target operand.
- Static guards: compiler TPI proves the object sizes and uint32 guard type.
  Named `g_LastTime` and `s_MusicRoom` BSS extents plus adjacent generated guard
  contributions independently resolve S3 and S1. Full function replay accepts
  FPS drawing and music-room registration; complete EH graph replay also
  validates the newly resolved music-room handler.
- `AnmManager::DrawInner`: PDB TPI proves `g_ZeroPointFive` is const f32. Its
  unique aligned typed read-only target literal resolves all four fields.
- `ReplayManager::AddedCallbackRecord`: the final two bytes of the six-byte
  version string are copied through a relocation at literal-base+4. A bounded
  offset inside the independently mapped full literal resolves this field.

- `MainMenu::OnUpdatePreInput` (`0x004555DD`, 3,152 coverage / 3,184 full bytes):
  move the loop into `MainMenuPreInput.cpp`, linked as native COFF. Its TU-local
  AnmVm constructor declaration hint prevents speculative inline slots and gives
  the observed `0x64` allocation. Constructor bodies, ABI and layout remain
  shared. Ordinary strcpy calls and separate header/function size profiles
  preserve all existing exact routines and independent diagnostic identities.
  Complete replay includes the 32-byte table; there are no separate supplements.

All four former COFF supplements now pass from the maintained product. The
canonical manifest contains no separate probe supplements.

## Recovered result registration

`ResultScreen::RegisterChain` (`0x0044A302`, 339 bytes) now has the observed
`0x2C` frame. ScoreListNode remains a 12-byte resource-free node; its result grid
is initialized explicitly with the shared C++ stride/count helper before the
existing whole-object clear and cursor initialization. There are no other
score-node default construction sites in the maintained sources.

The native registration TU keeps this helper out of speculative inlining. A
separate native constructor emitter uses the normal inline profile, preserving
all 251 constructor bytes and all 265 deletion bytes. This is one shared class
and initializer implementation, with optimization/emission attributes varying
by TU. No artificial storage or conditional behavioral bodies are involved.
The required constructor COMDAT uses dllexport to force VC7 emission; this adds
one PE export and does not claim whole-image identity.

Splitting the units retains two copies of an EH capsule that LTCG previously
shared. Both complete FuncInfo/unwind graphs, both handlers and both cleanup
bodies independently replay against one unique original graph. Oracle v3
records that alias only after complete comparisons. Extra unmatched descriptors,
changed cleanup homes, unknown callees and ambiguous target graphs are rejected
by synthetic negative controls.

Earlier out-of-line, implicit-inline, packing, destructor, initialization-wrapper
and frame-profile probes are historical diagnostics. The shared initialization
profile above preserves every accepted routine in the final product.

## Build evidence and cache correction

A cold 32-TU rebuild and both links succeeded after the source fixes. The
maintained native measurement is 694/694 named exact functions. With the
explicitly approved 32-byte x87 primitive, authored acceptance is 686/686 and
100%. The original scope remains 380,493 coverage bytes.

VC7 `/GL` objects still need their compiler PDBs during a later link. Deleting
those PDBs before reusing cached objects lost 641 source procedure records in
an experiment. The cache now binds object **and compiler PDB** hashes and
recompiles missing pairs. It also invalidates headers/resources and checks
source, tool, generated-header and command fingerprints. Compiler PDBs needed
for incremental linking remain; obsolete diagnostic artifacts can be removed.

`verify-linked-product.py` checks the actual entry binding, imports, full source
native measurements, unsuppressed link status, faithful/metadata source data,
and independently relocated named globals. Its `--require-all-exact` completion gate passes.

## Approved mathematical primitive

A private VC7 COFF probe compiled four ordinary C++ expressions (direct float,
double, cached sine/cosine and a local receiver) under `/Od /Oi`, `/Os /Oi`,
`/O2 /Oi` and `/Od /Og /Oi`. Optimized variants can be 32 bytes but emit separate
FCOS/FSIN operations; the target uses FSINCOS with a shared x87 stack result.
Unoptimized variants call CRT math helpers. None of these probes establishes
a C++ exact match. On 2026-10-24 the user explicitly approved the already
verified 32-byte FSINCOS/scaled-output primitive. `config/source-policy.toml`
binds the exception to its target, extent and both source bodies. The routine
now replays from the maintained linked product and earns authored exact credit;
all other whole-function assembly substitutions remain prohibited.

## Additional frame diagnostics

Compiler `/FAcs` listings identified the former mismatches as reserved, unused inline
constructor storage. The earlier `ResultScreen::RegisterChain` had 36 unused bytes between
its pointer homes and parameter copy; the target has 24. In
`MainMenu::OnUpdatePreInput`, eight unused bytes precede the `new AnmVm[]`
iterator homes. Constructor visibility changes this reservation, but the
previous global visibility experiment regressed other exact functions.

Normal COFF and native LTCG compilation reproduce the same frame differences.
Stock and patched frontends also agree. Ordinary calling-convention, debug
format, floating-point and inlining switches did not preserve the target body
while correcting both frames. Disabling implicit inlining changes actual calls
and code lengths, so it cannot establish a match. No fabricated storage or
conditional class definition was admitted.

A direct `noinline` annotation on AnmVm reproduced MainMenu's desired frame,
but changed the already exact MainMenu constructor and registration frames.
Marking the base constructor `noinline` additionally changed AnmVm's own body.
These probes confirm the source of the reservation and were rejected as
global fixes. Statement-scoped inline-depth controls also changed the generated
vector-construction helper or registry calls instead of preserving the body.

Further controlled probes and the separate VC7 linear-flow classifier are
recorded in [frame diagnostics](FRAME_DIAGNOSTICS.md). The tested cost switches
did not resolve these frames; the accepted source/TU profiles do. Backend identity is now checked in both load paths and
bound to every native receipt; old receipts require a rebuild.
