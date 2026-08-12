#!/usr/bin/env python3
"""Recreate an ICO from the exact target's RT_GROUP_ICON and RT_ICON data."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct
import tomllib


ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "resources" / "th07.exe"
TARGET_CONFIG = ROOT / "config" / "target.toml"


class PeResources:
    def __init__(self, path: Path):
        self.data = path.read_bytes()
        pe_offset = struct.unpack_from("<I", self.data, 0x3C)[0]
        if self.data[:2] != b"MZ" or self.data[pe_offset : pe_offset + 4] != b"PE\0\0":
            raise ValueError("target is not a PE image")
        section_count = struct.unpack_from("<H", self.data, pe_offset + 6)[0]
        optional_size = struct.unpack_from("<H", self.data, pe_offset + 20)[0]
        optional = pe_offset + 24
        if struct.unpack_from("<H", self.data, optional)[0] != 0x10B:
            raise ValueError("target is not PE32")
        resource_rva, _ = struct.unpack_from("<II", self.data, optional + 96 + 16)
        section_table = optional + optional_size
        self.sections: list[tuple[int, int, int, int]] = []
        for index in range(section_count):
            offset = section_table + index * 40
            virtual_size, rva, raw_size, raw_offset = struct.unpack_from(
                "<IIII", self.data, offset + 8
            )
            self.sections.append((rva, max(virtual_size, raw_size), raw_offset, raw_size))
        self.resource_base = self.rva_to_offset(resource_rva)

    def rva_to_offset(self, rva: int) -> int:
        for section_rva, mapped_size, raw_offset, raw_size in self.sections:
            delta = rva - section_rva
            if 0 <= delta < mapped_size and delta < raw_size:
                return raw_offset + delta
        raise ValueError(f"RVA 0x{rva:X} is not backed by target bytes")

    def entries(self, relative_offset: int) -> list[tuple[int, bool, int]]:
        offset = self.resource_base + relative_offset
        named, ids = struct.unpack_from("<HH", self.data, offset + 12)
        result = []
        for index in range(named + ids):
            name, child = struct.unpack_from("<II", self.data, offset + 16 + index * 8)
            if name & 0x80000000:
                continue
            result.append((name, bool(child & 0x80000000), child & 0x7FFFFFFF))
        return result

    def resource(self, type_id: int, name_id: int, language: int) -> bytes:
        relative = 0
        for wanted in (type_id, name_id, language):
            choices = {name: (is_dir, child) for name, is_dir, child in self.entries(relative)}
            if wanted not in choices:
                raise ValueError(f"resource path {type_id}/{name_id}/{language:#x} is missing")
            is_dir, relative = choices[wanted]
            if wanted != language and not is_dir:
                raise ValueError("resource tree ended before the language level")
        data_rva, size = struct.unpack_from("<II", self.data, self.resource_base + relative)
        offset = self.rva_to_offset(data_rva)
        return self.data[offset : offset + size]


def target_digest() -> str:
    return hashlib.sha256(TARGET.read_bytes()).hexdigest()


def build_icon() -> tuple[bytes, list[int]]:
    with TARGET_CONFIG.open("rb") as stream:
        expected = tomllib.load(stream)["target"]["sha256"]
    if target_digest() != expected:
        raise ValueError("resources/th07.exe is not the configured exact target")
    pe = PeResources(TARGET)
    group = pe.resource(14, 105, 0x0411)
    reserved, icon_type, count = struct.unpack_from("<HHH", group)
    if reserved != 0 or icon_type != 1 or len(group) != 6 + 14 * count:
        raise ValueError("unexpected RT_GROUP_ICON structure")
    directory = bytearray(struct.pack("<HHH", 0, 1, count))
    images: list[bytes] = []
    ids: list[int] = []
    image_offset = 6 + 16 * count
    for index in range(count):
        width, height, colors, zero, planes, bits, size, icon_id = struct.unpack_from(
            "<BBBBHHIH", group, 6 + index * 14
        )
        image = pe.resource(3, icon_id, 0x0411)
        if zero != 0 or len(image) != size:
            raise ValueError(f"icon {icon_id} metadata does not match its payload")
        directory.extend(
            struct.pack(
                "<BBBBHHII",
                width,
                height,
                colors,
                0,
                planes,
                bits,
                size,
                image_offset,
            )
        )
        images.append(image)
        ids.append(icon_id)
        image_offset += size
    return bytes(directory) + b"".join(images), ids


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if not args.check and args.output is None:
        parser.error("--output is required unless --check is used")
    icon, ids = build_icon()
    if args.output is not None:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(icon)
    if args.check:
        print(f"target icon OK: group 105, RT_ICON ids {ids}, ICO {len(icon)} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
