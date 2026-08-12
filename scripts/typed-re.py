#!/usr/bin/env python3
"""Infer typed VC7 reconstruction facts from exact TH07 instructions.

This helper is deliberately below the semantic decompiler layer. It extracts
reproducible stack, width, receiver, constant, call, and compiler-shaping facts
and keeps them separate from source hypotheses. It never edits source, IDA, or
the reconstruction ledger.
"""

from __future__ import annotations

import argparse
from collections import defaultdict
import csv
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tomllib
from typing import Any

try:
    from capstone import Cs, CS_ARCH_X86, CS_MODE_32, CS_AC_READ, CS_AC_WRITE
    from capstone.x86 import (
        X86_OP_IMM,
        X86_OP_MEM,
        X86_OP_REG,
        X86_REG_EBP,
        X86_REG_EBX,
        X86_REG_ECX,
        X86_REG_EDI,
        X86_REG_EDX,
        X86_REG_ESI,
        X86_REG_ESP,
    )
except ImportError as error:  # pragma: no cover - environment diagnostic
    raise SystemExit("capstone is required: python3 -m pip install capstone") from error


ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "resources" / "th07.exe"
TARGET_CONFIG = ROOT / "config" / "target.toml"
FUNCTIONS = ROOT / "config" / "functions.csv"
RELOCATIONS = ROOT / "config" / "reccmp-relocations.csv"
RULES = ROOT / "config" / "typed-re-rules.toml"
MANIFEST = ROOT / "config" / "match-units.toml"


def canonical(value: str | int) -> str:
    return f"0x{int(value, 0) if isinstance(value, str) else value:08X}"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


class PeImage:
    def __init__(self, path: Path):
        self.data = path.read_bytes()
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        if self.data[pe : pe + 4] != b"PE\0\0":
            raise ValueError("target is not a PE image")
        count = struct.unpack_from("<H", self.data, pe + 6)[0]
        optional_size = struct.unpack_from("<H", self.data, pe + 20)[0]
        self.base = struct.unpack_from("<I", self.data, pe + 52)[0]
        self.image_size = struct.unpack_from("<I", self.data, pe + 80)[0]
        table = pe + 24 + optional_size
        self.sections: list[tuple[int, int, int, int]] = []
        for index in range(count):
            offset = table + 40 * index
            virtual_size, virtual_address, raw_size, raw_pointer = struct.unpack_from(
                "<IIII", self.data, offset + 8
            )
            self.sections.append(
                (virtual_address, max(virtual_size, raw_size), raw_pointer, raw_size)
            )

    def contains(self, address: int) -> bool:
        return self.base <= address < self.base + self.image_size

    def read(self, address: int, size: int) -> bytes:
        rva = address - self.base
        for section_rva, mapped_size, raw_pointer, raw_size in self.sections:
            if section_rva <= rva and rva + size <= section_rva + mapped_size:
                within = rva - section_rva
                raw_count = max(0, min(size, raw_size - within))
                raw = self.data[raw_pointer + within : raw_pointer + within + raw_count]
                if len(raw) != raw_count:
                    raise ValueError("target PE section is truncated")
                return raw + bytes(size - raw_count)
        raise ValueError(f"target range {canonical(address)}+{size:#x} is unmapped")


def load_target() -> tuple[PeImage, dict[str, Any]]:
    with TARGET_CONFIG.open("rb") as stream:
        target = tomllib.load(stream)["target"]
    actual = sha256(TARGET)
    if TARGET.stat().st_size != int(target["size"]) or actual != target["sha256"]:
        raise ValueError("target identity mismatch")
    return PeImage(TARGET), target


def ledger_row(address: str) -> dict[str, str]:
    with FUNCTIONS.open(newline="", encoding="utf-8") as stream:
        row = next((item for item in csv.DictReader(stream) if item["address"] == address), None)
    if row is None:
        raise ValueError(f"address is absent from function ledger: {address}")
    return row


