#!/usr/bin/env python3
"""Verify vendored upstream subsets against the checked-in SHA-256 manifest."""

from __future__ import annotations

import csv
import hashlib
from pathlib import Path
import tomllib


ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / "config" / "third-party.toml"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    with CONFIG.open("rb") as stream:
        config = tomllib.load(stream)
    if config.get("schema_version") != 1:
        raise ValueError("unsupported third-party manifest schema")
    packages = {item["name"]: item for item in config["packages"]}
    manifest = (ROOT / config["file_manifest"]).resolve()
    if not manifest.is_relative_to(ROOT):
        raise ValueError("third-party file manifest escapes the repository")
    expected: dict[str, dict[str, str]] = {name: {} for name in packages}
    with manifest.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != ["package", "path", "sha256"]:
            raise ValueError("invalid third-party file manifest header")
        for row in reader:
            if row["package"] not in packages or not row["path"]:
                raise ValueError("invalid third-party file row")
            relative = Path(row["path"])
            if relative.is_absolute() or ".." in relative.parts:
                raise ValueError(f"unsafe third-party path: {relative}")
            if row["path"] in expected[row["package"]]:
                raise ValueError(f"duplicate third-party path: {row['path']}")
            expected[row["package"]][row["path"]] = row["sha256"]
    total = 0
    for name, package in packages.items():
        root = (ROOT / package["path"]).resolve()
        if not root.is_relative_to(ROOT / "third_party"):
            raise ValueError(f"package path escapes third_party: {name}")
        project_files = set(package.get("project_files", []))
        actual = {
            str(path.relative_to(root)).replace("\\", "/")
            for path in root.rglob("*")
            if path.is_file()
        }
        permitted = set(expected[name]) | project_files
        if actual != permitted:
            missing = sorted(permitted - actual)
            extra = sorted(actual - permitted)
            raise ValueError(f"{name} file set mismatch; missing={missing}, extra={extra}")
        for relative, digest in expected[name].items():
            if sha256(root / relative) != digest:
                raise ValueError(f"{name}/{relative} differs from the upstream release")
            total += 1
    print(f"third-party source OK: {len(packages)} packages, {total} upstream files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
