#!/usr/bin/env python3
"""Find relocation-aware TH07 instruction clones in exact TH06/TH08 binaries."""

from __future__ import annotations

import argparse
from collections import defaultdict
import csv
import hashlib
from pathlib import Path
import re
import struct
import sys
from typing import Iterable

try:
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_GRP_CALL, CS_GRP_JUMP
    from capstone.x86 import X86_OP_IMM, X86_OP_MEM, X86_OP_REG
except ImportError as exc:  # pragma: no cover - environment diagnostic
    raise SystemExit("capstone is required: python3 -m pip install capstone") from exc


ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "resources" / "th07.exe"
LEDGER = ROOT / "config" / "functions.csv"
OUTPUT = ROOT / "config" / "cross-version-clones.csv"
REFERENCES = {
    "th06": {
        "binary": Path("/mnt/d/Entertainment/Game/Touhou/th06/th06.exe"),
        "mapping": ROOT / "_references" / "th06" / "config" / "mapping.csv",
        "sha256": "9f76483c46256804792399296619c1274363c31cd8f1775fafb55106fb852245",
    },
    "th08": {
        "binary": Path("/mnt/d/Entertainment/Game/Touhou/[th08] 东方永夜抄 (日文版)/th08.exe"),
        "mapping": ROOT / "_references" / "th08" / "config" / "mapping.csv",
        "sha256": "330fbdbf58a710829d65277b4f312cfbb38d5448b3df523e79350b879213d924",
    },
}
FIELDS = [
    "target_address",
    "target_size",
    "th06_address",
    "th06_name",
    "th08_address",
    "th08_name",
    "name_agreement",
    "normalized_sha256",
    "exact_bytes",
    "evidence",
]


class PeImage:
    def __init__(self, path: Path, expected_sha256: str | None = None):
        self.path = path
        self.data = path.read_bytes()
        digest = hashlib.sha256(self.data).hexdigest()
        if expected_sha256 and digest != expected_sha256:
            raise ValueError(f"unexpected SHA-256 for {path}: {digest}")
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        if self.data[pe : pe + 4] != b"PE\0\0":
            raise ValueError(f"not a PE image: {path}")
        section_count = struct.unpack_from("<H", self.data, pe + 6)[0]
        optional_size = struct.unpack_from("<H", self.data, pe + 20)[0]
        self.image_base = struct.unpack_from("<I", self.data, pe + 52)[0]
        self.image_size = struct.unpack_from("<I", self.data, pe + 80)[0]
        section_table = pe + 24 + optional_size
        self.sections: list[tuple[int, int, int, int]] = []
        for index in range(section_count):
            offset = section_table + 40 * index
            virtual_size, virtual_address, raw_size, raw_pointer = struct.unpack_from(
                "<IIII", self.data, offset + 8
            )
            self.sections.append((virtual_address, virtual_size, raw_pointer, raw_size))

    def read(self, address: int, size: int) -> bytes:
        rva = address - self.image_base
        for virtual_address, virtual_size, raw_pointer, raw_size in self.sections:
            section_size = max(virtual_size, raw_size)
            if virtual_address <= rva and rva + size <= virtual_address + section_size:
                within = rva - virtual_address
                raw_available = max(0, min(size, raw_size - within))
                raw = self.data[raw_pointer + within : raw_pointer + within + raw_available]
                if len(raw) != raw_available:
                    raise ValueError(f"truncated PE range at 0x{address:08X}")
                return raw + bytes(size - raw_available)
        raise ValueError(f"range 0x{address:08X}+{size:#x} is outside {self.path}")

    def contains_address(self, value: int) -> bool:
        value &= 0xFFFFFFFF
        return self.image_base <= value < self.image_base + self.image_size


def canonical(address: int) -> str:
    return f"0x{address:08X}"


def load_ledger() -> list[tuple[int, int, str]]:
    with LEDGER.open(newline="", encoding="utf-8") as stream:
        return [
            (int(row["address"], 0), int(row["size"]), row["current_name"])
            for row in csv.DictReader(stream)
        ]


def load_mapping(path: Path) -> list[tuple[int, int, str]]:
    rows: list[tuple[int, int, str]] = []
    with path.open(newline="", encoding="utf-8") as stream:
        for line, row in enumerate(csv.reader(stream), start=1):
            if len(row) < 3:
                raise ValueError(f"{path}:{line}: expected name,address,size")
            rows.append((int(row[1], 0), int(row[2], 0), row[0]))
    return rows


def operand_token(insn: object, operand: object, start: int, size: int, image: PeImage) -> str:
    if operand.type == X86_OP_REG:
        return f"r{operand.size}:{insn.reg_name(operand.reg)}"
    if operand.type == X86_OP_IMM:
        value = int(operand.imm) & 0xFFFFFFFF
        if CS_GRP_CALL in insn.groups or CS_GRP_JUMP in insn.groups:
            if start <= value < start + size:
                return f"rel{operand.size}:{value - start:#x}"
            return f"rel{operand.size}:external"
        if image.contains_address(value):
            return f"i{operand.size}:image"
        return f"i{operand.size}:{int(operand.imm)}"
    if operand.type == X86_OP_MEM:
        memory = operand.mem
        segment = insn.reg_name(memory.segment) if memory.segment else "-"
        base = insn.reg_name(memory.base) if memory.base else "-"
        index = insn.reg_name(memory.index) if memory.index else "-"
        displacement = int(memory.disp)
        if not memory.base and not memory.index and image.contains_address(displacement):
            disp = "image"
        else:
            disp = str(displacement)
        return f"m{operand.size}:{segment}:{base}:{index}:{memory.scale}:{disp}"
    return f"o{operand.size}:{operand.type}"


