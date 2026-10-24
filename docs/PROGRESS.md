# Reconstruction progress

Generated from the reviewed origin ledger and canonical complete-extent manifest. Byte totals use non-overlapping target function coverage; replay also checks trailing switch tables.

- Authored exact functions: **685 / 686**
- Authored exact bytes: **380,154 / 380,493 (99.9109%)**
- Unaccepted authored functions: **1**; see `config/remaining.csv`.
- Every accepted function now replays from one maintained native linked product.
- Unmodified some100 reference, current oracle: **686 / 694 named functions; 345,019 / 381,170 bytes (90.5158%)**.
- Maintained linked product: **693 / 694 named functions; 380,831 / 381,170 bytes (99.9111%)**.

The two native-build figures include eight SDK/compiler functions. Authored acceptance includes the explicitly user-approved 32-byte x87 primitive bound by `config/source-policy.toml`. Oracle v2 adds independently validated compiler guards, typed constants, bounded string offsets and CRT forwarding identities; the initial v1 reference snapshot is archived under `config/legacy/`. They use a different scope from the authored total above. These are function-byte measurements, not whole-executable or runtime validation.

Linked SDK/CRT code, compiler helpers, DInput data tables, and alignment do not inflate authored progress. Historical runtime exact claims are preserved separately from the current canonical replay.

Reviewed ledger: 1,546 entries; Ghidra seed inventory: 1,573 entries. See [boundary review](BOUNDARY_REVIEW.md) and [strict oracle](ORACLE.md).

| Status | Functions |
| --- | ---: |
| `unclassified` | 0 |
| `identified` | 0 |
| `decompiled` | 0 |
| `implemented` | 0 |
| `compiles` | 1 |
| `matching` | 685 |
| `library` | 860 |
| `blocked` | 0 |
