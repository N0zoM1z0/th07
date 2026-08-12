#!/usr/bin/env python3
"""Generate lightweight Markdown and SVG progress from the function ledger."""

from __future__ import annotations

from collections import Counter
import csv
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
FUNCTIONS = ROOT / "config" / "functions.csv"
MARKDOWN = ROOT / "docs" / "PROGRESS.md"
SVG = ROOT / "resources" / "progress.svg"
ORDER = ["unclassified", "identified", "decompiled", "implemented", "compiles", "matching", "library", "blocked"]


def render() -> tuple[str, str]:
    with FUNCTIONS.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    authored = [row for row in rows if row["status"] != "library"]
    matching = [row for row in authored if row["status"] == "matching"]
    libraries = [row for row in rows if row["status"] == "library"]
    exact_libraries = [row for row in libraries if row["match_percent"] == "100.00"]
    total_bytes = sum(int(row["size"]) for row in authored)
    matching_bytes = sum(int(row["size"]) for row in matching)
    all_bytes = sum(int(row["size"]) for row in rows)
    library_bytes = sum(int(row["size"]) for row in libraries)
    exact_library_bytes = sum(int(row["size"]) for row in exact_libraries)
    exact_bytes = matching_bytes + exact_library_bytes
    function_pct = 100 * len(matching) / len(authored) if authored else 0.0
    byte_pct = 100 * matching_bytes / total_bytes if total_bytes else 0.0
    combined_pct = 100 * exact_bytes / all_bytes if all_bytes else 0.0
    counts = Counter(row["status"] for row in rows)
    lines = [
        "# Reconstruction progress", "",
        "Generated from `config/functions.csv`. Only verified 100% comparisons count as reconstructed.", "",
        f"- Matching authored functions: **{len(matching)} / {len(authored)} ({function_pct:.2f}%)**",
        f"- Matching authored bytes: **{matching_bytes:,} / {total_bytes:,} ({byte_pct:.2f}%)**",
        f"- Exact library functions: **{len(exact_libraries)} / {len(libraries)}; {exact_library_bytes:,} / {library_bytes:,} bytes**",
        f"- Combined exact bytes: **{exact_bytes:,} / {all_bytes:,} ({combined_pct:.2f}%)**",
        f"- IDA seed inventory: **{len(rows):,} functions / {all_bytes:,} function bytes**", "",
        "| Status | Functions |", "| --- | ---: |",
    ]
    lines.extend(f"| `{status}` | {counts[status]:,} |" for status in ORDER)
    lines.extend(["", "IDA extents are seed boundaries and are reconciled before exact comparison.", ""])
    filled = 440 * combined_pct / 100
    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="560" height="116" role="img" aria-label="TH07 reconstruction progress {combined_pct:.2f}% by bytes">
  <rect width="560" height="116" rx="8" fill="#1f2335"/>
  <text x="24" y="32" fill="#f4f4f5" font-family="sans-serif" font-size="16" font-weight="600">TH07 source reconstruction</text>
  <text x="536" y="32" fill="#f4f4f5" text-anchor="end" font-family="monospace" font-size="14">{combined_pct:.2f}% exact bytes</text>
  <rect x="24" y="48" width="440" height="14" rx="7" fill="#3b4058"/>
  <rect x="24" y="48" width="{filled:.2f}" height="14" rx="7" fill="#e46c8c"/>
  <text x="24" y="88" fill="#c8cad2" font-family="sans-serif" font-size="13">Authored exact: {len(matching):,} / {len(authored):,} functions · {matching_bytes:,} / {total_bytes:,} bytes</text>
  <text x="24" y="108" fill="#c8cad2" font-family="sans-serif" font-size="13">Library exact: {len(exact_libraries):,} / {len(libraries):,} functions · {exact_library_bytes:,} / {library_bytes:,} bytes</text>
</svg>
'''
    return "\n".join(lines), svg


def main() -> int:
    markdown, svg = render()
    if "--check" in sys.argv:
        stale = []
        if not MARKDOWN.exists() or MARKDOWN.read_text(encoding="utf-8") != markdown:
            stale.append(str(MARKDOWN.relative_to(ROOT)))
        if not SVG.exists() or SVG.read_text(encoding="utf-8") != svg:
            stale.append(str(SVG.relative_to(ROOT)))
        if stale:
            print("stale generated progress: " + ", ".join(stale))
            return 1
        print("progress artifacts are current")
        return 0
    MARKDOWN.write_text(markdown, encoding="utf-8")
    SVG.write_text(svg, encoding="utf-8")
    print(f"updated {MARKDOWN.relative_to(ROOT)} and {SVG.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
