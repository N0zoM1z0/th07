# Application and runtime boundary review

The historical inventory held 1,527 entries. Ghidra headless independently
inventoried 1,573 entries. Reconciliation with source annotations, complete
native linker procedure extents and target bytes produces 1,546 ledger entries,
including 19 application helpers absent from the historical inventory.
Analysis inventories remain evidence rather than authority over a byte extent.

| Target range (end exclusive) | Classification | Evidence |
| --- | --- | --- |
| `0x00401000`–`0x0046004B` | Source application region | All 694 named procedure starts reconciled with PDB and target |
| Gaps between complete procedures | Alignment | Every gap byte is `90` or `CC` |
| `0x0046004B`–`0x00460050` | Alignment | Five alignment bytes |
| `0x00460050`–`0x00460A90` | DirectInput object-format data | 164 records × 16 bytes |
| `0x00460A90`–`0x00461A90` | DirectInput object-format data | 256 records × 16 bytes |
| `0x00461A90` onward | Linked SDK/CRT region | Boundary closes both DirectInput tables; historical runtime inventory |
| `0x0048C3A0` onward | Compiler EH/startup region | Target/compiler helper inventory |

The 24-byte descriptors at `0x0048D46C` and `0x0048D674` establish the two
DirectInput table pointers, record counts and 16-byte strides. Thus bytes
embedded inside `.text` are classified from structures rather than treated as
missing authored functions. Every Ghidra application start maps to the reviewed
source set. There are no unreviewed application gaps or overlapping complete
accepted procedure extents.

Three SDK inline routines (`D3DXMatrixIdentity`, `D3DXVec3Length`,
`D3DXVec3Cross`) and five compiler deleting wrappers are excluded from authored
scope. The compiler-origin label itself grants no exactness. Historical runtime
exact evidence remains historical until separately replayed; the canonical
source manifest separately includes the eight native exact SDK/compiler entries.

`config/boundary-review.json` records alignment intervals, SDK descriptors,
origin totals and the compiler EH graph audit. `config/function-origins.csv`
provides one explicit origin for every active ledger entry. The reviewed authored
set is a source-function denominator, not a claim to classify every executable
byte as authored code.
