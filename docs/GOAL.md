# Reconstruction objective and completion evidence

Reach **100% admitted authored exact functions and bytes**, and verify that the
same maintained product builds and links correctly.

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
5. Source issues are resolved without masking bytes, padding,
   fake locals, or reclassifying functions out of authored scope. Whole-function
   assembly remains prohibited except for the explicitly user-approved 32-byte
   x87 primitive bound by `config/source-policy.toml`.

Current state is **686/686 authored functions and 380,493/380,493 bytes (100%)**,
all in one product. A cold 32-TU build, both links, full 694-unit replay and
`verify-linked-product.py --require-all-exact` pass. All 132 reviewed application
globals/vtables also pass. Public CI revalidates the tracking, progress and
synthetic positive/negative controls on each push.

The original denominator and all comparison bytes remain intact. Oracle v3
permits duplicate native EH descriptors only after independent complete graph,
handler and cleanup replay proves one unique original identity; unexplained
count differences and ambiguous identities still fail closed.

Runtime playability and whole-file identity have not been established. The
constructor emission profile adds one PE export and omits original icon resources.
