#!/usr/bin/env python3
"""Capture or read a hash-attested, address-bounded TH07 work packet."""

from __future__ import annotations

import argparse
import asyncio
import csv
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tomllib
from typing import Any

from ida_mcp_client import DEFAULT_SERVER, call_json, open_session, parse_int, require_target


ROOT = Path(__file__).resolve().parents[1]
PACKETS = ROOT / "build" / "work-packets"


def canonical(value: str | int) -> str:
    return f"0x{parse_int(value):08X}"


def file_sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def csv_row(filename: str, address_field: str, address: str) -> dict[str, str] | None:
    path = ROOT / "config" / filename
    if not path.exists():
        return None
    with path.open(newline="", encoding="utf-8") as stream:
        return next(
            (row for row in csv.DictReader(stream) if canonical(row[address_field]) == address),
            None,
        )


def target_identity() -> dict[str, object]:
    with (ROOT / "config" / "target.toml").open("rb") as stream:
        target = tomllib.load(stream)["target"]
    executable = ROOT / "resources" / str(target["filename"])
    actual_size = executable.stat().st_size
    actual_sha256 = file_sha256(executable)
    if actual_size != int(target["size"]) or actual_sha256 != target["sha256"]:
        raise ValueError("local target identity mismatch")
    return {
        "filename": target["filename"],
        "version": target["version"],
        "size": actual_size,
        "sha256": actual_sha256,
    }


def match_entry(address: str) -> tuple[str, dict[str, object], dict[str, object]] | None:
    with (ROOT / "config" / "match-units.toml").open("rb") as stream:
        manifest = tomllib.load(stream)
    for name, unit in manifest["units"].items():
        for function in unit["functions"]:
            if function["address"] == address:
                return name, unit, function
    return None


def comparison_state(address: str) -> dict[str, object]:
    matched = match_entry(address)
    if matched is None:
        return {"state": "not_configured"}
    unit_name, unit, function = matched
    obj = ROOT / str(unit["object"])
    provenance = obj.with_suffix(obj.suffix + ".provenance.json")
    state: dict[str, object] = {
        "state": "not_built" if not obj.exists() else "built",
        "unit": unit_name,
        "object": unit["object"],
        "function": function,
        "provenance": None,
        "comparison": None,
    }
    if not obj.exists() or not provenance.exists():
        return state
    state["provenance"] = json.loads(provenance.read_text(encoding="utf-8"))
    command = [
        sys.executable,
        str(ROOT / "scripts" / "compare-function.py"),
        "--symbol-base",
        str(function["symbol_base"]),
        "--json",
    ]
    for mapping in function.get("rel32_targets", []):
        command.extend(["--rel32-target", str(mapping)])
    for mapping in function.get("dir32_targets", []):
        command.extend(["--dir32-target", str(mapping)])
    command.extend([address, str(obj)])
    completed = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    try:
        state["comparison"] = json.loads(completed.stdout)
    except json.JSONDecodeError:
        state["comparison"] = {
            "result": "error",
            "failure": {"category": "comparison.output_invalid", "message": completed.stderr},
        }
    state["state"] = "compared"
    return state


async def safe_call(session: Any, name: str, arguments: dict[str, object]) -> dict[str, object]:
    try:
        return {"status": "ok", "content": await call_json(session, name, arguments)}
    except Exception as error:  # preserve a local backend failure without discarding siblings
        return {"status": "failed", "error": f"{type(error).__name__}: {error}"}


async def capture(address: str, server: str) -> dict[str, object]:
    async with open_session(server) as (session, initialized):
        before = await require_target(session)
        calls = {
            "function": ("get_function_by_address", {"address": address}),
            "disassembly": ("disassemble_function", {"start_address": address}),
            "decompilation": ("decompile_function", {"address": address}),
            "callers": ("get_callers", {"function_address": address}),
            "callees": ("get_callees", {"function_address": address}),
            "xrefs_to": ("get_xrefs_to", {"address": address}),
        }
        analysis = {
            key: await safe_call(session, tool, arguments)
            for key, (tool, arguments) in calls.items()
        }
        after = await require_target(session)
        if before != after:
            raise RuntimeError("IDA target metadata changed during packet capture")
        return {
            "protocol_version": initialized.protocolVersion,
            "server": initialized.serverInfo.name,
            "target_before": before,
            "target_after": after,
            "analysis": analysis,
        }


def boundary_state(address: str, ledger: dict[str, str], backend: dict[str, object]) -> dict[str, object]:
    observed = backend["analysis"]["function"]  # type: ignore[index]
    if not isinstance(observed, dict) or observed.get("status") != "ok":
        return {"status": "backend_unavailable", "ledger": ledger}
    content = observed.get("content")
    if not isinstance(content, dict):
        return {"status": "backend_unparsed", "ledger": ledger, "backend": content}
    ida_address = canonical(content.get("address", address))
    ida_size = parse_int(content.get("size", 0))
    agrees = ida_address == address and ida_size == int(ledger["size"])
    return {
        "status": "agrees" if agrees else "conflict",
        "ledger_address": address,
        "ledger_size": int(ledger["size"]),
        "ledger_span_end": ledger["span_end"],
        "ida_address": ida_address,
        "ida_size": ida_size,
        "rule": "ledger plus target instruction reconciliation is authoritative",
    }


def create_packet(address: str, server: str) -> dict[str, object]:
    ledger = csv_row("functions.csv", "address", address)
    if ledger is None:
        raise ValueError(f"address is absent from function inventory: {address}")
    backend = asyncio.run(capture(address, server))
    manifests = {}
    for filename in (
        "target.toml",
        "functions.csv",
        "known-symbols.csv",
        "known-globals.csv",
        "cross-version-clones.csv",
        "match-units.toml",
        "reccmp-relocations.csv",
    ):
        path = ROOT / "config" / filename
        if path.exists():
            manifests[filename] = file_sha256(path)
    return {
        "schema_version": 1,
        "captured_utc": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
        "address": address,
        "target": target_identity(),
        "ledger": ledger,
        "known_symbol": csv_row("known-symbols.csv", "address", address),
        "cross_version_candidate": csv_row("cross-version-clones.csv", "target_address", address),
        "boundary": boundary_state(address, ledger, backend),
        "backend": backend,
        "comparison": comparison_state(address),
        "manifest_sha256": manifests,
        "evidence_class": "fresh exact-target semantic packet; comparison status remains independently gated",
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("address")
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--refresh", action="store_true")
    mode.add_argument("--cached", action="store_true")
    parser.add_argument("--server", default=DEFAULT_SERVER)
    args = parser.parse_args()
    try:
        address = canonical(args.address)
        packet_path = PACKETS / f"{address}.json"
        if args.cached:
            if not packet_path.exists():
                raise FileNotFoundError(f"cached packet is missing: {packet_path}")
            packet = json.loads(packet_path.read_text(encoding="utf-8"))
            packet["delivery"] = "cached advisory evidence; do not use alone for status transitions"
        else:
            packet = create_packet(address, args.server)
            PACKETS.mkdir(parents=True, exist_ok=True)
            packet_path.write_text(json.dumps(packet, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
            packet["delivery"] = "fresh coordinator capture"
        print(json.dumps(packet, indent=2, ensure_ascii=False))
        return 0
    except (OSError, RuntimeError, ValueError, KeyError, tomllib.TOMLDecodeError) as error:
        print(json.dumps({"result": "error", "failure": {"message": str(error)}}, indent=2), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
