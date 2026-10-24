# Reconstruction progress

Generated from the reviewed origin ledger and canonical complete-extent manifest. Byte totals use non-overlapping target function coverage; replay also checks trailing switch tables.

- Authored exact functions: **678 / 686**
- Authored exact bytes: **373,295 / 380,493 (98.1082%)**
- Unaccepted authored functions: **8**; see `config/remaining.csv`.
- Four accepted functions use preserved, freshly rebuilt COFF probes; the others use the maintained native LTCG build.
- Unmodified some100 reference: **682 / 694 named functions; 341,585 / 381,170 bytes (89.6149%)**.
- Maintained LTCG product alone: **683 / 694 named functions; 371,049 / 381,170 bytes (97.3448%)**.

The two native-build figures include eight SDK/compiler functions and precede the authored-source policy exclusion. They use a different scope from the authored total above. These are function-byte measurements, not whole-executable or runtime validation.

Linked SDK/CRT code, compiler helpers, DInput data tables, and alignment do not inflate authored progress. Historical runtime exact claims are preserved separately from the current canonical replay.

Reviewed ledger: 1,546 entries; Ghidra seed inventory: 1,573 entries. See [boundary review](BOUNDARY_REVIEW.md) and [strict oracle](ORACLE.md).

| Status | Functions |
| --- | ---: |
| `unclassified` | 0 |
| `identified` | 0 |
| `decompiled` | 0 |
| `implemented` | 0 |
| `compiles` | 8 |
| `matching` | 678 |
| `library` | 860 |
| `blocked` | 0 |
