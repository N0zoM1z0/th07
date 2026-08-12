#!/usr/bin/env python3
"""Fail-closed identity check for the one supported TH07 executable."""

from __future__ import annotations

import hashlib
from pathlib import Path
import sys
import tomllib


ROOT = Path(__file__).resolve().parents[1]
CONFIG = ROOT / "config" / "target.toml"


def main() -> int:
    with CONFIG.open("rb") as stream:
        expected = tomllib.load(stream)["target"]
    default = ROOT / "resources" / str(expected["filename"])
    path = Path(sys.argv[1]).expanduser().resolve() if len(sys.argv) > 1 else default
    if not path.is_file():
        print(f"missing target: {path}", file=sys.stderr)
        print("copy the original Japanese v1.00b th07.exe to resources/th07.exe", file=sys.stderr)
        return 1

    size = path.stat().st_size
    digest = hashlib.sha256(path.read_bytes()).hexdigest()
    if size != int(expected["size"]) or digest != str(expected["sha256"]).lower():
        print(f"unsupported executable: {path}", file=sys.stderr)
        print(f"  size:   {size} (expected {expected['size']})", file=sys.stderr)
        print(f"  sha256: {digest} (expected {expected['sha256']})", file=sys.stderr)
        return 1

    print(f"target OK: {path}")
    print(f"sha256: {digest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

