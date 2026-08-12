# zlib 1.1.3 provenance

- Official archive: `https://zlib.net/fossils/zlib-1.1.3.tar.gz`
- Archive SHA-256: `cae5847bc0e1cf113d3f70d037400da3e47c2e2b7b1c96b0b08447a5fbb906f4`
- Release date: 1998-07-09
- TH07 evidence: the pinned D3DX8 `zutil.obj` records Microsoft's `zlib113`
  build path and the exact `1.1.3` version string.

This directory is an unmodified subset of the official release containing the
inflate-side members present in `d3dx8.lib`. The upstream `README` includes the
zlib license. Version provenance is strong adjacent evidence; only a strict
VC7 target comparison can promote a source-built function to `library`.
