#!/usr/bin/env python3
"""Build maintained TH07 sources through the pinned headless VC7/LTCG profile."""
import subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
raise SystemExit(subprocess.call([sys.executable,str(root/'scripts/build-reference.py'),'--source-root',str(root),*sys.argv[1:]]))
