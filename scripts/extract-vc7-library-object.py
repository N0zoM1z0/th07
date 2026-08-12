#!/usr/bin/env python3
"""Extract one i386 COFF member from a SHA-pinned VC7-era library."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[1]
LIBRARIES = {
    "d3dx8": (
        ROOT / ".tools" / "vc7" / "mssdk" / "lib" / "d3dx8.lib",
        "32148094cebbbe9b55f7769787cdde5926ca01014e072844f5152d25072e1f44",
    ),
    "libc": (
        ROOT
        / ".tools"
        / "vc7"
        / "PROGRAM FILES"
        / "MICROSOFT VISUAL STUDIO .NET"
        / "VC7"
        / "LIB"
        / "LIBC.LIB",
        "bd963bdc9388da3452b550886d5a4a35ff300dd66c8f88319daebbbcb5c14eb6",
    ),
    "libcmt": (
        ROOT
        / ".tools"
        / "vc7"
        / "PROGRAM FILES"
        / "MICROSOFT VISUAL STUDIO .NET"
        / "VC7"
        / "LIB"
        / "LIBCMT.LIB",
        "8815af7b9b6e0e28b77708ede25ab7ecfc4b05e1d8811f092c516cff5ce19d94",
    ),
}


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def archive_members(library: bytes) -> list[tuple[str, bytes]]:
    if not library.startswith(b"!<arch>\n"):
        raise ValueError("selected file is not a COFF archive")
    members: list[tuple[str, bytes]] = []
    long_names = b""
    offset = 8
    while offset + 60 <= len(library):
        header = library[offset : offset + 60]
        if header[58:60] != b"`\n":
            raise ValueError("malformed COFF archive member header")
        try:
            size = int(header[48:58].decode("ascii").strip())
        except ValueError as error:
            raise ValueError("malformed COFF archive member size") from error
        body_start = offset + 60
        body_end = body_start + size
        if body_end > len(library):
            raise ValueError("truncated COFF archive member")
        raw_name = header[:16].decode("ascii", errors="replace").rstrip()
        body = library[body_start:body_end]
        if raw_name == "//":
            long_names = body
        elif raw_name.startswith("/") and raw_name[1:].isdigit():
            name_offset = int(raw_name[1:])
            endings = [
                end
                for end in (
                    long_names.find(b"\0", name_offset),
                    long_names.find(b"/\n", name_offset),
                )
                if end >= 0
            ]
            if not endings:
                raise ValueError("invalid COFF archive long-name reference")
            name = long_names[name_offset : min(endings)].decode(
                "ascii", errors="replace"
            )
            members.append((name.replace("\\", "/"), body))
        elif raw_name not in {"/", "//"}:
            members.append((raw_name.removesuffix("/"), body))
        offset = body_end + (size & 1)
    return members


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--library", choices=sorted(LIBRARIES), required=True)
    parser.add_argument(
        "--object",
        dest="object_name",
        required=True,
        help="full archive member path, or a unique object basename",
    )
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    archive_path, expected_hash = LIBRARIES[args.library]
    library = archive_path.read_bytes()
    actual_hash = sha256(library)
    if actual_hash != expected_hash:
        raise ValueError(
            f"{archive_path.name} SHA-256 mismatch: got {actual_hash}, "
            f"expected {expected_hash}"
        )

    wanted = args.object_name.replace("\\", "/").lower()
    matches = [
        (name, body)
        for name, body in archive_members(library)
        if name.lower() == wanted
        or ("/" not in wanted and name.rsplit("/", 1)[-1].lower() == wanted)
    ]
    if len(matches) != 1:
        names = ", ".join(name for name, _ in matches[:8])
        raise ValueError(
            f"expected one {args.library}/{args.object_name} member, "
            f"got {len(matches)}: {names}"
        )
    member_name, obj = matches[0]
    if len(obj) < 2 or struct.unpack_from("<H", obj, 0)[0] != 0x014C:
        raise ValueError("selected archive member is not an i386 COFF object")

    output = args.output.resolve()
    if not output.is_relative_to((ROOT / "build").resolve()):
        raise ValueError("generated object must stay below build/")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(obj)
    print(
        f"VC7 {args.library}/{member_name}: {len(obj)} bytes, "
        f"sha256={sha256(obj)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
