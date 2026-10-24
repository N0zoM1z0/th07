"""Separate the flat maintained layout from the untouched upstream checkout."""
from pathlib import Path


def source_dir(source: Path, reference: Path) -> Path:
    return source / ("src/th07" if source.resolve() == reference.resolve() else "src")


def source_inputs(source: Path, reference: Path) -> list[Path]:
    base = source_dir(source, reference)
    code = [p for p in sorted(base.rglob("*"))
            if p.is_file() and p.suffix in {".cpp", ".hpp", ".h"}
            and p.relative_to(base).parts[0] != "legacy"]
    return code + [p for p in sorted((source / "resources/csv").rglob("*")) if p.is_file()]
