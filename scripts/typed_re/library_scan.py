#!/usr/bin/env python3
"""Propose strict relocation-aware TH07 matches from SHA-pinned VC7 libraries.

The scanner never edits the ledger or match manifest.  It first requires exact
identity outside COFF relocation fields, solves relocation destinations from
the attested target, then replays the repository's canonical COFF comparator.
Only unique, comparator-exact candidates are reported as promotable.
"""

from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import csv
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import struct
import sys
from typing import Any

from . import facts as typed_re


ROOT = Path(__file__).resolve().parents[2]
FUNCTIONS = ROOT / "config" / "functions.csv"
BUILD = ROOT / "build" / "library-scan"
IMAGE_REL_I386_DIR32 = 0x0006
IMAGE_REL_I386_REL32 = 0x0014
IMAGE_SCN_CNT_UNINITIALIZED_DATA = 0x00000080
IMAGE_SCN_LNK_NRELOC_OVFL = 0x01000000


def load_script(module_name: str, filename: str):
    spec = importlib.util.spec_from_file_location(module_name, ROOT / "scripts" / filename)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load repository script: {filename}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    spec.loader.exec_module(module)
    return module


extractor = load_script("th07_vc7_extract", "extract-vc7-library-object.py")
comparator = load_script("th07_compare_function", "compare-function.py")


def canonical(value: int) -> str:
    return f"0x{value:08X}"


def signed32(value: int) -> int:
    return value if value < 1 << 31 else value - (1 << 32)


def safe_member_name(member: str, body: bytes) -> str:
    stem = re.sub(r"[^A-Za-z0-9_.-]+", "_", member.rsplit("/", 1)[-1])
    return f"{hashlib.sha256(body).hexdigest()[:12]}-{stem}"


