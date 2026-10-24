# Active reconstruction objective

Reach **100% admitted authored exact functions and bytes**, and verify that the
same maintained product builds and links correctly. The objective remains active.

Completion requires all of the following:

1. All 686 reviewed authored functions (380,493 coverage bytes) have admissible
   source and independently replay complete exact extents, including tables.
2. All accepted implementations come from the maintained linked product; a
   separate focused probe cannot substitute for the final product.
3. A fresh serial headless VC7 build and both faithful/metadata links succeed
   without suppressing errors. Compiler PDBs, master PDB/image binding and full
   native procedure evidence remain intact.
4. `replay-exact-units.py`, `verify-linked-product.py --require-all-exact`, target
   attestation, tracking/progress checks and public CI all pass on current state.
5. The three remaining source issues are resolved without masking bytes, padding,
   fake locals, reclassifying them out of authored scope, or replacing behavior
   with whole-function assembly.

Current state is 683/686 authored functions and 376,970/380,493 bytes (99.0741%),
all in one product. `verify-linked-product.py` separately reports successful
build/link verification and incomplete exact-source reconstruction. Its
`--require-all-exact` gate deliberately fails while authored work remains.
Runtime playability and whole-file identity have not been established.
