# IJG JPEG 6a provenance

- Official archive: `https://www.ijg.org/files/jpegsrc.v6a.tar.gz`
- Archive SHA-256: `0e963fdd29d5049f61f3dc137bdc9437473a0ad505dbcfc8925f0fcad46c5929`
- Release string: `6a  7-Feb-96`
- TH07 evidence: the pinned D3DX8 `jerror.obj` contains the same release and
  Thomas G. Lane copyright strings.

This directory is an unmodified subset of the official release containing the
decompression, IDCT, memory-manager, and quantizer members present in
`d3dx8.lib`. The upstream `README` contains its copyright and redistribution
terms. D3DX's missing generated `jconfig.h`, namespace wrapping, optimized IDCT
objects, and object partition are adapter concerns and are not guessed here.