def relocation_addresses() -> dict[int, list[dict[str, Any]]]:
    result: dict[int, list[dict[str, Any]]] = defaultdict(list)
    with RELOCATIONS.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream):
            base = int(row["address"], 0)
            addends = [int(value, 0) for value in row["addends"].split(";") if value]
            for addend in addends:
                result[base + addend].append(
                    {
                        "key": row["coff_symbol"],
                        "base": canonical(base),
                        "addend": addend,
                        "validation": row.get("validation") or "literal",
                        "evidence": row["evidence"],
                    }
                )
    return result


def access_name(access: int) -> str:
    names = []
    if access & CS_AC_READ:
        names.append("read")
    if access & CS_AC_WRITE:
        names.append("write")
    return "+".join(names) or "unspecified"


def width_name(size: int) -> str:
    return {1: "byte", 2: "word", 4: "dword", 8: "qword", 10: "tbyte"}.get(
        size, f"{size}-byte"
    )


def strong_type(mnemonic: str, size: int) -> str | None:
    if mnemonic == "movzx":
        return {1: "u8", 2: "u16"}.get(size)
    if mnemonic == "movsx":
        return {1: "i8", 2: "i16"}.get(size)
    if mnemonic == "fild":
        return {2: "i16", 4: "i32", 8: "i64"}.get(size)
    if mnemonic.startswith(("fld", "fst", "fcomp", "fadd", "fsub", "fmul", "fdiv")):
        return {4: "f32", 8: "f64", 10: "long_double"}.get(size)
    return None


def find_compare(address: str) -> tuple[str, dict[str, Any]] | None:
    # Import the build driver lazily so this helper consumes the same expanded
    # manifest (including generated prebuilt units) as canonical comparisons.
    sys.path.insert(0, str(ROOT / "scripts"))
    import build as build_driver  # type: ignore

    manifest = build_driver.load_manifest()
    for name, unit in manifest["units"].items():
        for function in unit["functions"]:
            if function["address"] == address:
                return name, function
    return None


