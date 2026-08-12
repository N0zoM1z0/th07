#!/usr/bin/env python3
"""Build and strictly compare reproducible focused VC7 match units."""

from __future__ import annotations

import argparse
import csv
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tomllib


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "config" / "match-units.toml"
FUNCTIONS = ROOT / "config" / "functions.csv"
TARGET_CONFIG = ROOT / "config" / "target.toml"
VC7_PREBUILT_SHA256 = {
    "d3dx8": "32148094cebbbe9b55f7769787cdde5926ca01014e072844f5152d25072e1f44",
    "libc": "bd963bdc9388da3452b550886d5a4a35ff300dd66c8f88319daebbbcb5c14eb6",
    "libcmt": "8815af7b9b6e0e28b77708ede25ab7ecfc4b05e1d8811f092c516cff5ce19d94",
}


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def repository_path(value: str, *, output: bool = False) -> Path:
    path = (ROOT / value).resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError(f"path escapes repository: {value}")
    if output and not path.is_relative_to(ROOT / "build"):
        raise ValueError(f"generated object must be under build/: {value}")
    return path


def load_manifest() -> dict[str, object]:
    with MANIFEST.open("rb") as stream:
        manifest = tomllib.load(stream)
    if manifest.get("schema_version") != 1:
        raise ValueError("unsupported match-unit schema")
    with TARGET_CONFIG.open("rb") as stream:
        target_sha256 = tomllib.load(stream)["target"]["sha256"]
    if manifest.get("target_sha256") != target_sha256:
        raise ValueError("match-unit target digest disagrees with config/target.toml")
    units = manifest.get("units")
    if not isinstance(units, dict) or not units:
        raise ValueError("match-unit manifest has no units")
    prebuilt_index = manifest.get("prebuilt_index")
    if prebuilt_index:
        index_path = repository_path(str(prebuilt_index))
        with index_path.open(newline="", encoding="utf-8") as stream:
            reader = csv.DictReader(stream)
            expected = ["unit", "member", "address", "symbol"]
            if reader.fieldnames != expected:
                raise ValueError(f"invalid prebuilt index header: {reader.fieldnames}")
            for row in reader:
                name = row["unit"]
                member = row["member"]
                if not name or not member or not row["symbol"]:
                    raise ValueError("prebuilt index contains an empty required field")
                unit = units.setdefault(
                    name,
                    {
                        "kind": "vc7_prebuilt",
                        "source": "docs/LIBRARY_RECOVERY.md",
                        "object": f"build/match-units/{name.replace('-', '_')}.obj",
                        "profile": "vc7-prebuilt",
                        "toolchain_library": "d3dx8",
                        "toolchain_object": member,
                        "toolchain_archive_sha256": "32148094cebbbe9b55f7769787cdde5926ca01014e072844f5152d25072e1f44",
                        "prebuilt_index": str(prebuilt_index),
                        "notes": "Audited relocation-free raw-exact D3DX8 archive breadth wave.",
                        "functions": [],
                    },
                )
                if unit.get("kind") != "vc7_prebuilt" or unit.get("toolchain_object") != member:
                    raise ValueError(f"prebuilt index unit {name} mixes archive members")
                functions = unit["functions"]
                assert isinstance(functions, list)
                functions.append({"address": row["address"], "symbol_base": row["symbol"]})
    prebuilt_relocated_index = manifest.get("prebuilt_relocated_index")
    if prebuilt_relocated_index:
        index_path = repository_path(str(prebuilt_relocated_index))
        with index_path.open(newline="", encoding="utf-8") as stream:
            reader = csv.DictReader(stream)
            expected = [
                "unit",
                "library",
                "member",
                "address",
                "symbol",
                "rel32_targets",
                "dir32_targets",
            ]
            if reader.fieldnames != expected:
                raise ValueError(f"invalid relocated prebuilt index header: {reader.fieldnames}")
            for row in reader:
                name = row["unit"]
                library = row["library"]
                member = row["member"]
                if (
                    not name
                    or library not in VC7_PREBUILT_SHA256
                    or not member
                    or not row["symbol"]
                ):
                    raise ValueError("relocated prebuilt index has an invalid required field")
                unit = units.setdefault(
                    name,
                    {
                        "kind": "vc7_prebuilt",
                        "source": "docs/LIBRARY_RECOVERY.md",
                        "object": f"build/match-units/{name.replace('-', '_')}.obj",
                        "profile": "vc7-prebuilt",
                        "toolchain_library": library,
                        "toolchain_object": member,
                        "toolchain_archive_sha256": VC7_PREBUILT_SHA256[library],
                        "prebuilt_index": str(prebuilt_relocated_index),
                        "notes": "Unique relocation-aware strict replay from a SHA-pinned VC7 archive.",
                        "functions": [],
                    },
                )
                if (
                    unit.get("kind") != "vc7_prebuilt"
                    or unit.get("toolchain_library") != library
                    or unit.get("toolchain_object") != member
                ):
                    raise ValueError(f"relocated prebuilt unit {name} mixes archives or members")
                function = {"address": row["address"], "symbol_base": row["symbol"]}
                for field in ("rel32_targets", "dir32_targets"):
                    mappings = [value for value in row[field].split(";") if value]
                    if mappings:
                        function[field] = mappings
                functions = unit["functions"]
                assert isinstance(functions, list)
                functions.append(function)
    with FUNCTIONS.open(newline="", encoding="utf-8") as stream:
        ledger = {row["address"] for row in csv.DictReader(stream)}
    claimed: set[str] = set()
    for name, raw_unit in units.items():
        if not isinstance(raw_unit, dict):
            raise ValueError(f"invalid unit: {name}")
        unit = raw_unit
        if unit.get("kind") not in {"probe", "vc7_prebuilt"}:
            raise ValueError(f"unsupported unit kind for {name}: {unit.get('kind')}")
        repository_path(str(unit["source"]))
        repository_path(str(unit["object"]), output=True)
        if unit.get("profile") not in {
            "vc7-text-os",
            "vc7-default",
            "vc7-debug-od",
            "vc7-debug-od-no-gs",
            "vc7-debug-od-no-gs-g6",
            "vc7-debug-od-no-gs-g6-no-op",
            "vc7-size-ob1-no-gs-g6",
            "vc7-prebuilt",
        }:
            raise ValueError(f"unknown compiler profile in unit {name}")
        if unit["kind"] == "vc7_prebuilt":
            if unit["profile"] != "vc7-prebuilt":
                raise ValueError(f"prebuilt unit {name} has the wrong profile")
            library = unit.get("toolchain_library")
            if library not in VC7_PREBUILT_SHA256:
                raise ValueError(f"prebuilt unit {name} has an invalid library")
            member = unit.get("toolchain_object")
            if not isinstance(member, str) or not member.lower().endswith(".obj"):
                raise ValueError(f"prebuilt unit {name} has an invalid object member")
            if unit.get("toolchain_archive_sha256") != VC7_PREBUILT_SHA256[library]:
                raise ValueError(f"prebuilt unit {name} has the wrong archive digest")
        functions = unit.get("functions")
        if not isinstance(functions, list) or not functions:
            raise ValueError(f"unit {name} has no functions")
        for function in functions:
            if not isinstance(function, dict):
                raise ValueError(f"unit {name} has an invalid function row")
            address = f"0x{int(str(function['address']), 0):08X}"
            if address != function["address"] or address not in ledger:
                raise ValueError(f"unit {name} has an unknown/noncanonical address: {address}")
            if address in claimed:
                raise ValueError(f"function appears in multiple match units: {address}")
            claimed.add(address)
            if not function.get("symbol_base"):
                raise ValueError(f"unit {name} function {address} lacks symbol_base")
            for field in ("rel32_targets", "dir32_targets"):
                mappings = function.get(field, [])
                if not isinstance(mappings, list) or any("=" not in str(item) for item in mappings):
                    raise ValueError(f"unit {name} function {address} has invalid {field}")
    return manifest


