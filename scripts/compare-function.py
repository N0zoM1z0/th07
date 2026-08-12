#!/usr/bin/env python3
"""Strictly compare one VC7 COFF function with the attested TH07 image."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
import tomllib


ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "resources" / "th07.exe"
TARGET_CONFIG = ROOT / "config" / "target.toml"
FUNCTIONS = ROOT / "config" / "functions.csv"
RELOCATIONS = ROOT / "config" / "reccmp-relocations.csv"
IMAGE_REL_I386_DIR32 = 0x0006
IMAGE_REL_I386_REL32 = 0x0014
IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080
IMAGE_SCN_LNK_NRELOC_OVFL = 0x01000000


def canonical(value: str) -> str:
    return f"0x{int(value, 0):08X}"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def verify_target() -> str:
    with TARGET_CONFIG.open("rb") as stream:
        expected = tomllib.load(stream)["target"]
    if TARGET.stat().st_size != int(expected["size"]):
        raise ValueError("target size mismatch")
    actual = sha256(TARGET)
    if actual != expected["sha256"]:
        raise ValueError("target SHA-256 mismatch")
    return actual


def target_bytes(address: int, size: int) -> bytes:
    data = TARGET.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    section_count = struct.unpack_from("<H", data, pe + 6)[0]
    optional_size = struct.unpack_from("<H", data, pe + 20)[0]
    section_table = pe + 24 + optional_size
    image_base = struct.unpack_from("<I", data, pe + 52)[0]
    rva = address - image_base
    for index in range(section_count):
        offset = section_table + index * 40
        virtual_size, virtual_address, raw_size, raw_pointer = struct.unpack_from(
            "<IIII", data, offset + 8
        )
        mapped_size = max(virtual_size, raw_size)
        if virtual_address <= rva < virtual_address + mapped_size:
            section_offset = rva - virtual_address
            if section_offset + size > mapped_size:
                raise ValueError("target range crosses a PE section boundary")
            raw_count = max(0, min(size, raw_size - section_offset))
            start = raw_pointer + section_offset
            raw = data[start : start + raw_count]
            if len(raw) != raw_count:
                raise ValueError("PE section raw data is truncated")
            return raw + bytes(size - raw_count)
    raise ValueError("target address is not mapped by a PE section")


def first_mismatch(expected: bytes, actual: bytes, address: int) -> dict[str, object] | None:
    shared = min(len(expected), len(actual))
    offset = next(
        (index for index in range(shared) if expected[index] != actual[index]),
        shared if len(expected) != len(actual) else None,
    )
    if offset is None:
        return None
    return {
        "offset": offset,
        "address": f"0x{address + offset:08X}",
        "target_byte": f"{expected[offset]:02x}" if offset < len(expected) else None,
        "object_byte": f"{actual[offset]:02x}" if offset < len(actual) else None,
        "target_context": expected[offset : offset + 8].hex(" "),
        "object_context": actual[offset : offset + 8].hex(" "),
    }


def rel32_operand_kind(code: bytes | bytearray, field_offset: int) -> str | None:
    if field_offset >= 1 and code[field_offset - 1] in (0xE8, 0xE9):
        return "call" if code[field_offset - 1] == 0xE8 else "jmp"
    if (
        field_offset >= 2
        and code[field_offset - 2] == 0x0F
        and 0x80 <= code[field_offset - 1] <= 0x8F
    ):
        return "jcc"
    return None


def load_relocations() -> dict[str, tuple[int, bytes, frozenset[int], str]]:
    rows: dict[str, tuple[int, bytes, frozenset[int], str]] = {}
    with RELOCATIONS.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            name = row["coff_symbol"]
            if name in rows:
                raise ValueError(f"duplicate relocation allowlist key: {name}")
            literal = bytes.fromhex(row["data_hex"])
            addends = frozenset(
                int(value, 0) for value in row["addends"].split(";") if value
            )
            validation = row.get("validation") or "literal"
            if not literal or not addends or validation not in {"literal", "address"}:
                raise ValueError(f"invalid relocation allowlist row: {name}")
            rows[name] = (int(row["address"], 0), literal, addends, validation)
    return rows


def coff_symbol_bytes(
    path: Path,
    symbol_name: str,
    function_address: int,
    function_size: int,
    rel32_targets: dict[str, int],
    dir32_overrides: dict[str, str],
) -> bytes:
    data = path.read_bytes()
    if len(data) < 20 or struct.unpack_from("<H", data, 0)[0] != 0x014C:
        raise ValueError("object is not an i386 COFF object")
    section_count = struct.unpack_from("<H", data, 2)[0]
    symbol_pointer, symbol_count = struct.unpack_from("<II", data, 8)
    optional_size = struct.unpack_from("<H", data, 16)[0]
    section_table = 20 + optional_size
    string_table = symbol_pointer + symbol_count * 18
    sections: list[dict[str, int | bytes]] = []
    for index in range(section_count):
        offset = section_table + index * 40
        raw_size, raw_pointer = struct.unpack_from("<II", data, offset + 16)
        sections.append(
            {
                "name": data[offset : offset + 8].rstrip(b"\0"),
                "raw_size": raw_size,
                "raw_pointer": raw_pointer,
                "reloc_pointer": struct.unpack_from("<I", data, offset + 24)[0],
                "reloc_count": struct.unpack_from("<H", data, offset + 32)[0],
                "characteristics": struct.unpack_from("<I", data, offset + 36)[0],
            }
        )

    def read_name(offset: int) -> str:
        raw = data[offset : offset + 8]
        if raw[:4] == bytes(4):
            start = string_table + struct.unpack_from("<I", raw, 4)[0]
            end = data.find(b"\0", start)
            if end < 0:
                raise ValueError("unterminated COFF symbol name")
            return data[start:end].decode("ascii", errors="replace")
        return raw.rstrip(b"\0").decode("ascii", errors="replace")

    symbols: dict[int, dict[str, int | str]] = {}
    selected: dict[str, int | str] | None = None
    index = 0
    while index < symbol_count:
        offset = symbol_pointer + index * 18
        symbol = {
            "name": read_name(offset),
            "value": struct.unpack_from("<I", data, offset + 8)[0],
            "section": struct.unpack_from("<h", data, offset + 12)[0],
        }
        symbols[index] = symbol
        if symbol["name"] == symbol_name:
            if selected is not None:
                raise ValueError(f"multiple COFF symbols named {symbol_name}")
            selected = symbol
        index += 1 + data[offset + 17]

    if selected is None:
        raise ValueError(f"COFF symbol is missing: {symbol_name}")
    section_number = int(selected["section"])
    if not 0 < section_number <= len(sections):
        raise ValueError("COFF function symbol has an invalid section")
    section = sections[section_number - 1]
    if not bytes(section["name"]).startswith(b".text"):
        raise ValueError("COFF function symbol is not in a .text section")
    if int(section["characteristics"]) & IMAGE_SCN_LNK_NRELOC_OVFL:
        raise ValueError("COFF relocation-overflow sections are unsupported")

    value = int(selected["value"])
    raw_pointer = int(section["raw_pointer"])
    raw_size = int(section["raw_size"])
    code = bytearray(data[raw_pointer + value : raw_pointer + raw_size])
    allowlist = load_relocations()
    reloc_pointer = int(section["reloc_pointer"])
    for reloc_index in range(int(section["reloc_count"])):
        offset = reloc_pointer + reloc_index * 10
        virtual_address, symbol_index, relocation_type = struct.unpack_from(
            "<IIH", data, offset
        )
        field_offset = virtual_address - value
        if not 0 <= field_offset < function_size:
            continue
        if field_offset > len(code) - 4:
            raise ValueError("COFF relocation extends beyond function section")
        target_symbol = symbols.get(symbol_index)
        if target_symbol is None:
            raise ValueError("COFF relocation references an invalid symbol index")
        target_name = str(target_symbol["name"])
        target_section_number = int(target_symbol["section"])
        raw_addend = struct.unpack_from("<I", code, field_offset)[0]
        signed_addend = raw_addend if raw_addend < 1 << 31 else raw_addend - (1 << 32)

        if relocation_type == IMAGE_REL_I386_REL32:
            if rel32_operand_kind(code, field_offset) is None:
                raise ValueError(f"unsupported REL32 operand for {target_name}")
            if target_section_number == section_number:
                destination = function_address + int(target_symbol["value"]) - value
            elif target_name in rel32_targets:
                destination = rel32_targets[target_name]
            else:
                raise ValueError(f"unknown REL32 target: {target_name}")
            displacement = destination + signed_addend - (function_address + field_offset + 4)
            if not -(1 << 31) <= displacement < 1 << 31:
                raise ValueError("REL32 displacement overflow")
            struct.pack_into("<i", code, field_offset, displacement)
            continue

        if relocation_type != IMAGE_REL_I386_DIR32:
            raise ValueError(f"unsupported COFF relocation type: {relocation_type:#x}")
        if target_section_number == section_number:
            destination = function_address + int(target_symbol["value"]) - value + signed_addend
            struct.pack_into("<I", code, field_offset, destination & 0xFFFFFFFF)
            continue

        key = dir32_overrides.get(f"{target_name}+0x{raw_addend:X}", dir32_overrides.get(target_name, target_name))
        if key not in allowlist:
            raise ValueError(f"unknown DIR32 target: {target_name}")
        destination, literal, allowed_addends, validation = allowlist[key]
        if raw_addend not in allowed_addends:
            raise ValueError(f"unverified DIR32 addend for {target_name}: {raw_addend:#x}")
        if validation == "literal" and 0 < target_section_number <= len(sections):
            target_section = sections[target_section_number - 1]
            object_offset = int(target_symbol["value"]) + signed_addend
            if int(target_section["characteristics"]) & IMAGE_SCN_CNT_UNINITIALIZED_DATA:
                object_literal = bytes(len(literal))
            else:
                start = int(target_section["raw_pointer"]) + object_offset
                object_literal = data[start : start + len(literal)]
            if object_literal != literal:
                raise ValueError(f"COFF literal does not match allowlist for {target_name}")
        if target_bytes(destination + signed_addend, len(literal)) != literal:
            raise ValueError(f"target literal no longer matches allowlist for {target_name}")
        struct.pack_into("<I", code, field_offset, (destination + signed_addend) & 0xFFFFFFFF)
    return bytes(code)


def failure_record(error: Exception) -> tuple[str, dict[str, str]]:
    message = str(error)
    category = "comparison.error"
    if "REL32" in message or "DIR32" in message or "relocation" in message:
        category = "relocation.blocked"
    elif "target" in message and ("mismatch" in message or "size" in message):
        category = "target.identity_mismatch"
    elif isinstance(error, FileNotFoundError):
        category = "coff.file_not_found"
    elif isinstance(error, (ValueError, struct.error)):
        category = "coff.malformed"
    return ("blocked" if category.startswith("relocation.") else "error", {"category": category, "message": message})


def parse_mapping(value: str) -> tuple[str, str]:
    name, separator, mapped = value.partition("=")
    if not separator or not name or not mapped:
        raise ValueError(f"invalid mapping: {value}")
    return name, mapped


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--symbol-base", required=True, help="exact decorated COFF function symbol")
    parser.add_argument("--rel32-target", action="append", default=[], metavar="SYMBOL=ADDRESS")
    parser.add_argument("--dir32-target", action="append", default=[], metavar="SYMBOL=ALLOWLIST_KEY")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("address")
    parser.add_argument("object")
    args = parser.parse_args()

    address = args.address
    report: dict[str, object] = {
        "schema_version": 1,
        "result": "error",
        "address": address,
        "object": str(Path(args.object).resolve()),
        "symbol": args.symbol_base,
        "first_mismatch": None,
        "failure": None,
    }
    try:
        address = canonical(address)
        report["address"] = address
        report["target_executable_sha256"] = verify_target()
        with FUNCTIONS.open(newline="", encoding="utf-8") as stream:
            row = next((item for item in csv.DictReader(stream) if item["address"] == address), None)
        if row is None:
            raise ValueError(f"address is absent from function inventory: {address}")
        size = int(row["size"])
        target = target_bytes(int(address, 0), size)
        rel32_targets = {
            name: int(mapped, 0) for name, mapped in map(parse_mapping, args.rel32_target)
        }
        dir32_targets = dict(map(parse_mapping, args.dir32_target))
        section_tail = coff_symbol_bytes(
            Path(args.object),
            args.symbol_base,
            int(address, 0),
            size,
            rel32_targets,
            dir32_targets,
        )
        actual = section_tail[:size]
        exact = actual == target and len(actual) == size
        report.update(
            {
                "result": "exact" if exact else "mismatch",
                "size": size,
                "object_section_tail_size": len(section_tail),
                "target_sha256": hashlib.sha256(target).hexdigest(),
                "object_compared_sha256": hashlib.sha256(actual).hexdigest(),
                "first_mismatch": first_mismatch(target, actual, int(address, 0)),
            }
        )
        if args.json:
            print(json.dumps(report, indent=2))
        else:
            print(f"{address} {args.symbol_base}: {'exact' if exact else 'mismatch'} ({size} bytes)")
            if report["first_mismatch"]:
                print(json.dumps(report["first_mismatch"], indent=2))
        return 0 if exact else 1
    except (OSError, ValueError, struct.error) as error:
        result, failure = failure_record(error)
        report["result"] = result
        report["failure"] = failure
        if args.json:
            print(json.dumps(report, indent=2))
        else:
            print(f"error: {failure['category']}: {failure['message']}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
