#!/usr/bin/env python3
"""Drive the evidence-gated path from current source probes to a full TH07 PE."""

from __future__ import annotations

import argparse
import csv
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tomllib


ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "config" / "executable.toml"
MATCH_UNITS = ROOT / "config" / "match-units.toml"
FUNCTIONS = ROOT / "config" / "functions.csv"


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def repo_path(value: str, *, generated: bool = False) -> Path:
    path = (ROOT / value).resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError(f"path escapes repository: {value}")
    if generated and not path.is_relative_to(ROOT / "build"):
        raise ValueError(f"generated path must remain below build/: {value}")
    return path


def read_pe(path: Path) -> dict[str, object]:
    data = path.read_bytes()
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    if data[:2] != b"MZ" or data[pe : pe + 4] != b"PE\0\0":
        raise ValueError("target is not a PE image")
    machine, section_count, timestamp = struct.unpack_from("<HHI", data, pe + 4)
    optional_size, characteristics = struct.unpack_from("<HH", data, pe + 20)
    optional = pe + 24
    if struct.unpack_from("<H", data, optional)[0] != 0x10B:
        raise ValueError("target is not PE32")
    fields = {
        "machine": machine,
        "characteristics": characteristics,
        "timestamp": timestamp,
        "linker_major": data[optional + 2],
        "linker_minor": data[optional + 3],
        "entry_rva": struct.unpack_from("<I", data, optional + 16)[0],
        "image_base": struct.unpack_from("<I", data, optional + 28)[0],
        "section_alignment": struct.unpack_from("<I", data, optional + 32)[0],
        "file_alignment": struct.unpack_from("<I", data, optional + 36)[0],
        "subsystem_major": struct.unpack_from("<H", data, optional + 48)[0],
        "subsystem_minor": struct.unpack_from("<H", data, optional + 50)[0],
        "size_of_image": struct.unpack_from("<I", data, optional + 56)[0],
        "size_of_headers": struct.unpack_from("<I", data, optional + 60)[0],
        "subsystem": struct.unpack_from("<H", data, optional + 68)[0],
        "dll_characteristics": struct.unpack_from("<H", data, optional + 70)[0],
        "stack_reserve": struct.unpack_from("<I", data, optional + 72)[0],
        "stack_commit": struct.unpack_from("<I", data, optional + 76)[0],
        "heap_reserve": struct.unpack_from("<I", data, optional + 80)[0],
        "heap_commit": struct.unpack_from("<I", data, optional + 84)[0],
    }
    sections = []
    section_table = optional + optional_size
    for index in range(section_count):
        offset = section_table + index * 40
        sections.append(
            {
                "name": data[offset : offset + 8].rstrip(b"\0").decode("ascii"),
                "virtual_size": struct.unpack_from("<I", data, offset + 8)[0],
                "rva": struct.unpack_from("<I", data, offset + 12)[0],
                "raw_size": struct.unpack_from("<I", data, offset + 16)[0],
                "raw_offset": struct.unpack_from("<I", data, offset + 20)[0],
                "characteristics": struct.unpack_from("<I", data, offset + 36)[0],
            }
        )

    def rva_offset(rva: int) -> int:
        for section in sections:
            delta = rva - int(section["rva"])
            if 0 <= delta < int(section["raw_size"]):
                return int(section["raw_offset"]) + delta
        raise ValueError(f"RVA 0x{rva:X} is not file-backed")

    import_rva = struct.unpack_from("<I", data, optional + 96 + 8)[0]
    imports = []
    cursor = rva_offset(import_rva)
    while any(data[cursor : cursor + 20]):
        lookup, _, _, name_rva, iat = struct.unpack_from("<IIIII", data, cursor)
        name_offset = rva_offset(name_rva)
        end = data.index(0, name_offset)
        dll = data[name_offset:end].decode("ascii")
        thunk = rva_offset(lookup or iat)
        count = 0
        while struct.unpack_from("<I", data, thunk + count * 4)[0]:
            count += 1
        imports.append({"name": dll, "count": count})
        cursor += 20
    fields["sections"] = sections
    fields["imports"] = imports
    return fields


