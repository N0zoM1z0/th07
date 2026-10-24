# Source tree

This directory contains the maintained game sources directly. The native product uses the pinned
upstream TU list and the three native COFF units in
[`config/native-build.toml`](../config/native-build.toml). Build it with
`python3 scripts/build-whole.py`; verify it with
`python3 scripts/replay-exact-units.py` and
`python3 scripts/verify-linked-product.py --require-all-exact`.

`legacy/` preserves the historical focused probes separately from the current
sources. They remain inputs to [`config/match-units.toml`](../config/match-units.toml)
and are excluded from the maintained product. Keep them for the recorded ABI and
compiler experiments; do not combine both source graphs in one link.

[`docs/ARCHITECTURE.md`](../docs/ARCHITECTURE.md) maps the maintained modules.
[`docs/MATCHING_NOTES.md`](../docs/MATCHING_NOTES.md) explains the recovered
compiler-sensitive cases. Source changes require fresh native replay and a
provenance update; formatting or TU changes can affect VC7 output.

Reconstructed bodies must follow target evidence. Empty behavioral stubs,
speculative TH06/TH08 copies and artificial matching storage remain prohibited.