class CoffObject:
    def __init__(self, data: bytes):
        if len(data) < 20 or struct.unpack_from("<H", data, 0)[0] != 0x014C:
            raise ValueError("not an i386 COFF object")
        self.data = data
        section_count = struct.unpack_from("<H", data, 2)[0]
        self.symbol_pointer, self.symbol_count = struct.unpack_from("<II", data, 8)
        optional_size = struct.unpack_from("<H", data, 16)[0]
        table = 20 + optional_size
        self.string_table = self.symbol_pointer + self.symbol_count * 18
        self.sections: list[dict[str, Any]] = []
        for index in range(section_count):
            offset = table + index * 40
            raw_size, raw_pointer = struct.unpack_from("<II", data, offset + 16)
            self.sections.append(
                {
                    "number": index + 1,
                    "name": data[offset : offset + 8].rstrip(b"\0"),
                    "raw_size": raw_size,
                    "raw_pointer": raw_pointer,
                    "reloc_pointer": struct.unpack_from("<I", data, offset + 24)[0],
                    "reloc_count": struct.unpack_from("<H", data, offset + 32)[0],
                    "characteristics": struct.unpack_from("<I", data, offset + 36)[0],
                }
            )
        self.symbols: dict[int, dict[str, Any]] = {}
        index = 0
        while index < self.symbol_count:
            offset = self.symbol_pointer + index * 18
            aux_count = data[offset + 17]
            symbol = {
                "index": index,
                "name": self._name(offset),
                "value": struct.unpack_from("<I", data, offset + 8)[0],
                "section": struct.unpack_from("<h", data, offset + 12)[0],
                "type": struct.unpack_from("<H", data, offset + 14)[0],
                "storage": data[offset + 16],
                "aux_count": aux_count,
                "total_size": 0,
            }
            if aux_count and (symbol["type"] & 0x20):
                symbol["total_size"] = struct.unpack_from("<I", data, offset + 18 + 4)[0]
            self.symbols[index] = symbol
            index += 1 + aux_count
        self.name_counts = Counter(str(symbol["name"]) for symbol in self.symbols.values())

    def _name(self, offset: int) -> str:
        raw = self.data[offset : offset + 8]
        if raw[:4] == bytes(4):
            start = self.string_table + struct.unpack_from("<I", raw, 4)[0]
            end = self.data.find(b"\0", start)
            if end < 0:
                raise ValueError("unterminated COFF symbol name")
            return self.data[start:end].decode("ascii", errors="replace")
        return raw.rstrip(b"\0").decode("ascii", errors="replace")

    def initialized_sample(self, symbol: dict[str, Any], raw_addend: int) -> bytes | None:
        section_number = int(symbol["section"])
        if not 0 < section_number <= len(self.sections):
            return None
        section = self.sections[section_number - 1]
        if bytes(section["name"]).startswith(b".text") or (
            int(section["characteristics"]) & IMAGE_SCN_CNT_UNINITIALIZED_DATA
        ):
            return None
        addend = signed32(raw_addend)
        value = int(symbol["value"])
        sample_offset = value + addend
        following = [
            int(item["value"])
            for item in self.symbols.values()
            if int(item["section"]) == section_number and int(item["value"]) > value
        ]
        boundary = min(following) if following else int(section["raw_size"])
        length = min(16, boundary - sample_offset)
        if sample_offset < 0 or length < 4 or sample_offset + length > int(section["raw_size"]):
            return None
        start = int(section["raw_pointer"]) + sample_offset
        sample = self.data[start : start + length]
        # Zero-initialized mutable storage is intentionally not inferred from
        # a common byte pattern. Only initialized constants/tables are proposed.
        return sample if any(sample) else None

    def functions(self, minimum_size: int) -> list[dict[str, Any]]:
        by_location: dict[tuple[int, int], list[dict[str, Any]]] = defaultdict(list)
        for symbol in self.symbols.values():
            section_number = int(symbol["section"])
            if not 0 < section_number <= len(self.sections) or not (int(symbol["type"]) & 0x20):
                continue
            section = self.sections[section_number - 1]
            if not bytes(section["name"]).startswith(b".text"):
                continue
            if int(section["characteristics"]) & IMAGE_SCN_LNK_NRELOC_OVFL:
                continue
            value = int(symbol["value"])
            if 0 <= value < int(section["raw_size"]):
                by_location[(section_number, value)].append(symbol)

        locations_by_section: dict[int, list[int]] = defaultdict(list)
        for section_number, value in by_location:
            locations_by_section[section_number].append(value)
        result = []
        for (section_number, value), aliases in by_location.items():
            section = self.sections[section_number - 1]
            following = [item for item in locations_by_section[section_number] if item > value]
            fallback_size = (min(following) if following else int(section["raw_size"])) - value
            sizes = [int(item["total_size"]) for item in aliases if int(item["total_size"]) > 0]
            size = min(sizes) if sizes else fallback_size
            if size < minimum_size or value + size > int(section["raw_size"]):
                continue
            usable = [
                item
                for item in aliases
                if self.name_counts[str(item["name"])] == 1
                and not str(item["name"]).startswith((".", "$"))
            ]
            if not usable:
                continue
            usable.sort(key=lambda item: (int(item["storage"]) != 2, str(item["name"])))
            symbol = usable[0]
            raw = int(section["raw_pointer"]) + value
            code = self.data[raw : raw + size]
            relocations = []
            pointer = int(section["reloc_pointer"])
            for reloc_index in range(int(section["reloc_count"])):
                offset = pointer + reloc_index * 10
                virtual_address, symbol_index, relocation_type = struct.unpack_from(
                    "<IIH", self.data, offset
                )
                field = virtual_address - value
                if not 0 <= field <= size - 4:
                    continue
                target = self.symbols.get(symbol_index)
                if target is None:
                    raise ValueError("relocation references an invalid symbol")
                relocations.append(
                    {
                        "field": field,
                        "type": relocation_type,
                        "target_name": str(target["name"]),
                        "target_section": int(target["section"]),
                        "target_value": int(target["value"]),
                        "raw_addend": struct.unpack_from("<I", code, field)[0],
                    }
                )
                relocations[-1]["object_literal"] = self.initialized_sample(
                    target, int(relocations[-1]["raw_addend"])
                )
            result.append(
                {
                    "symbol": str(symbol["name"]),
                    "section": section_number,
                    "value": value,
                    "size": size,
                    "code": code,
                    "relocations": relocations,
                }
            )
        return result