def compiler_paths() -> list[Path]:
    root = Path(os.environ.get("TH07_VC7_ROOT", ROOT / ".tools" / "vc7"))
    vc7 = root / "PROGRAM FILES" / "MICROSOFT VISUAL STUDIO .NET" / "VC7" / "BIN"
    return [vc7 / "CL.EXE", vc7 / "C1XX.DLL"]


def build_unit(name: str, unit: dict[str, object], json_mode: bool) -> dict[str, object]:
    source = repository_path(str(unit["source"]))
    output = repository_path(str(unit["object"]), output=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    if unit["kind"] == "vc7_prebuilt":
        command = [
            sys.executable,
            str(ROOT / "scripts" / "extract-vc7-library-object.py"),
            "--library",
            str(unit["toolchain_library"]),
            "--object",
            str(unit["toolchain_object"]),
            "--output",
            str(output),
        ]
    else:
        command = [
            str(ROOT / "scripts" / "compile-unit.sh"),
            str(source),
            str(output),
            str(unit["profile"]),
        ]
    completed = subprocess.run(
        command,
        cwd=ROOT,
        capture_output=json_mode,
        text=json_mode,
    )
    if completed.returncode != 0:
        message = completed.stderr.strip() if json_mode else f"compiler exited {completed.returncode}"
        raise RuntimeError(message)
    compiler_hashes = {}
    if unit["kind"] != "vc7_prebuilt":
        for path in compiler_paths():
            if not path.is_file():
                raise ValueError(f"compiler component is missing after build: {path}")
            compiler_hashes[path.name] = file_sha256(path)
    inputs = {
        "source_sha256": file_sha256(source),
        "manifest_sha256": file_sha256(MANIFEST),
        "compile_script_sha256": file_sha256(ROOT / "scripts" / "compile-unit.sh"),
        "runner_sha256": file_sha256(ROOT / "scripts" / "vc7run.bat"),
        "compiler_sha256": compiler_hashes,
        "profile": unit["profile"],
    }
    if unit["kind"] == "vc7_prebuilt":
        inputs.update(
            {
                "toolchain_library": unit["toolchain_library"],
                "toolchain_object": unit["toolchain_object"],
                "toolchain_archive_sha256": unit["toolchain_archive_sha256"],
                "extractor_sha256": file_sha256(
                    ROOT / "scripts" / "extract-vc7-library-object.py"
                ),
            }
        )
        if unit.get("prebuilt_index"):
            inputs["prebuilt_index_sha256"] = file_sha256(
                repository_path(str(unit["prebuilt_index"]))
            )
    input_digest = hashlib.sha256(
        json.dumps(inputs, sort_keys=True, separators=(",", ":")).encode()
    ).hexdigest()
    provenance = {
        "schema_version": 1,
        "target_sha256": load_manifest()["target_sha256"],
        "unit": name,
        "kind": unit["kind"],
        "source": str(source.relative_to(ROOT)),
        "object": str(output.relative_to(ROOT)),
        "object_sha256": file_sha256(output),
        "input_digest": input_digest,
        "inputs": inputs,
        "command": command,
        "built_utc": datetime.now(timezone.utc).isoformat().replace("+00:00", "Z"),
    }
    if unit["kind"] == "vc7_prebuilt":
        provenance.update(
            {
                "toolchain_library": unit["toolchain_library"],
                "toolchain_object": unit["toolchain_object"],
                "toolchain_archive_sha256": unit["toolchain_archive_sha256"],
            }
        )
    provenance_path = output.with_suffix(output.suffix + ".provenance.json")
    provenance_path.write_text(json.dumps(provenance, indent=2) + "\n", encoding="utf-8")
    return provenance


def compare_unit(unit: dict[str, object], json_mode: bool) -> tuple[int, list[dict[str, object]]]:
    output = repository_path(str(unit["object"]), output=True)
    reports: list[dict[str, object]] = []
    return_code = 0
    functions = unit["functions"]
    assert isinstance(functions, list)
    for function in functions:
        assert isinstance(function, dict)
        command = [
            sys.executable,
            str(ROOT / "scripts" / "compare-function.py"),
            "--symbol-base",
            str(function["symbol_base"]),
        ]
        for mapping in function.get("rel32_targets", []):
            command.extend(["--rel32-target", str(mapping)])
        for mapping in function.get("dir32_targets", []):
            command.extend(["--dir32-target", str(mapping)])
        if json_mode:
            command.append("--json")
        command.extend([str(function["address"]), str(output)])
        completed = subprocess.run(command, cwd=ROOT, capture_output=json_mode, text=json_mode)
        return_code = max(return_code, completed.returncode)
        if json_mode:
            try:
                reports.append(json.loads(completed.stdout))
            except json.JSONDecodeError:
                reports.append(
                    {
                        "result": "error",
                        "failure": {
                            "category": "comparison.output_invalid",
                            "message": completed.stderr or completed.stdout,
                        },
                    }
                )
                return_code = 1
    return return_code, reports


def find_unit(selector: str, manifest: dict[str, object]) -> tuple[str, dict[str, object]]:
    units = manifest["units"]
    assert isinstance(units, dict)
    if selector in units:
        unit = units[selector]
        assert isinstance(unit, dict)
        return selector, unit
    matches = [
        (name, unit)
        for name, unit in units.items()
        if Path(str(unit["object"])).name == selector
    ]
    if len(matches) != 1:
        raise ValueError(f"match unit not found or ambiguous: {selector}")
    name, unit = matches[0]
    assert isinstance(name, str) and isinstance(unit, dict)
    return name, unit


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument("--unit")
    selection.add_argument("--object-name")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--compare", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    try:
        manifest = load_manifest()
        units = manifest["units"]
        assert isinstance(units, dict)
        if args.check:
            count = sum(len(unit["functions"]) for unit in units.values())
            result = {"ok": True, "schema_version": 1, "units": len(units), "functions": count}
            print(json.dumps(result, indent=2) if args.json else f"match-unit graph OK: {len(units)} units, {count} functions")
            return 0
        if args.list:
            rows = [
                {
                    "unit": name,
                    "source": unit["source"],
                    "object": unit["object"],
                    "addresses": [function["address"] for function in unit["functions"]],
                }
                for name, unit in units.items()
            ]
            if args.json:
                print(json.dumps(rows, indent=2))
            else:
                for row in rows:
                    print(f"{row['unit']}: {row['source']} -> {row['object']} ({', '.join(row['addresses'])})")
            return 0
        selector = args.unit or args.object_name
        if not selector:
            parser.error("select --unit, --object-name, --list, or --check")
        name, unit = find_unit(selector, manifest)
        provenance = build_unit(name, unit, args.json)
        code, comparisons = compare_unit(unit, args.json) if args.compare else (0, [])
        if args.json:
            print(json.dumps({"result": "ok" if code == 0 else "comparison_failed", "build": provenance, "comparisons": comparisons}, indent=2))
        return code
    except (OSError, RuntimeError, ValueError, tomllib.TOMLDecodeError) as error:
        if args.json:
            print(json.dumps({"result": "error", "failure": {"category": "build.error", "message": str(error)}}, indent=2))
        else:
            print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
