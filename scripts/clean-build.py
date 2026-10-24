#!/usr/bin/env python3
"""Remove known disposable diagnostics; preserve native builds and replay evidence."""
from __future__ import annotations

import argparse
from contextlib import nullcontext
import fcntl
import json
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
DIAGNOSTICS = (
    "frame-probes", "compiler-locals", "compiler-trace", "trig-probe",
    "match-units", "goal-result-declaration.json", "main-trial-comparison.json",
    "result-trial-comparison.json", "result-trial-eh-diagnostic.json",
)
PRESERVED = (
    "build/authored/", "build/reference/", "build/authored-comparison.json",
    "build/reference-baseline.json", "build/exact-replay.json",
    "build/link-verification.json", ".analysis/", ".tools/", ".private/",
    "_reference/", "ghidra-project/",
)


def byte_size(path: Path) -> int:
    if path.is_symlink():
        return 0
    if path.is_file():
        return path.stat().st_size
    return sum(p.stat().st_size for p in path.rglob("*")
               if not p.is_symlink() and p.is_file())


def plan(root: Path) -> list[Path]:
    build = root / "build"
    if build.is_symlink():
        raise ValueError("refusing to clean through a symlinked build directory")
    manifest = root / "config" / "exact-units.json"
    units = json.loads(manifest.read_text()).get("units", {}) if manifest.is_file() else {}
    retired_probes = (isinstance(units, dict) and bool(units)
                      and all(isinstance(u, dict) and u.get("kind") == "native-linked"
                              for u in units.values()))
    paths = [build / name for name in DIAGNOSTICS
             if name != "match-units" or retired_probes]
    duplicate = build / "reference-baseline-v3.json"
    canonical = build / "reference-baseline.json"
    if (not duplicate.is_symlink() and not canonical.is_symlink()
            and duplicate.is_file() and canonical.is_file()
            and duplicate.read_bytes() == canonical.read_bytes()):
        paths.append(duplicate)
    return [p for p in paths if p.exists() or p.is_symlink()]


def clean(root: Path = ROOT, *, apply: bool = False) -> dict:
    root = root.resolve()
    # Share the compiler lock so an active diagnostic build cannot be removed.
    lock_file = root.parent / ".th07-vc7-compile.lock"
    with (lock_file.open("a") if apply else nullcontext()) as lock:
        if apply:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        paths = plan(root)
        rows = [{"path": p.relative_to(root).as_posix(), "bytes": byte_size(p)}
                for p in paths]
        if apply:
            for p in paths:
                if p.is_symlink() or p.is_file():
                    p.unlink()
                else:
                    shutil.rmtree(p)
    return {"mode": "applied" if apply else "dry-run", "remove": rows,
            "bytes": sum(r["bytes"] for r in rows), "preserved": list(PRESERVED)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--apply", action="store_true", help="remove listed diagnostics")
    mode.add_argument("--dry-run", action="store_true", help="list only (the default)")
    args = parser.parse_args()
    try:
        result = clean(apply=args.apply)
    except BlockingIOError:
        parser.error("compiler lock is held; wait for the active build to finish")
    except ValueError as error:
        parser.error(str(error))
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
