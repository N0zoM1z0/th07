# Build and exact-matching plan

## Toolchain

The target's PE linker and Rich header identify Visual C++ .NET 2002 (VC7,
build 9466). The TH06 and TH08 reference repositories already contain a
reproducible acquisition/configuration path for that compiler and the DirectX 8
SDK. TH07 will reuse the toolchain design while keeping its own source list,
link order, addresses, globals, and comparison manifests.

## Staged acceptance

1. Verify the exact target and export the address ledger.
2. Recover one small routine and its ABI.
3. Compile the smallest plausible translation unit with VC7.
4. Compare target instructions and relocations at function scope.
5. Add object-level objdiff configuration once original object partitions are
   supported by evidence.
6. Add executable-level reccmp only after source, globals, link order, and PE
   layout are sufficiently complete.

Every comparison is fail-closed. Unresolved calls, absolute data references,
imports, float literals, and string relocations must be mapped to an exact
symbol or reported as blockers. A standalone object can establish exact
function code generation, but cannot by itself prove the original translation
unit or final linked executable.

## Guardrails

Do not use naked assembly, copied target byte arrays, manual padding, fake
types, or ABI lies solely to force a match. Prefer source expressions that the
original programmer could reasonably have written and keep semantics stable
while changing code shape.

