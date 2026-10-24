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
    from reconstruction_state import state
    summary=state()
    authored=[r for r in rows if r['status']!='library']
    matching=[r for r in authored if r['status']=='matching']
    total_bytes=summary['authored_bytes'];matching_bytes=summary['authored_exact_bytes']
    byte_pct=summary['authored_exact_percent']
    counts=Counter(row['status'] for row in rows)
    lines=["# Reconstruction progress", "",
        "Generated from the reviewed origin ledger and canonical complete-extent manifest. Byte totals use non-overlapping target function coverage; replay also checks trailing switch tables.", "",
        f"- Authored exact functions: **{len(matching)} / {len(authored)}**",
        f"- Authored exact bytes: **{matching_bytes:,} / {total_bytes:,} ({byte_pct:.4f}%)**",
        f"- Unaccepted authored functions: **{summary['remaining_functions']}**; see `config/remaining.csv`.",
        "- Every accepted function now replays from one maintained native linked product.",
        f"- Unmodified some100 reference, current oracle: **{summary['reference_exact_functions']} / {summary['reference_named_functions']} named functions; {summary['reference_exact_bytes']:,} / {summary['reference_named_bytes']:,} bytes ({100*summary['reference_exact_bytes']/summary['reference_named_bytes']:.4f}%)**.",
        f"- Maintained linked product: **{summary['native_named_exact_functions']} / {summary['reference_named_functions']} named functions; {summary['native_named_exact_bytes']:,} / {summary['reference_named_bytes']:,} bytes ({summary['native_named_exact_percent']:.4f}%)**.", "",
        "The two native-build figures include eight SDK/compiler functions and precede the authored-source policy exclusion. Oracle v2 adds independently validated compiler guards, typed constants, bounded string offsets and CRT forwarding identities; the initial v1 reference snapshot is archived under `config/legacy/`. They use a different scope from the authored total above. These are function-byte measurements, not whole-executable or runtime validation.", "",
        "Linked SDK/CRT code, compiler helpers, DInput data tables, and alignment do not inflate authored progress. Historical runtime exact claims are preserved separately from the current canonical replay.", "",
        f"Reviewed ledger: {len(rows):,} entries; Ghidra seed inventory: 1,573 entries. See [boundary review](BOUNDARY_REVIEW.md) and [strict oracle](ORACLE.md).", "",
        "| Status | Functions |", "| --- | ---: |"]
    lines.extend(f"| `{status}` | {counts[status]:,} |" for status in ORDER)
    lines.append("")
    filled=512*byte_pct/100
    svg=f'''<svg xmlns="http://www.w3.org/2000/svg" width="680" height="142" role="img" aria-labelledby="title desc">
  <title id="title">TH07 authored exact bytes: {byte_pct:.4f}%</title>
  <desc id="desc">{len(matching)} of {len(authored)} authored functions; {matching_bytes} of {total_bytes} bytes. All accepted functions use one maintained linked product. Whole executable equivalence is unverified.</desc>
  <rect width="680" height="142" rx="10" fill="#1f2335"/>
  <text x="24" y="32" fill="#f4f4f5" font-family="sans-serif" font-size="17">TH07 · Perfect Cherry Blossom</text>
  <text x="656" y="32" text-anchor="end" fill="#f4f4f5" font-family="monospace" font-size="16">{byte_pct:.4f}% authored exact</text>
  <rect x="24" y="50" width="512" height="14" rx="7" fill="#3b4058"/>
  <rect x="24" y="50" width="{filled:.2f}" height="14" rx="7" fill="#e46c8c"/>
  <text x="24" y="91" fill="#d5d7df" font-family="sans-serif" font-size="14">{len(matching)} / {len(authored)} functions · {matching_bytes:,} / {total_bytes:,} bytes · {summary['remaining_functions']} remaining</text>
  <text x="24" y="119" fill="#b7bdcf" font-family="sans-serif" font-size="12">Complete function extents + tables · one maintained VC7 / LTCG linked product</text>
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
