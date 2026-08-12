#!/usr/bin/env python3
"""Export a hash-attested IDA seed inventory and merge durable ledger fields."""

from __future__ import annotations

import argparse
import asyncio
import csv
from datetime import datetime, timezone
import json
from pathlib import Path

from ida_mcp_client import DEFAULT_SERVER, call_json, open_session, parse_int, require_target


ROOT = Path(__file__).resolve().parents[1]
ANALYSIS = ROOT / ".analysis"
OUTPUT = ROOT / "config" / "functions.csv"
FIELDS = [
    "address", "size", "span_end", "current_name", "proposed_name", "module",
    "status", "match_percent", "calling_convention", "signature", "is_thunk",
    "source_file", "evidence", "owner", "notes",
]
MANUAL_FIELDS = {
    "proposed_name", "module", "status", "match_percent", "calling_convention",
    "signature", "source_file", "evidence", "owner", "notes",
}


def canonical(value: object) -> str:
    return f"0x{parse_int(value):08X}"


def existing_rows() -> dict[str, dict[str, str]]:
    if not OUTPUT.exists():
        return {}
    with OUTPUT.open(newline="", encoding="utf-8") as stream:
        return {canonical(row["address"]): row for row in csv.DictReader(stream)}


async def export(server_name: str) -> tuple[dict[str, object], list[dict[str, object]], list[dict[str, object]], object]:
    async with open_session(server_name) as (session, initialized):
        metadata = await require_target(session)
        functions = await call_json(session, "list_functions", {"offset": 0, "count": 0})
        imports = await call_json(session, "list_imports", {"offset": 0, "count": 0})
        entry = await call_json(session, "get_entry_points", {})
        if not isinstance(functions, dict) or not isinstance(functions.get("data"), list):
            raise RuntimeError(f"unexpected function inventory: {functions!r}")
        if not isinstance(imports, dict) or not isinstance(imports.get("data"), list):
            raise RuntimeError(f"unexpected import inventory: {imports!r}")
        provenance = {
            "exported_utc": datetime.now(timezone.utc).isoformat(),
            "protocol_version": initialized.protocolVersion,
            "server": initialized.serverInfo.name,
            "metadata": metadata,
            "boundary_status": "IDA seed; reconcile target instructions before matching",
        }
        return provenance, functions["data"], imports["data"], entry


def write_inventory(provenance: dict[str, object], functions: list[dict[str, object]], imports: list[dict[str, object]], entry: object) -> int:
    ANALYSIS.mkdir(exist_ok=True)
    (ANALYSIS / "ida-functions.json").write_text(
        json.dumps({"provenance": provenance, "functions": functions}, indent=2) + "\n",
        encoding="utf-8",
    )
    (ANALYSIS / "ida-imports.json").write_text(
        json.dumps({"provenance": provenance, "entry_point": entry, "imports": imports}, indent=2) + "\n",
        encoding="utf-8",
    )

    old = existing_rows()
    rows: list[dict[str, str]] = []
    for item in functions:
        address = parse_int(item["address"])
        size = parse_int(item["size"])
        if size <= 0:
            raise RuntimeError(f"invalid function size at {address:#x}: {size}")
        key = canonical(address)
        row = {field: "" for field in FIELDS}
        row.update({
            "address": key,
            "size": str(size),
            "span_end": canonical(address + size - 1),
            "current_name": str(item.get("name", "")),
            "status": "unclassified",
            "match_percent": "0.00",
            "calling_convention": "unknown",
            "is_thunk": "false",
        })
        if key in old:
            for field in MANUAL_FIELDS:
                row[field] = old[key].get(field, row[field])
        rows.append(row)
    rows.sort(key=lambda row: int(row["address"], 16))
    if len({row["address"] for row in rows}) != len(rows):
        raise RuntimeError("IDA inventory contains duplicate function entries")
    with OUTPUT.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=FIELDS, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    print(f"wrote {len(rows)} IDA seed functions to {OUTPUT.relative_to(ROOT)}")
    print(f"wrote {len(imports)} imports and provenance below .analysis/")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--server", default=DEFAULT_SERVER)
    args = parser.parse_args()
    return write_inventory(*asyncio.run(export(args.server)))


if __name__ == "__main__":
    raise SystemExit(main())