def run_compare(address: str) -> dict[str, Any]:
    matched = find_compare(address)
    if matched is None:
        return {"state": "not_configured"}
    unit, _function = matched
    completed = subprocess.run(
        [sys.executable, str(ROOT / "scripts" / "build.py"), "--unit", unit, "--compare", "--json"],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    try:
        output = json.loads(completed.stdout)
    except json.JSONDecodeError:
        return {
            "state": "error",
            "unit": unit,
            "failure": completed.stderr or completed.stdout,
        }
    report = next(
        (item for item in output.get("comparisons", []) if item.get("address") == address),
        None,
    )
    return {"state": "compared", "unit": unit, "report": report}


def analyze(address: str, compare: bool) -> dict[str, Any]:
    image, target = load_target()
    row = ledger_row(address)
    start = int(address, 0)
    body = image.read(start, int(row["size"]))
    decoder = Cs(CS_ARCH_X86, CS_MODE_32)
    decoder.detail = True
    instructions = list(decoder.disasm(body, start))
    decoded = sum(insn.size for insn in instructions)
    if decoded != len(body):
        raise ValueError(f"instruction decode stops at +{decoded:#x} of {len(body):#x}")

    relocs = relocation_addresses()
    ledger_names: dict[int, str] = {}
    with FUNCTIONS.open(newline="", encoding="utf-8") as stream:
        for item in csv.DictReader(stream):
            ledger_names[int(item["address"], 0)] = item["proposed_name"] or item["current_name"]

    frame_size = 0
    frame_instruction: dict[str, Any] | None = None
    saved_registers: list[str] = []
    register_homes: list[dict[str, Any]] = []
    stack: dict[int, dict[str, Any]] = {}
    image_refs: dict[tuple[int, int, str], dict[str, Any]] = {}
    calls: list[dict[str, Any]] = []
    returns: list[int] = []
    features: set[str] = set()

    for index, insn in enumerate(instructions):
        mnemonic = insn.mnemonic
        if mnemonic in {"movzx", "movsx", "idiv"}:
            features.add(mnemonic)
        if mnemonic == "sub" and len(insn.operands) == 2:
            left, right = insn.operands
            if left.type == X86_OP_REG and left.reg == X86_REG_ESP and right.type == X86_OP_IMM:
                frame_size = int(right.imm)
                frame_instruction = {
                    "address": canonical(insn.address),
                    "size": insn.size,
                    "bytes": bytes(insn.bytes).hex(" "),
                }
        if index < 16 and mnemonic == "push" and insn.operands:
            operand = insn.operands[0]
            if operand.type == X86_OP_REG and operand.reg in {
                X86_REG_EBX,
                X86_REG_ESI,
                X86_REG_EDI,
            }:
                name = insn.reg_name(operand.reg)
                if name not in saved_registers:
                    saved_registers.append(name)
        if index < 20 and mnemonic == "mov" and len(insn.operands) == 2:
            destination, source = insn.operands
            if (
                destination.type == X86_OP_MEM
                and destination.mem.base == X86_REG_EBP
                and destination.mem.index == 0
                and destination.mem.disp < 0
                and source.type == X86_OP_REG
                and source.reg in {X86_REG_ECX, X86_REG_EDX}
            ):
                register_homes.append(
                    {
                        "register": insn.reg_name(source.reg),
                        "stack_offset": destination.mem.disp,
                        "instruction": canonical(insn.address),
                    }
                )
                features.add("register_home")

        for operand_index, operand in enumerate(insn.operands):
            if operand.type == X86_OP_MEM and operand.mem.base == X86_REG_EBP and operand.mem.index == 0:
                displacement = int(operand.mem.disp)
                slot = stack.setdefault(
                    displacement,
                    {
                        "offset": displacement,
                        "region": "local" if displacement < 0 else "argument_or_frame",
                        "widths": set(),
                        "strong_types": set(),
                        "uses": [],
                    },
                )
                slot["widths"].add(width_name(operand.size))
                inferred = strong_type(mnemonic, operand.size)
                if inferred:
                    slot["strong_types"].add(inferred)
                if len(slot["uses"]) < 12:
                    slot["uses"].append(
                        {
                            "address": canonical(insn.address),
                            "mnemonic": mnemonic,
                            "access": access_name(getattr(operand, "access", 0)),
                        }
                    )

            absolute: int | None = None
            kind = ""
            if operand.type == X86_OP_MEM and operand.mem.base == 0 and operand.mem.index == 0:
                absolute = int(operand.mem.disp) & 0xFFFFFFFF
                kind = "absolute_memory"
            elif operand.type == X86_OP_IMM:
                value = int(operand.imm) & 0xFFFFFFFF
                if image.contains(value):
                    absolute = value
                    kind = "image_immediate"
            if absolute is not None and image.contains(absolute):
                role = "image_reference"
                if (
                    mnemonic == "mov"
                    and operand_index == 1
                    and insn.operands[0].type == X86_OP_REG
                    and insn.operands[0].reg == X86_REG_ECX
                ):
                    role = "global_ecx_receiver_candidate"
                    features.add("global_ecx_receiver")
                inferred = strong_type(mnemonic, operand.size)
                if inferred == "f64":
                    features.add("x87_f64")
                key = (absolute, operand.size, kind)
                record = image_refs.setdefault(
                    key,
                    {
                        "address": canonical(absolute),
                        "kind": kind,
                        "role": role,
                        "operand_width": width_name(operand.size),
                        "strong_type": inferred,
                        "allowlist": relocs.get(absolute, []),
                        "uses": [],
                    },
                )
                if len(record["uses"]) < 12:
                    record["uses"].append(
                        {"instruction": canonical(insn.address), "mnemonic": mnemonic}
                    )

        if mnemonic == "call" and insn.operands and insn.operands[0].type == X86_OP_IMM:
            destination = int(insn.operands[0].imm) & 0xFFFFFFFF
            calls.append(
                {
                    "instruction": canonical(insn.address),
                    "destination": canonical(destination),
                    "ledger_name": ledger_names.get(destination),
                }
            )
        if mnemonic == "ret":
            cleanup = int(insn.operands[0].imm) if insn.operands else 0
            returns.append(cleanup)

    if register_homes:
        homes = {item["register"] for item in register_homes}
        calling_convention_hint = "__fastcall" if "edx" in homes else "__thiscall"
    elif any(returns):
        calling_convention_hint = "callee_cleanup (__stdcall candidate)"
    else:
        calling_convention_hint = "caller_cleanup (__cdecl candidate)"

    stack_rows = []
    for displacement in sorted(stack):
        item = stack[displacement]
        item["widths"] = sorted(item["widths"])
        item["strong_types"] = sorted(item["strong_types"])
        stack_rows.append(item)

    comparison = run_compare(address) if compare else {"state": "not_requested"}
    report = comparison.get("report") if isinstance(comparison, dict) else None
    if isinstance(report, dict):
        tail = report.get("object_section_tail_size")
        size = report.get("size")
        if isinstance(tail, int) and isinstance(size, int):
            if tail < size:
                features.add("object_tail_short")
            elif tail > size:
                features.add("object_tail_long")
        mismatch = report.get("first_mismatch")
        if isinstance(mismatch, dict) and frame_instruction:
            mismatch_address = int(str(mismatch["address"]), 0)
            frame_address = int(str(frame_instruction["address"]), 0)
            if frame_address <= mismatch_address < frame_address + int(frame_instruction["size"]):
                features.add("frame_mismatch")

    with RULES.open("rb") as stream:
        rule_file = tomllib.load(stream)
    if rule_file.get("target_sha256") != target["sha256"]:
        raise ValueError("typed reconstruction rules target digest mismatch")
    recommendations = [
        rule
        for rule in rule_file.get("rules", [])
        if set(rule.get("requires", [])).issubset(features)
    ]

    return {
        "schema_version": 1,
        "address": address,
        "target_sha256": target["sha256"],
        "ledger": row,
        "exact_observations": {
            "decoded_bytes": decoded,
            "instruction_count": len(instructions),
            "frame_size": frame_size,
            "frame_instruction": frame_instruction,
            "saved_registers": saved_registers,
            "register_homes": register_homes,
            "stack_accesses": stack_rows,
            "image_references": sorted(image_refs.values(), key=lambda item: item["address"]),
            "direct_calls": calls,
            "return_cleanup_bytes": sorted(set(returns)),
        },
        "inferences": {
            "features": sorted(features),
            "calling_convention_hint": calling_convention_hint,
            "local_order_low_address_to_high": [
                item["offset"] for item in stack_rows if item["region"] == "local"
            ],
            "compiler_recommendations": recommendations,
            "warning": "Inferences guide the next VC7 probe; only canonical 100% comparison changes status.",
        },
        "comparison": comparison,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("address", nargs="?")
    parser.add_argument(
        "--compare",
        action="store_true",
        help="build and compare the owning canonical unit, then classify mismatch shape",
    )
    parser.add_argument("--json", action="store_true", help="retained for command symmetry")
    parser.add_argument(
        "--check",
        action="store_true",
        help="run target-pinned typed-fact regressions without compiling",
    )
    args = parser.parse_args()
    try:
        if args.check:
            aux = analyze("0x0043E0A0", False)
            aux_observed = aux["exact_observations"]
            aux_features = aux["inferences"]["features"]
            score = analyze("0x0043EB90", False)
            score_observed = score["exact_observations"]
            score_features = score["inferences"]["features"]
            failures = []
            if aux_observed["frame_size"] != 0x44:
                failures.append("0x0043E0A0 frame regression")
            if not any(
                item["register"] == "ecx" and item["stack_offset"] == -0x44
                for item in aux_observed["register_homes"]
            ):
                failures.append("0x0043E0A0 ECX-home regression")
            if "x87_f64" not in aux_features:
                failures.append("0x0043E0A0 qword-float regression")
            if score_observed["frame_size"] != 0x48 or "esi" not in score_observed["saved_registers"]:
                failures.append("0x0043EB90 frame/saved-ESI regression")
            if "idiv" not in score_features:
                failures.append("0x0043EB90 idiv regression")
            if failures:
                raise ValueError("; ".join(failures))
            print("typed reconstruction facts OK: 2 target-pinned regressions")
            return 0
        if not args.address:
            parser.error("address is required unless --check is selected")
        print(json.dumps(analyze(canonical(args.address), args.compare), indent=2))
        return 0
    except (OSError, ValueError, KeyError, struct.error, tomllib.TOMLDecodeError) as error:
        print(
            json.dumps(
                {"result": "error", "failure": {"message": str(error)}}, indent=2
            ),
            file=sys.stderr,
        )
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
