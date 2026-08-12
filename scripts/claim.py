#!/usr/bin/env python3
"""Coordinator-only claim management for address-bounded reconstruction work."""

from __future__ import annotations

import argparse
import csv
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CLAIMS = ROOT / "config" / "claims.csv"
FUNCTIONS = ROOT / "config" / "functions.csv"
FIELDS = ["address", "owner", "started_utc", "branch", "notes"]


def canonical(value: str) -> str:
    return f"0x{int(value, 0):08X}"


def read_rows() -> list[dict[str, str]]:
    with CLAIMS.open(newline="", encoding="utf-8") as stream:
        reader = csv.DictReader(stream)
        if reader.fieldnames != FIELDS:
            raise ValueError(f"unexpected claims columns: {reader.fieldnames}")
        return list(reader)


def write_rows(rows: list[dict[str, str]]) -> None:
    with CLAIMS.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=FIELDS, lineterminator="\n")
        writer.writeheader()
        writer.writerows(sorted(rows, key=lambda row: int(row["address"], 0)))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    add = subparsers.add_parser("add")
    add.add_argument("addresses", nargs="+")
    add.add_argument("--owner", required=True)
    add.add_argument("--branch", default="main")
    add.add_argument("--notes", default="")
    release = subparsers.add_parser("release")
    release.add_argument("addresses", nargs="+")
    subparsers.add_parser("list")
    args = parser.parse_args()

    rows = read_rows()
    if args.command == "list":
        for row in rows:
            print(f"{row['address']} {row['owner']} {row['branch']} {row['notes']}")
        return 0

    addresses = {canonical(value) for value in args.addresses}
    with FUNCTIONS.open(newline="", encoding="utf-8") as stream:
        ledger = {row["address"] for row in csv.DictReader(stream)}
    missing = sorted(addresses - ledger)
    if missing:
        raise ValueError(f"claim addresses absent from ledger: {', '.join(missing)}")

    existing = {row["address"]: row for row in rows}
    if args.command == "add":
        conflicts = sorted(addresses & existing.keys())
        if conflicts:
            details = ", ".join(f"{address} ({existing[address]['owner']})" for address in conflicts)
            raise ValueError(f"already claimed: {details}")
        started = datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")
        rows.extend(
            {
                "address": address,
                "owner": args.owner,
                "started_utc": started,
                "branch": args.branch,
                "notes": args.notes,
            }
            for address in addresses
        )
    else:
        absent = sorted(addresses - existing.keys())
        if absent:
            raise ValueError(f"cannot release unclaimed addresses: {', '.join(absent)}")
        rows = [row for row in rows if row["address"] not in addresses]
    write_rows(rows)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
