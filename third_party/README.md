# Third-party source

This directory contains redistributable upstream source used by statically
linked code in TH07's DirectX 8 D3DX library. Each versioned directory retains
its upstream license-bearing file and has a `README.th07.md` provenance note.

The files are verified as byte-identical subsets of named official releases by
`python3 scripts/verify-third-party.py`. They are external library source, not
repository-authored game reconstruction. Microsoft D3DX8, VC7 runtime, SDK
headers/libraries, and extracted object files remain local under ignored
`.tools/` or `build/` and are never redistributed here.

D3DX compiled these releases with Microsoft-specific configuration, namespace,
and object partition changes. Any reconstruction adapter therefore belongs in
a separate project-owned directory; do not edit the upstream baseline.