def signature(image: PeImage, address: int, body: bytes) -> tuple[str, bool]:
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(body, address))
    decoded = sum(instruction.size for instruction in instructions)
    complete = decoded == len(body)
    tokens = []
    for insn in instructions:
        operands = ",".join(
            operand_token(insn, operand, address, len(body), image)
            for operand in insn.operands
        )
        tokens.append(f"{insn.mnemonic}|{operands}")
    if not complete:
        tokens.append(f"undecoded:{body[decoded:].hex()}")
    return hashlib.sha256("\n".join(tokens).encode()).hexdigest(), complete


def normalized_name(name: str) -> str:
    name = re.sub(r"^th(?:06|08)::", "", name)
    if re.fullmatch(r"FUN_[0-9A-Fa-f]+", name) or name.startswith("sub_"):
        return ""
    return name


def index_functions(image: PeImage, functions: Iterable[tuple[int, int, str]]) -> tuple[dict[tuple[int, str], list[tuple[int, str, bytes]]], int]:
    index: dict[tuple[int, str], list[tuple[int, str, bytes]]] = defaultdict(list)
    incomplete = 0
    for address, size, name in functions:
        try:
            body = image.read(address, size)
            digest, complete = signature(image, address, body)
        except ValueError:
            continue
        if not complete:
            incomplete += 1
        index[(size, digest)].append((address, name, body))
    return index, incomplete


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="fail when the generated CSV is stale")
    args = parser.parse_args()

    target = PeImage(TARGET, "35467eaf8dc7fc85f024f16fb2037255f151cefda33cf4867bc9122aaa2e80ca")
    reference_indices: dict[str, dict[tuple[int, str], list[tuple[int, str, bytes]]]] = {}
    incomplete_counts: dict[str, int] = {}
    for version, paths in REFERENCES.items():
        binary = Path(paths["binary"])
        mapping = Path(paths["mapping"])
        if not binary.is_file() or not mapping.is_file():
            raise SystemExit(f"missing {version} reference binary or mapping")
        image = PeImage(binary, str(paths["sha256"]))
        index, incomplete = index_functions(image, load_mapping(mapping))
        reference_indices[version] = index
        incomplete_counts[version] = incomplete

    rows: list[dict[str, str]] = []
    target_incomplete = 0
    for address, size, _current_name in load_ledger():
        body = target.read(address, size)
        digest, complete = signature(target, address, body)
        if not complete:
            target_incomplete += 1
        matches: dict[str, tuple[int, str, bytes] | None] = {}
        for version in REFERENCES:
            candidates = reference_indices[version].get((size, digest), [])
            matches[version] = candidates[0] if len(candidates) == 1 else None
        if not any(matches.values()):
            continue
        names = {
            version: normalized_name(match[1]) if match else ""
            for version, match in matches.items()
        }
        agreement = bool(names["th06"] and names["th06"] == names["th08"])
        exact = all(match is None or match[2] == body for match in matches.values())
        evidence_parts = [
            f"unique relocation-aware instruction identity with {version.upper()}"
            for version, match in matches.items()
            if match is not None
        ]
        if agreement:
            evidence_parts.append("TH06/TH08 semantic names agree")
        rows.append({
            "target_address": canonical(address),
            "target_size": str(size),
            "th06_address": canonical(matches["th06"][0]) if matches["th06"] else "",
            "th06_name": matches["th06"][1] if matches["th06"] else "",
            "th08_address": canonical(matches["th08"][0]) if matches["th08"] else "",
            "th08_name": matches["th08"][1] if matches["th08"] else "",
            "name_agreement": "true" if agreement else "false",
            "normalized_sha256": digest,
            "exact_bytes": "true" if exact else "false",
            "evidence": "; ".join(evidence_parts),
        })

    from io import StringIO
    buffer = StringIO(newline="")
    writer = csv.DictWriter(buffer, fieldnames=FIELDS, lineterminator="\n")
    writer.writeheader()
    writer.writerows(rows)
    rendered = buffer.getvalue()
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text(encoding="utf-8") != rendered:
            print(f"stale generated clone map: {OUTPUT.relative_to(ROOT)}")
            return 1
        print(f"cross-version clone map is current: {len(rows)} candidates")
        return 0
    OUTPUT.write_text(rendered, encoding="utf-8")
    consensus = sum(row["name_agreement"] == "true" for row in rows)
    exact = sum(row["exact_bytes"] == "true" for row in rows)
    print(f"wrote {len(rows)} unique clone candidates to {OUTPUT.relative_to(ROOT)}")
    print(f"dual-version name agreement: {consensus}; raw exact bytes: {exact}")
    print(f"partial disassemblies: th07={target_incomplete}, th06={incomplete_counts['th06']}, th08={incomplete_counts['th08']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
