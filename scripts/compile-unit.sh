#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ $# -ne 3 ]]; then
  echo "usage: scripts/compile-unit.sh src/file.cpp build/file.obj PROFILE" >&2
  exit 2
fi

source_path="$(realpath "$1")"
output_path="$(realpath -m "$2")"
pdb_path="$output_path.pdb"
profile="$3"
vc7_root="${TH07_VC7_ROOT:-$repo_root/.tools/vc7}"
wine_prefix="${TH07_WINEPREFIX:-$repo_root/.tools/wine-vc7}"
cl="$vc7_root/PROGRAM FILES/MICROSOFT VISUAL STUDIO .NET/VC7/BIN/CL.EXE"

if [[ ! -f "$cl" ]]; then
  echo "missing VC7 toolchain; run scripts/bootstrap-tools.sh" >&2
  exit 1
fi
if ! command -v wine >/dev/null || ! command -v winepath >/dev/null; then
  echo "Wine and winepath are required for the VC7 probe compiler" >&2
  exit 1
fi

case "$profile" in
  vc7-text-os)
    profile_flags=(/Os)
    ;;
  vc7-default)
    profile_flags=()
    ;;
  vc7-debug-od)
    # TH06/TH08 use unoptimized code generation for Midi and zwave. The
    # explicit trailing switches override the shared optimization defaults;
    # keep the common /Gr used by the adjacent-engine build configuration.
    profile_flags=(/Od /Ob1 /Op /G5 /GS)
    ;;
  *)
    echo "unknown VC7 compiler profile: $profile" >&2
    exit 2
    ;;
esac

mkdir -p "$(dirname "$output_path")" "$wine_prefix"
export WINEPREFIX="$wine_prefix"
export WINEARCH=win32
export WINEDEBUG=-all

source_win="$(winepath -w "$source_path")"
output_win="$(winepath -w "$output_path")"
pdb_win="$(winepath -w "$pdb_path")"
src_win="$(winepath -w "$repo_root/src")"
runner_win="$(winepath -w "$repo_root/scripts/vc7run.bat")"
export DEVENV_PREFIX
DEVENV_PREFIX="$(winepath -w "$vc7_root")"

wine "$runner_win" cl.exe \
  /nologo \
  /c \
  /MT \
  /EHsc \
  /Gs \
  /DNDEBUG \
  /Zi \
  /Gy \
  /GF \
  /Oi \
  /Gr \
  "${profile_flags[@]}" \
  "/I$src_win" \
  "/Fd$pdb_win" \
  "/Fo$output_win" \
  "$source_win"
