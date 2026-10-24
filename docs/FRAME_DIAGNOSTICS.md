# Recovered VC7 frame differences

Both routines now replay completely from the maintained linked product. The
table records their former frame differences; diagnostic similarity alone
never grants credit.

| Routine | Complete compared extent | Target allocation | Former allocation |
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

A cold 30-TU build, faithful/metadata links and full replay proved all 3,184 menu
bytes, including the 32-byte table. At that point the product had 693/694 named
and 685/686 authored exact routines. The result registration recovery below
closes the last routine without changing the denominator or using a supplement.

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
adjustment of the tested cost switches unproductive. No compiler binary was
modified.

Both compiler load paths are now checked: the private frontend directory's C2
used by compilation and the stock C2 used by the LTCG linker. Build receipts,
incremental invalidation and native replay require the pinned backend hash.
Legacy receipts lacking that field require a rebuild. Negative controls reject
a changed backend while leaving the compiler driver, frontend and linker intact.

## Runtime compiler tracing

A headless WineDbg GDB proxy and batch GDB traced compilation of the earlier
ResultScreen source without changing either compiler or game instructions.
The trace confirms three imports of the ordinary vector-construction helper
while considering ResultScreen's constructor inside RegisterChain. The first
two helpers construct ANM arrays; the third constructs the score-node array.
The later post-import gate at `0x107727B3` accepts all three vector helpers
but returns zero for the result constructor, after the nested imports. The
final emitted RegisterChain still calls the standalone constructor.

At `0x1076885C`, the initial early-inline budget has a hardcoded minimum of
1000, using twice the function's recorded intermediate-language size above
that minimum. The trace observes a budget of 1000 before importing the result
constructor, 451 before the first vector helper, 406 before the second, and
155 before the score-node helper. This is a separate path from the previously
tested `inlS`/`inlT` cost settings. It does not establish that a supported option
can remove the unwanted storage, or account for each stack slot's lifetime.

Further private COFF probes demonstrate why frame size alone is insufficient:

| Source/profile probe | RegisterChain | Standalone constructor | DeletedCallback |
| --- | --- | --- | --- |
| Original maintained source, EH enabled | 339 bytes, frame `0x38` | 251 bytes, frame `0x38` | Exact in maintained product |
| Empty score-node destructor, EH enabled | 339 bytes, frame `0x2C` | 210 bytes, frame `0x48` | 256 bytes instead of 265 |
| Same destructor, EH disabled | 308 bytes, frame `0x34` | 251 bytes, frame `0x38` | Not recovered |

The destructor changes the generated array-construction/cleanup machinery.
Its desired registration allocation therefore cannot authorize admission:
the complete constructor and deletion behavior must also pass. Nothrow and
forceinline hints, destructor optimization settings, score-grid wrappers,
initialization helpers and alias settings did not produce a complete fix.
None of these probe source changes entered the maintained product. In
particular, no differing class definitions across translation units were used.

## Recovered result registration

The trace also exposes three groups of unnamed argument temporaries in the
registration scope after the speculative array-helper calls. Making score-node
initialization an explicit shared C++ stride/count walk permits a native TU-local
noinline hint on that helper. The registration frame becomes `0x2C`. The node
layout remains 12 bytes and all three links/data fields are initialized before
the existing whole-object clear. No other maintained site default-constructs
score nodes. Both the class and helper bodies are identical across TUs.

`ResultScreenCtor.cpp` emits the real shared constructor with the normal inline
profile; all 251 bytes match. Its VC7 dllexport emission attribute ensures that
the constructor COMDAT exists for registration's call, adding one PE export.
The out-of-line initialization helper is discarded; it adds no callable code
to the final product. Deletion still matches all 265 bytes.

Native registration retains its own complete EH capsule while LTCG retains
the equivalent capsule shared by other registrations. Oracle v3 independently
compares every descriptor/unwind word and every handler/cleanup byte before
recording this unique alias. It does not solve an address from the operand
under comparison. Unexplained descriptor-count changes remain rejected.

A cold 32-TU build, both links, full 694-unit replay and the all-exact product
gate pass: 686/686 authored functions and 380,493/380,493 coverage bytes. All
132 reviewed application globals/vtables also pass. Whole-file identity and
runtime playability remain outside this result.