def normalized(code: bytes, relocations: list[dict[str, Any]]) -> bytes:
    result = bytearray(code)
    occupied: set[int] = set()
    for relocation in relocations:
        field = int(relocation["field"])
        field_bytes = set(range(field, field + 4))
        if occupied & field_bytes:
            raise ValueError("overlapping COFF relocations")
        occupied |= field_bytes
        result[field : field + 4] = bytes(4)
    return bytes(result)


def solve_relocations(
    function: dict[str, Any],
    address: int,
    target: bytes,
    ledger: dict[str, dict[str, str]],
    image: Any,
    library_name: str,
    member: str,
) -> tuple[list[str], list[str], list[str], list[dict[str, str]]]:
    rel32: dict[str, str] = {}
    dir32: dict[str, str] = {}
    blockers: list[str] = []
    proposals: list[dict[str, str]] = []
    allowlist = comparator.load_relocations()
    code = function["code"]
    section_number = int(function["section"])
    for relocation in function["relocations"]:
        field = int(relocation["field"])
        kind = int(relocation["type"])
        name = str(relocation["target_name"])
        target_section = int(relocation["target_section"])
        raw_addend = int(relocation["raw_addend"])
        addend = signed32(raw_addend)
        if kind == IMAGE_REL_I386_REL32:
            if comparator.rel32_operand_kind(code, field) is None:
                blockers.append(f"unsupported REL32 operand {name} at +0x{field:X}")
                continue
            if target_section == section_number:
                continue
            displacement = struct.unpack_from("<i", target, field)[0]
            destination = address + field + 4 + displacement - addend
            key = canonical(destination)
            if key not in ledger:
                blockers.append(f"REL32 {name} resolves outside function ledger: {key}")
                continue
            previous = rel32.setdefault(name, key)
            if previous != key:
                blockers.append(f"REL32 {name} resolves inconsistently")
            continue
        if kind != IMAGE_REL_I386_DIR32:
            blockers.append(f"unsupported relocation type {kind:#x} for {name}")
            continue
        if target_section == section_number or name == "__except_list":
            continue
        target_value = struct.unpack_from("<I", target, field)[0]
        base = (target_value - addend) & 0xFFFFFFFF
        mapped: str | None = None
        if raw_addend == 0 and canonical(base) in ledger:
            mapped = canonical(base)
        else:
            keys = [
                key
                for key, (destination, _literal, addends, _validation) in allowlist.items()
                if destination == base and raw_addend in addends
            ]
            if name in keys:
                mapped = name
            elif len(keys) == 1:
                mapped = keys[0]
            elif len(keys) > 1:
                blockers.append(f"DIR32 {name}+0x{raw_addend:X} has ambiguous allowlist keys")
            else:
                blockers.append(
                    f"DIR32 {name}+0x{raw_addend:X} needs allowlist base {canonical(base)}"
                )
                literal = relocation.get("object_literal")
                if isinstance(literal, bytes):
                    try:
                        target_literal = image.read(target_value, len(literal))
                    except ValueError:
                        target_literal = b""
                    if literal == target_literal:
                        key = f"vc7_auto_{base:08X}_{raw_addend:08X}"
                        proposals.append(
                            {
                                "coff_symbol": key,
                                "address": canonical(base),
                                "data_hex": literal.hex(),
                                "addends": f"0x{raw_addend:X}",
                                "evidence": (
                                    f"SHA-pinned {library_name}/{member} initialized symbol "
                                    f"{name}+0x{raw_addend:X} equals exact target bytes at "
                                    f"{canonical(target_value)}"
                                ),
                                "validation": "literal",
                                "source_symbol": name,
                            }
                        )
        if mapped is not None:
            mapping_name = f"{name}+0x{raw_addend:X}" if raw_addend else name
            previous = dir32.setdefault(mapping_name, mapped)
            if previous != mapped:
                blockers.append(f"DIR32 {mapping_name} resolves inconsistently")
    return (
        [f"{name}={mapped}" for name, mapped in sorted(rel32.items())],
        [f"{name}={mapped}" for name, mapped in sorted(dir32.items())],
        blockers,
        proposals,
    )


