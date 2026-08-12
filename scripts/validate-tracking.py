#!/usr/bin/env python3
"""Validate target identity manifests, function ledger, claims, and names."""

from __future__ import annotations

import csv
from pathlib import Path
import tomllib


ROOT = Path(__file__).resolve().parents[1]
FUNCTIONS = ROOT / "config" / "functions.csv"
FIELDS = [
    "address", "size", "span_end", "current_name", "proposed_name", "module",
    "status", "match_percent", "calling_convention", "signature", "is_thunk",
    "source_file", "evidence", "owner", "notes",
]
STATUSES = {"unclassified", "identified", "decompiled", "implemented", "compiles", "matching", "library", "blocked"}


def fail(message: str) -> None:
    raise ValueError(message)


def main() -> int:
    errors: list[str] = []
    try:
        with (ROOT / "config" / "target.toml").open("rb") as stream:
            target = tomllib.load(stream)
        if len(str(target["target"]["sha256"])) != 64 or len(str(target["target"]["md5"])) != 32:
            fail("target digests have invalid lengths")
        if int(target["target"]["size"]) <= 0:
            fail("target size must be positive")
    except (OSError, KeyError, TypeError, ValueError, tomllib.TOMLDecodeError) as exc:
        errors.append(f"target config: {exc}")

    ledger: dict[str, dict[str, str]] = {}
    try:
        with FUNCTIONS.open(newline="", encoding="utf-8") as stream:
            reader = csv.DictReader(stream)
            if reader.fieldnames != FIELDS:
                fail(f"unexpected function columns: {reader.fieldnames}")
            previous = -1
            previous_end = -1
            for line, row in enumerate(reader, start=2):
                address = int(row["address"], 0)
                size = int(row["size"])
                span_end = int(row["span_end"], 0)
                if row["address"] != f"0x{address:08X}":
                    fail(f"line {line}: noncanonical address {row['address']}")
                if address <= previous:
                    fail(f"line {line}: addresses are duplicate or unsorted")
                if size <= 0 or span_end < address or size > span_end - address + 1:
                    fail(f"line {line}: invalid size/span")
                if address <= previous_end:
                    fail(f"line {line}: function span overlaps the previous entry")
                if row["status"] not in STATUSES:
                    fail(f"line {line}: invalid status {row['status']!r}")
                percent = float(row["match_percent"])
                if not 0.0 <= percent <= 100.0:
                    fail(f"line {line}: invalid match percent")
                if row["status"] == "matching" and (percent != 100.0 or not row["evidence"]):
                    fail(f"line {line}: matching row lacks exact evidence")
                if row["is_thunk"] not in {"true", "false"}:
                    fail(f"line {line}: is_thunk must be true or false")
                ledger[row["address"]] = row
                previous = address
                previous_end = span_end
        if not ledger:
            fail("function ledger is empty")
    except (OSError, ValueError, KeyError) as exc:
        errors.append(f"function ledger: {exc}")

    for filename in ("known-symbols.csv", "known-globals.csv"):
        try:
            seen: set[str] = set()
            with (ROOT / "config" / filename).open(newline="", encoding="utf-8") as stream:
                for line, row in enumerate(csv.DictReader(stream), start=2):
                    address = f"0x{int(row['address'], 0):08X}"
                    if address in seen:
                        fail(f"{filename}:{line}: duplicate address {address}")
                    seen.add(address)
                    if not row["name"] or not row["evidence"]:
                        fail(f"{filename}:{line}: name and evidence are required")
                    if filename == "known-symbols.csv" and ledger and address not in ledger:
                        fail(f"{filename}:{line}: address is absent from ledger")
        except (OSError, ValueError, KeyError) as exc:
            errors.append(str(exc))

    try:
        seen_claims: set[str] = set()
        with (ROOT / "config" / "claims.csv").open(newline="", encoding="utf-8") as stream:
            for line, row in enumerate(csv.DictReader(stream), start=2):
                address = f"0x{int(row['address'], 0):08X}"
                if address in seen_claims or (ledger and address not in ledger):
                    fail(f"claims.csv:{line}: duplicate or unknown address {address}")
                if not row["owner"] or not row["started_utc"]:
                    fail(f"claims.csv:{line}: owner and start time are required")
                seen_claims.add(address)
    except (OSError, ValueError, KeyError) as exc:
        errors.append(str(exc))

    clone_map = ROOT / "config" / "cross-version-clones.csv"
    if clone_map.exists():
        try:
            expected_fields = [
                "target_address", "target_size", "th06_address", "th06_name",
                "th08_address", "th08_name", "name_agreement",
                "normalized_sha256", "exact_bytes", "evidence",
            ]
            seen_targets: set[str] = set()
            with clone_map.open(newline="", encoding="utf-8") as stream:
                reader = csv.DictReader(stream)
                if reader.fieldnames != expected_fields:
                    fail(f"unexpected clone-map columns: {reader.fieldnames}")
                for line, row in enumerate(reader, start=2):
                    address = f"0x{int(row['target_address'], 0):08X}"
                    if address in seen_targets or address not in ledger:
                        fail(f"cross-version-clones.csv:{line}: duplicate or unknown target {address}")
                    seen_targets.add(address)
                    if int(row["target_size"]) != int(ledger[address]["size"]):
                        fail(f"cross-version-clones.csv:{line}: target size mismatch")
                    if row["name_agreement"] not in {"true", "false"} or row["exact_bytes"] not in {"true", "false"}:
                        fail(f"cross-version-clones.csv:{line}: invalid boolean")
                    if len(row["normalized_sha256"]) != 64 or not row["evidence"]:
                        fail(f"cross-version-clones.csv:{line}: missing signature evidence")
        except (OSError, ValueError, KeyError) as exc:
            errors.append(str(exc))

    if errors:
        for error in errors:
            print(f"error: {error}")
        return 1
    print(f"tracking data OK: {len(ledger):,} functions")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
