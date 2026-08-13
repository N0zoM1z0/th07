# Enemy subsystem reconstruction

This note records target-backed structure for the TH07 1.00b enemy update and
render pipeline. It is not a substitute for strict per-function byte
comparison. Names borrowed from TH06 or TH08 remain hypotheses until the TH07
instructions, ABI, and offsets confirm them.

## Exact dependency frontier

The following helpers are already strict VC7 byte matches and form the first
stable layer below the large manager callbacks:

| Address | Role | Size |
| --- | --- | ---: |
| `0x0041F2E0` | allocate and initialize an enemy slot | 327 |
| `0x0041F580` | advance attached enemy effects | 240 |
| `0x0041F670` | release attached enemy effects | 115 |
| `0x0041F6F0` | run one 13-opcode enemy timeline lane | 1599 |
| `0x0041FD70` | handle enemy life thresholds | 516 |
| `0x0041FF80` | handle enemy timer/spell threshold | 847 |
| `0x004202D0` | despawn and unlink an enemy | 210 |
| `0x004203B0` | conditionally clamp enemy position | 213 |
| `0x00420490` | bridge enemy graze and player kill-box collision | 386 |
| `0x004220F0` | interpolate a wrapped angle | 114 |
| `0x00422CA0` | high-priority draw adapter | 23 |
| `0x00422CC0` | low-priority draw adapter | 26 |
| `0x00422CE0` | manager registration callback | 394 |
| `0x00422E70` | manager deletion callback | 193 |
| `0x00422F40` | register update/draw chains | 267 |
| `0x00423050` | cut update/draw chains | 50 |
| `0x004232A0` | test whether an active boss exists | 64 |
| `0x004552D0` | draw the enemy trail triangle fan | 298 |

`0x00420490` is the exact direct player-collision helper. Its inlined timer
`HasTicked` call preserves the target `0x4C` frame and the temporary lifetimes
around both D3DX vector divisions.

## EnemyManager update (`0x00420620`)

Target facts:

- Function extent is `0x00420620..0x004220D7` (6840 bytes), `__thiscall`, with
  a `0x210`-byte stack frame. The receiver is saved at `[ebp-0x1E8]`.
- The manager owns 480 fixed-size enemy slots beginning at `manager+0x4F50`;
  each slot is `0x4F48` bytes.
- The active count is at `manager+0x9545BC`.
- Timeline lanes begin at `manager+0x9545F4`, use a `0x10`-byte stride, and
  precede the enemy-slot pass.
- The update timer is at `manager+0x9546F4`; four per-pass scratch slots begin
  at `manager+0x954700`.
- Eight boss pointers begin at `manager+0x954598`.
- Within an enemy, position begins at `+0x2B0C`, the life-related words are at
  `+0x2BB8/+0x2BBC`, the graze/collision timer begins at `+0x2BC4`, and the
  ECL state/timer tail begins at `+0x4F30`.
- The main pass is ordered as timeline ECL processing, fixed-pool enemy scan,
  per-enemy ECL, movement and clamping, ANM state, collision/damage/death and
  item/effect handling, followed by manager timer/periodic cleanup work.

TH06 provides a useful semantic map for this sequence, but its layouts and
some branches differ. The TH07 target remains authoritative. The dedicated
`src/EnemyManagerUpdate.cpp` unit now compiles a substantial target-backed
implementation of every major phase: timeline lanes, ECL and movement,
follow-target interpolation, trail history and collision, primary/secondary
ANM, damage/rank/spell scaling, target selection, death/items/effects, boss UI,
timers, and the four draw lists. It is not yet a strict byte match.

Typed instruction-shape comparison proved especially useful on this body. It
exposed the ECL manager's first dword as a pointer to timeline metadata, the
signed pause byte at `0x4BFEE0`, and several VC7 expression/bitfield shapes.
Successive source corrections extended the exact instruction-topology prefix
from 8 to more than 400 instructions before later branches diverged. This is a
compiler-shaping diagnostic only and does not grant matching status.

Three large update dependencies are now strict exact matches. `0x0041F6F0` runs one 16-byte
timeline lane with a `0x84` frame. Its 13 target opcodes cover enemy spawning
(including randomized coordinates selected by the `-990.0` sentinel), message
start/wait, boss interrupts, player power changes, and boss-slot waits.
`0x0041FD70` is the `0x10`-frame life callback: it scans four ordered threshold
and ECL-subroutine entries before a 480-slot non-boss clear pass and a `0xD4`
combat-state reset. `0x0041FF80` is the distinct `0x2C`-frame timer callback:
it updates the boss time display, selects the greatest remaining threshold,
enters an ECL subroutine, updates spell timeout/capture and rank state, performs
the same non-boss clear, and resets combat parameters. TH06 corroborates these
roles, but the listed layout and branches are TH07 observations.

## Next implementation cuts

The highest-leverage sequence is:

1. converge the update body's remaining collision/death-tail branches and
   preserve the target `0x210` local allocation;
2. use the same exact-helper-first approach for the render body at
   `0x00422170`.

## EnemyManager render (`0x00422170`)

The target function spans `0x00422170..0x00422C9E` (2863 bytes), has a
`0xFC`-byte frame, and returns with `ret 4`. The two draw adapters prove its
fastcall-shaped inputs: manager in `ECX`, first draw group in `EDX`, and the
exclusive end group on the stack. Each group starts from a head pointer at
`manager+0x954700+group*4` and follows the enemy link at `+0x4F44`.

The first bounded phase updates and draws the primary ANM VM and two adjacent
VM slots of `0x24C` bytes. Active VMs have a nonnegative script index at
`+0x1D8`; world position combines enemy position at `+0x2B0C`, VM offsets at
`+0x230`, and the global render offsets. The primary VM uses Z `0.29`, while
the surrounding slots use Z `0.3`, before calling the rotated ANM draw helper
at `0x0044F9A0`. A later branch temporarily changes primary scale/color for a
trail or triangle-fan effect and then restores them. These target facts define
the next source cut; TH06 only corroborates the phase order.

The reproducible compiler packet is generated with:

```sh
python3 scripts/typed-re.py 0x00420620 --json
```

Generated packets and comparison reports remain under `build/` and are not
committed.