def scan(library_name: str, minimum_size: int, include_tracked: bool, address_filter: str | None):
    archive_path, expected_hash = extractor.LIBRARIES[library_name]
    archive = archive_path.read_bytes()
    actual_hash = extractor.sha256(archive)
    if actual_hash != expected_hash:
        raise ValueError(f"{library_name} archive SHA-256 mismatch")
    image, target_config = typed_re.load_target()
    with FUNCTIONS.open(newline="", encoding="utf-8") as stream:
        ledger = {row["address"]: row for row in csv.DictReader(stream)}
    eligible = {
        int(address, 0): row
        for address, row in ledger.items()
        if (include_tracked or row["status"] not in {"matching", "library"})
        and (address_filter is None or address == address_filter)
    }
    targets_by_size: dict[int, list[tuple[int, bytes]]] = defaultdict(list)
    for address, row in eligible.items():
        size = int(row["size"])
        if size >= minimum_size:
            targets_by_size[size].append((address, image.read(address, size)))

    pairs: list[dict[str, Any]] = []
    object_count = function_count = 0
    member_bodies: dict[str, tuple[bytes, Path]] = {}
    for member, body in extractor.archive_members(archive):
        try:
            coff = CoffObject(body)
        except (ValueError, struct.error):
            continue
        object_count += 1
        functions = coff.functions(minimum_size)
        function_count += len(functions)
        for function in functions:
            candidates = targets_by_size.get(int(function["size"]), [])
            if not candidates:
                continue
            try:
                object_normalized = normalized(function["code"], function["relocations"])
            except ValueError:
                continue
            matches = [
                (address, target)
                for address, target in candidates
                if normalized(target, function["relocations"]) == object_normalized
            ]
            if len(matches) != 1:
                continue
            address, target = matches[0]
            rel32, dir32, blockers, proposals = solve_relocations(
                function, address, target, ledger, image, library_name, member
            )
            pairs.append(
                {
                    "library": library_name,
                    "member": member,
                    "address": canonical(address),
                    "size": int(function["size"]),
                    "symbol": function["symbol"],
                    "rel32_targets": rel32,
                    "dir32_targets": dir32,
                    "blockers": blockers,
                    "relocation_proposals": proposals,
                    "body": body,
                    "target": target,
                }
            )

    by_target: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for pair in pairs:
        by_target[pair["address"]].append(pair)
    exact = []
    blocked = []
    ambiguous = []
    relocation_proposals: dict[tuple[str, str], dict[str, str]] = {}
    for address, choices in sorted(by_target.items()):
        unique_locations = {
            (item["member"].lower(), item["symbol"], item["size"]) for item in choices
        }
        if len(unique_locations) != 1:
            ambiguous.append(
                {
                    "address": address,
                    "size": choices[0]["size"],
                    "choices": [
                        {"member": item["member"], "symbol": item["symbol"]} for item in choices
                    ],
                }
            )
            continue
        item = choices[0]
        if item["blockers"]:
            for proposal in item["relocation_proposals"]:
                key = (proposal["address"], proposal["addends"])
                previous = relocation_proposals.get(key)
                if previous is None or len(proposal["data_hex"]) > len(previous["data_hex"]):
                    relocation_proposals[key] = proposal
            blocked.append({key: value for key, value in item.items() if key not in {"body", "target"}})
            continue
        body = item["body"]
        member = item["member"]
        if member not in member_bodies:
            output = BUILD / library_name / safe_member_name(member, body)
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_bytes(body)
            member_bodies[member] = (body, output)
        output = member_bodies[member][1]
        try:
            rel32_mappings = {
                name: int(mapped, 0)
                for name, mapped in map(comparator.parse_mapping, item["rel32_targets"])
            }
            actual = comparator.coff_symbol_bytes(
                output,
                item["symbol"],
                int(address, 0),
                item["size"],
                rel32_mappings,
                dict(comparator.parse_mapping(value) for value in item["dir32_targets"]),
            )[: item["size"]]
        except (OSError, ValueError, struct.error) as error:
            record = {key: value for key, value in item.items() if key not in {"body", "target"}}
            record["blockers"] = [f"canonical comparator: {error}"]
            blocked.append(record)
            continue
        if actual != item["target"] or len(actual) != item["size"]:
            continue
        record = {key: value for key, value in item.items() if key not in {"body", "target", "blockers"}}
        record["unit"] = (
            f"vc7-{library_name}-auto-"
            + re.sub(r"[^a-z0-9]+", "-", Path(member).stem.lower()).strip("-")
        )
        record["target_sha256"] = hashlib.sha256(item["target"]).hexdigest()
        exact.append(record)

    return {
        "schema_version": 1,
        "target_sha256": target_config["sha256"],
        "library": library_name,
        "archive_sha256": actual_hash,
        "minimum_size": minimum_size,
        "objects_scanned": object_count,
        "functions_scanned": function_count,
        "eligible_target_functions": len(eligible),
        "exact_candidates": exact,
        "blocked_candidates": blocked,
        "ambiguous_candidates": ambiguous,
        "relocation_proposals": [
            relocation_proposals[key] for key in sorted(relocation_proposals)
        ],
        "summary": {
            "exact_functions": len(exact),
            "exact_bytes": sum(item["size"] for item in exact),
            "blocked_functions": len(blocked),
            "ambiguous_functions": len(ambiguous),
            "relocation_proposals": len(relocation_proposals),
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", choices=sorted(extractor.LIBRARIES))
    parser.add_argument("--min-size", type=int, default=8)
    parser.add_argument("--include-tracked", action="store_true")
    parser.add_argument("--address")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    if args.min_size < 1:
        parser.error("--min-size must be positive")
    library = args.library
    address = canonical(int(args.address, 0)) if args.address else None
    include_tracked = args.include_tracked
    if args.check:
        library = "d3dx8"
        address = "0x00464C99"
        include_tracked = True
    if library is None:
        parser.error("--library is required unless --check is used")
    try:
        report = scan(library, args.min_size, include_tracked, address)
        if args.check:
            exact = report["exact_candidates"]
            expected_rel32 = "?BltSame_DXTn@CD3DXBlt@@IAEJXZ=0x00463C7D"
            if (
                len(exact) != 1
                or exact[0]["address"] != address
                or expected_rel32 not in exact[0]["rel32_targets"]
            ):
                raise ValueError("scanner regression did not recover relocated CD3DXBlt::BltSame")
            proposal_report = scan(library, args.min_size, True, "0x004637A6")
            if not any(
                item["address"] == "0x004637A6"
                and any(
                    "=vc7_auto_00498B58_00000000" in mapping
                    for mapping in item["dir32_targets"]
                )
                for item in proposal_report["exact_candidates"]
            ):
                raise ValueError("scanner regression did not replay BltTriangle3D initialized data")
            print(
                "VC7 library scanner OK: relocated code and initialized data strict exact"
            )
        elif args.json:
            print(json.dumps(report, indent=2))
        else:
            summary = report["summary"]
            print(
                f"{library}: {summary['exact_functions']} exact / {summary['exact_bytes']} bytes; "
                f"{summary['blocked_functions']} blocked; {summary['ambiguous_functions']} ambiguous"
            )
        return 0
    except (OSError, ValueError, struct.error) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