def load_and_validate() -> tuple[dict[str, object], dict[str, object]]:
    with MANIFEST.open("rb") as stream:
        manifest = tomllib.load(stream)
    if manifest.get("schema_version") != 1:
        raise ValueError("unsupported executable manifest schema")
    target = ROOT / "resources" / "th07.exe"
    if file_sha256(target) != manifest["target_sha256"]:
        raise ValueError("resources/th07.exe is not the manifest target")
    pe = read_pe(target)
    expected_pe = manifest["pe"]
    for key, expected in expected_pe.items():
        if pe.get(key) != expected:
            raise ValueError(f"PE {key} mismatch: target {pe.get(key)!r}, manifest {expected!r}")
    if pe["sections"] != manifest["sections"]:
        raise ValueError("PE section table disagrees with executable manifest")
    if pe["imports"] != manifest["imports"]["descriptors"]:
        raise ValueError("PE import descriptor order/counts disagree with executable manifest")
    toolchain = manifest["toolchain"]
    if (pe["linker_major"], pe["linker_minor"]) != (
        toolchain["linker_major"],
        toolchain["linker_minor"],
    ):
        raise ValueError("target linker version disagrees with executable manifest")
    with MATCH_UNITS.open("rb") as stream:
        unit_manifest = tomllib.load(stream)
    units = unit_manifest["units"]
    for name in manifest["build"]["source_units"]:
        unit = units.get(name)
        if not unit or unit.get("kind") != "probe":
            raise ValueError(f"EXE source unit is not a canonical source probe: {name}")
        repo_path(unit["source"])
    repo_path(manifest["resources"]["script"])
    repo_path(manifest["build"]["output"], generated=True)
    repo_path(manifest["build"]["resource_output"], generated=True)
    if manifest["build"]["link_enabled"] and not manifest["build"]["final_objects"]:
        raise ValueError("link_enabled requires an explicit final object list")
    return manifest, unit_manifest


def ledger_summary() -> dict[str, int]:
    with FUNCTIONS.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    library = [row for row in rows if row["status"] == "library"]
    source = [
        row for row in rows if row["status"] != "library" and row["source_file"]
    ]
    unresolved = [
        row for row in rows if row["status"] != "library" and not row["source_file"]
    ]
    return {
        "total_functions": len(rows),
        "total_function_bytes": sum(int(row["size"]) for row in rows),
        "source_functions": len(source),
        "source_function_bytes": sum(int(row["size"]) for row in source),
        "library_functions": len(library),
        "library_function_bytes": sum(int(row["size"]) for row in library),
        "unresolved_functions": len(unresolved),
        "unresolved_function_bytes": sum(int(row["size"]) for row in unresolved),
    }


def compile_known(manifest: dict[str, object], jobs: int) -> None:
    units = list(manifest["build"]["source_units"])

    def compile_one(unit: str) -> tuple[str, int, str]:
        completed = subprocess.run(
            [sys.executable, str(ROOT / "scripts" / "build.py"), "--unit", unit],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        return unit, completed.returncode, completed.stdout + completed.stderr

    failures = []
    with ThreadPoolExecutor(max_workers=max(1, min(jobs, 2))) as pool:
        futures = [pool.submit(compile_one, unit) for unit in units]
        for future in as_completed(futures):
            unit, code, output = future.result()
            if code:
                failures.append((unit, output))
            else:
                print(f"compiled {unit}")
    if failures:
        raise RuntimeError("\n".join(f"{unit}: {output}" for unit, output in failures))


def wine_environment() -> tuple[dict[str, str], str, Path]:
    vc7 = Path(os.environ.get("TH07_VC7_ROOT", ROOT / ".tools" / "vc7"))
    prefix = Path(
        os.environ.get("TH07_WINEPREFIX", str(ROOT.parent / ".th07-wine-vc7"))
    )
    runner = ROOT / "scripts" / "vc7run.bat"
    if not (vc7 / "PROGRAM FILES/MICROSOFT VISUAL STUDIO .NET/VC7/BIN/LINK.EXE").is_file():
        raise ValueError("missing VC7 toolchain; run scripts/bootstrap-tools.sh")
    prefix.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update({"WINEPREFIX": str(prefix), "WINEARCH": "win32", "WINEDEBUG": "-all"})
    prefix_win = subprocess.check_output(["winepath", "-w", str(vc7)], env=env, text=True).strip()
    runner_win = subprocess.check_output(["winepath", "-w", str(runner)], env=env, text=True).strip()
    env["DEVENV_PREFIX"] = prefix_win
    return env, runner_win, vc7


def build_resources(manifest: dict[str, object]) -> Path:
    output = repo_path(manifest["build"]["resource_output"], generated=True)
    icon = ROOT / "build" / "exe" / "th07.ico"
    icon.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [sys.executable, str(ROOT / "scripts" / "extract-target-icon.py"), "--output", str(icon)],
        cwd=ROOT,
        check=True,
    )
    env, runner_win, _ = wine_environment()
    rc_win = subprocess.check_output(
        ["winepath", "-w", str(repo_path(manifest["resources"]["script"]))],
        env=env,
        text=True,
    ).strip()
    output_win = subprocess.check_output(
        ["winepath", "-w", str(output)], env=env, text=True
    ).strip()
    subprocess.run(
        ["wine", runner_win, "rc.exe", f"/fo{output_win}", rc_win],
        cwd=ROOT,
        env=env,
        check=True,
    )
    print(f"built {output.relative_to(ROOT)}")
    return output


