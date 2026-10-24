"""Cleanup must leave replay caches and paths outside its allowlist intact."""
import fcntl
import importlib.util
import json
from pathlib import Path
import unittest
from test_headless_oracle import Fixture

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("clean_build", ROOT / "scripts/clean-build.py")
cleaner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cleaner)


class CleanupTests(Fixture):
    def setUp(self):
        super().setUp()
        self.checkout = self.root / "checkout"
        self.checkout.mkdir()
        for path in ["build/frame-probes/probe.obj", "build/authored/a.obj",
                     "build/authored/a.pdb", "build/reference/a.pdb",
                     "build/exact-replay.json", "build/unknown/notes.txt",
                     ".analysis/notes.md"]:
            p = self.checkout / path
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(path.encode())

    def test_default_is_a_nonmutating_preview(self):
        report = cleaner.clean(self.checkout)
        self.assertEqual(report["mode"], "dry-run")
        self.assertEqual([r["path"] for r in report["remove"]], ["build/frame-probes"])
        self.assertTrue((self.checkout / "build/frame-probes/probe.obj").exists())
        self.assertFalse((self.root / ".th07-vc7-compile.lock").exists())

    def test_apply_preserves_native_pairs_evidence_and_unknown_paths(self):
        before = {p: p.read_bytes() for p in self.checkout.rglob("*")
                  if p.is_file() and "frame-probes" not in p.parts}
        cleaner.clean(self.checkout, apply=True)
        self.assertFalse((self.checkout / "build/frame-probes").exists())
        self.assertTrue(all(p.read_bytes() == data for p, data in before.items()))

    def test_diagnostic_symlink_does_not_delete_its_target(self):
        outside = self.root / "external"
        outside.mkdir()
        (outside / "keep").write_text("keep")
        (self.checkout / "build/compiler-trace").symlink_to(outside, target_is_directory=True)
        (self.checkout / "build/frame-probes/external").symlink_to(outside, target_is_directory=True)
        cleaner.clean(self.checkout, apply=True)
        self.assertEqual((outside / "keep").read_text(), "keep")

    def test_symlinked_build_root_is_rejected(self):
        build = self.checkout / "build"
        build.rename(self.checkout / "saved-build")
        build.symlink_to(self.checkout / "saved-build", target_is_directory=True)
        with self.assertRaisesRegex(ValueError, "symlinked"):
            cleaner.clean(self.checkout, apply=True)
        self.assertTrue((build / "frame-probes/probe.obj").exists())

    def test_duplicate_report_requires_identical_canonical_evidence(self):
        duplicate = self.checkout / "build/reference-baseline-v3.json"
        canonical = self.checkout / "build/reference-baseline.json"
        duplicate.write_text("evidence")
        cleaner.clean(self.checkout, apply=True)
        self.assertTrue(duplicate.exists())
        canonical.write_text("different")
        cleaner.clean(self.checkout, apply=True)
        self.assertTrue(duplicate.exists())
        canonical.write_text("evidence")
        cleaner.clean(self.checkout, apply=True)
        self.assertFalse(duplicate.exists())
        self.assertEqual(canonical.read_text(), "evidence")

    def test_active_compiler_prevents_cleanup(self):
        with (self.root / ".th07-vc7-compile.lock").open("a") as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            with self.assertRaises(BlockingIOError):
                cleaner.clean(self.checkout, apply=True)
        self.assertTrue((self.checkout / "build/frame-probes/probe.obj").exists())

    def test_canonical_coff_probe_remains_available_for_replay(self):
        obj = self.checkout / "build/match-units/accepted.obj"
        obj.parent.mkdir()
        obj.write_bytes(b"accepted COFF")
        config = self.checkout / "config"
        config.mkdir()
        (config / "exact-units.json").write_text(json.dumps({
            "units": {"one": {"kind": "legacy-coff"}}
        }))
        cleaner.clean(self.checkout, apply=True)
        self.assertEqual(obj.read_bytes(), b"accepted COFF")

    def test_retired_coff_outputs_can_be_removed_after_native_promotion(self):
        obj = self.checkout / "build/match-units/old.obj"
        obj.parent.mkdir()
        obj.write_bytes(b"retired COFF")
        cleaner.clean(self.checkout, apply=True)
        self.assertTrue(obj.exists())  # No manifest proves retirement yet.
        config = self.checkout / "config"
        config.mkdir()
        (config / "exact-units.json").write_text(json.dumps({
            "units": {"one": {"kind": "native-linked"}}
        }))
        cleaner.clean(self.checkout, apply=True)
        self.assertFalse(obj.exists())


if __name__ == "__main__":
    unittest.main()
