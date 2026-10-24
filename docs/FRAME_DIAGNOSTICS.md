# Remaining VC7 frame differences

One routine remains unaccepted: `ResultScreen::RegisterChain`. The menu loop
now passes from the maintained linked product. The table records the original
frame problem before that recovery; diagnostic similarity never grants credit.

| Routine | Complete compared extent | Target allocation | Current allocation |
| --- | ---: | ---: | ---: |
| `0x0044A302 ResultScreen::RegisterChain` | 339 | `0x2C` | `0x38` |
| `0x004555DD MainMenu::OnUpdatePreInput` | 3,184 including 32 table bytes | `0x64` | `0x6C` |

Both target frames were rechecked through the target-pinned typed extractor.
Compiler `/FAcs` listings locate unused storage among generated constructor
locals. RegisterChain has 36 unused bytes between the allocation-pointer homes
and the parameter home; the target has 24. PreInput has two extra dwords before
its array-construction iterator homes. The independently relocated native
comparison rejected both complete extents before the isolated menu recovery.

## Recovered menu loop

`MainMenuPreInput.cpp` is an additional native COFF translation unit linked into
our executable. A TU-local VC7 `noinline` declaration hint keeps the observed
AnmVm constructor call and removes eight speculative inline bytes. The class
layout, calling convention and constructor body are shared; only the compiler
optimization attribute varies for this unit. No constructor body is hidden or
conditionally replaced.

The unit parses common diagnostic stubs with the existing size profile, then
constructor headers with `/Od /Oi`, and applies the menu size profile plus
ordinary strcpy calls to the loop. Otherwise a native size-profile constructor
or a newly folded diagnostic identity changes unrelated measurements. This is
a pinned VC7 build profile. The other 29 units retain the upstream LTCG profile.

A cold 30-TU build, faithful/metadata links and full replay prove all 3,184 menu
bytes, including the 32-byte table. The maintained product now has 693/694 named
exact routines and 685/686 authored exact routines. All accepted implementations
are in that product, with no external COFF supplement. ResultScreen registration
still fails its complete comparison and remains in the denominator.

## Controlled probes

Normal COFF compilation of the complete translation units reproduces the native
LTCG frame differences, so it is useful for inexpensive rejection. COFF sizes
alone, particularly sections containing alignment, cannot establish acceptance.
Every accepted routine must still replay from the maintained linked product.

| Probe family | Result |
| --- | --- |
| Registry constness, pointer constness, calling convention, static/typed forms | Same two frames |
| Qualified global `new`, direct pointer initialization, narrower blocks, const/register locals | Same frames; an explicit array-count local adds real code/storage |
| Function definition order, including registration first and reversed order | Same frames |
| Explicit array member initialization and out-of-class inline timer/vector constructors | Same frames |
| Scoped size/speed/frame optimization settings | No complete fix; some change actual code |
| Global optimizations around constructor/header regions | Can produce PreInput's `0x64`, but alter already exact constructors/callers |
| Selective intrinsic tests and `/Oi` with ordinary `strcpy` calls | Same PreInput frame; other native bodies change |
| Disable implicit inlining, including explicitly marked helper variants | Changes actual calls and extents; rejected |
| C2 `inlS`, `inlT`, inline-depth/cost and `isize` options; EH/local-use toggles | Tested settings do not correct either frame |

A `noinline` AnmVm constructor similarly obtains the desired PreInput allocation
while changing MainMenu's constructor and registration frames. This was rejected as a
global source fix; the accepted menu profile isolates that compiler hint. No conditional constructor body, fabricated type, padding,
extra inert storage or instruction patch was admitted.

## Compiler evidence and next investigation

The stock VC7 backend is unchanged from the pinned tool repository:

- `c2.dll` SHA-256:
  `3755d22dd896ca0045b22b0478d6ad56185d0c98845e2aa2e1d2300460289ad0`.
- At image address `0x10771671`, the backend uses the `inlS`/`inlT` option globals
  in a cost-selection routine.
- A separate classifier at `0x1083446C` identifies linear flow and contains a
  literal 150-node size test (`cmp edi, 0x96` at `0x10834848`).
- The routine at `0x10773C48` has a distinct linear-flow inline path. Bounded
  headless Ghidra exports locate these routines; decompiler variable names and
  inferred prototypes are hypotheses, not recovered compiler source.

These observations, together with unchanged A/B output, make further blind
adjustment of the tested cost switches unproductive. Next investigate the
linear-flow path and the lifetime/import of generated constructor locals.
The current evidence does not prove the internal allocation gate responsible
for every unused slot. No compiler binary was modified.

Both compiler load paths are now checked: the private frontend directory's C2
used by compilation and the stock C2 used by the LTCG linker. Build receipts,
incremental invalidation and native replay require the pinned backend hash.
Legacy receipts lacking that field require a rebuild. Negative controls reject
a changed backend while leaving the compiler driver, frontend and linker intact.