def link_executable(manifest: dict[str, object], summary: dict[str, int]) -> None:
    if not manifest["build"]["link_enabled"]:
        raise ValueError("final link is evidence-gated: build.link_enabled is false")
    if summary["unresolved_functions"]:
        raise ValueError(
            f"final link closure is incomplete: {summary['unresolved_functions']} functions lack source/library ownership"
        )
    resource = build_resources(manifest)
    objects = [repo_path(value) for value in manifest["build"]["final_objects"]]
    missing = [str(path.relative_to(ROOT)) for path in objects if not path.is_file()]
    if missing:
        raise ValueError(f"final objects are missing: {', '.join(missing)}")
    output = repo_path(manifest["build"]["output"], generated=True)
    output.parent.mkdir(parents=True, exist_ok=True)
    env, runner_win, _ = wine_environment()

    def windows(path: Path) -> str:
        return subprocess.check_output(["winepath", "-w", str(path)], env=env, text=True).strip()

    response = output.with_suffix(".rsp")
    args = ["/nologo", f'/OUT:"{windows(output)}"']
    args.extend(manifest["link"]["flags"])
    args.extend(f'"{windows(path)}"' for path in objects + [resource])
    args.extend(manifest["link"]["libraries"])
    response.write_text("\n".join(args) + "\n", encoding="ascii")
    subprocess.run(
        ["wine", runner_win, "link.exe", f'@{windows(response)}'],
        cwd=ROOT,
        env=env,
        check=True,
    )
    print(f"linked {output.relative_to(ROOT)}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--plan", action="store_true")
    parser.add_argument("--compile-known", action="store_true")
    parser.add_argument("--resources", action="store_true")
    parser.add_argument("--link", action="store_true")
    parser.add_argument("--json", action="store_true")
    parser.add_argument("-j", "--jobs", type=int, default=2)
    args = parser.parse_args()
    try:
        manifest, _ = load_and_validate()
        subprocess.run(
            [sys.executable, str(ROOT / "scripts" / "extract-target-icon.py"), "--check"],
            cwd=ROOT,
            check=True,
            capture_output=args.json,
            text=True,
        )
        summary = ledger_summary()
        if args.compile_known:
            compile_known(manifest, args.jobs)
        if args.resources:
            build_resources(manifest)
        if args.link:
            link_executable(manifest, summary)
        report = {
            "result": "ok",
            "target_sha256": manifest["target_sha256"],
            "source_probe_units": len(manifest["build"]["source_units"]),
            "link_enabled": manifest["build"]["link_enabled"],
            "ledger": summary,
            "unknown_exact_inputs": manifest["link"]["unknown_exact_inputs"],
        }
        if args.json:
            print(json.dumps(report, indent=2))
        elif args.check or args.plan or not any((args.compile_known, args.resources, args.link)):
            print(
                "EXE manifest OK: "
                f"{len(manifest['sections'])} sections, "
                f"{len(manifest['imports']['descriptors'])} import descriptors, "
                f"{report['source_probe_units']} source probes"
            )
            print(
                "link closure: "
                f"{summary['source_functions']} source + {summary['library_functions']} library; "
                f"{summary['unresolved_functions']} functions / "
                f"{summary['unresolved_function_bytes']} bytes unresolved"
            )
        return 0
    except (OSError, ValueError, RuntimeError, subprocess.CalledProcessError) as error:
        if args.json:
            print(json.dumps({"result": "error", "error": str(error)}, indent=2))
        else:
            print(f"build-exe: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
