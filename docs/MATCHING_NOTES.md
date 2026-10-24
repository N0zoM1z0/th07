# Current matching evidence and next work

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

All four former COFF supplements now pass from the maintained product. The
canonical manifest contains no separate probe supplements.

## Remaining source issues

- `ResultScreen::RegisterChain`, 339 bytes: frame 0x38 versus target 0x2C;
  the affected parameter/temporary homes differ by 12 bytes. Remaining code
  and independently resolved relocations agree.
- `MainMenu::OnUpdatePreInput`, 3,152 coverage / 3,184 full bytes: frame 0x6C
  versus target 0x64. Most later homes shift by eight bytes. Full equality was
  observed when AnmVm's constructor definition was hidden, but applying that
  globally regressed 24 other constructors/callers. That experiment was rejected.
- `Float2`/`Float3::FromAngleMagnitude`, 32 bytes: binary exact but implemented
  entirely in assembly. Keep it unaccepted until admissible source is recovered;
  do not change origin or denominator to remove it.

Other rejected experiments: moving ResultScreen's constructor out of line made
its caller frame 0x14, while explicit inline outside the class preserved the
0x38 mismatch. `noinline`, delayed inline definitions in a separate input TU,
`inline_depth(1)`, separating ResultScreen's pointer declaration from its initialization, and
compiling ResultScreen/MainMenu without `/GL` did not fix
the two frames. A surrounding `pack(4)` did not alter the SDK union's local
placement. These changes were reverted; the native COFF profile list is empty.

## Build evidence and cache correction

A cold 29-TU rebuild and both links succeeded after the source fixes. The
maintained native measurement is 692/694 named exact functions, with the two
frame mismatches above. Authored source additionally withholds the 32-byte
assembly helper, giving 683/686 and 99.0741%.

VC7 `/GL` objects still need their compiler PDBs during a later link. Deleting
those PDBs before reusing cached objects lost 641 source procedure records in
an experiment. The cache now binds object **and compiler PDB** hashes and
recompiles missing pairs. It also invalidates headers/resources and checks
source, tool, generated-header and command fingerprints. Compiler PDBs needed
for incremental linking remain; obsolete diagnostic artifacts can be removed.

`verify-linked-product.py` checks the actual entry binding, imports, full source
native measurements, unsuppressed link status, faithful/metadata source data,
and independently relocated named globals. Its completion gate remains false.

## Mathematical helper probe

A private VC7 COFF probe compiled four ordinary C++ expressions (direct float,
double, cached sine/cosine and a local receiver) under `/Od /Oi`, `/Os /Oi`,
`/O2 /Oi` and `/Od /Og /Oi`. Optimized variants can be 32 bytes but emit separate
FCOS/FSIN operations; the target uses FSINCOS with a shared x87 stack result.
Unoptimized variants call CRT math helpers. None of these probes establishes
exact acceptance. Keep the 32-byte routine in the remaining ledger.
