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
5. The two remaining source issues are resolved without masking bytes, padding,
   fake locals, or reclassifying them out of authored scope. Whole-function
   assembly remains prohibited except for the explicitly user-approved 32-byte
   x87 primitive bound by `config/source-policy.toml`.

Current state is 684/686 authored functions and 377,002/380,493 bytes (99.0825%),
all in one product. `verify-linked-product.py` separately reports successful
build/link verification and incomplete exact-source reconstruction. Its
`--require-all-exact` gate deliberately fails while authored work remains.
Runtime playability and whole-file identity have not been established.
