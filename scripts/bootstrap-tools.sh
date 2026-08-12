#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
tools_root="$repo_root/.tools"
tooling_checkout="$tools_root/th08-tooling"
downloads="$tools_root/downloads"
vc7_root="$tools_root/vc7"
wine_prefix="${TH07_WINEPREFIX:-$(dirname "$repo_root")/.th07-wine-vc7}"
th08_commit=7ad379297baf4ff07f117747ea4edf8c7ed739d4

for command in git msiextract wine winepath python3 flock; do
  if ! command -v "$command" >/dev/null 2>&1; then
    echo "missing required command: $command" >&2
    echo "on Ubuntu install Wine (including i386 support), msitools, and Git" >&2
    exit 1
  fi
done

mkdir -p "$tools_root" "$downloads"
if [[ ! -d "$tooling_checkout/.git" ]]; then
  git clone https://github.com/GensokyoClub/th08.git "$tooling_checkout"
fi
git -C "$tooling_checkout" fetch --quiet origin "$th08_commit"
git -C "$tooling_checkout" checkout --quiet --detach "$th08_commit"

export WINEPREFIX="$wine_prefix"
export WINEARCH=win32
export WINEDEBUG=-all
wineboot --init

cl="$vc7_root/PROGRAM FILES/MICROSOFT VISUAL STUDIO .NET/VC7/BIN/CL.EXE"
dx8_header="$vc7_root/mssdk/include/d3d8.h"
if [[ ! -f "$cl" || ! -f "$dx8_header" ]]; then
  python3 "$tooling_checkout/scripts/create_devenv.py" \
    "$downloads" \
    "$vc7_root" \
    --only vs \
    --only dx8 \
    "$@"
fi

if [[ ! -f "$cl" ]]; then
  echo "VC7 bootstrap completed without the expected compiler: $cl" >&2
  exit 1
fi

# The upstream pragma installer assumes its output lives at th08/scripts/prefix.
# Compile and install the same reviewed shim through this repository's runner so
# a project-local .tools/vc7 prefix remains relocatable.
vc7_bin="$(dirname "$cl")"
original_c1xx="$vc7_bin/C1XXOrig.DLL"
if [[ ! -f "$original_c1xx" ]]; then
  shim_build="$tools_root/shim-build"
  mkdir -p "$shim_build"
  export DEVENV_PREFIX="$(winepath -w "$vc7_root")"
  runner_win="$(winepath -w "$repo_root/scripts/vc7run.bat")"
  shim_source_win="$(winepath -w "$tooling_checkout/scripts/pragma_var_order.cpp")"
  shim_dll_win="$(winepath -w "$shim_build/C1XX.DLL")"
  shim_obj_win="$(winepath -w "$shim_build/pragma_var_order.obj")"
  wine "$runner_win" CL.EXE \
    /nologo \
    /LD \
    "$shim_source_win" \
    "/Fo$shim_obj_win" \
    "/Fe$shim_dll_win"
  mv "$vc7_bin/C1XX.DLL" "$original_c1xx"
  cp "$shim_build/C1XX.DLL" "$vc7_bin/C1XX.DLL"
fi

if [[ ! -f "$vc7_bin/C1XX.DLL" || ! -f "$original_c1xx" ]]; then
  echo "VC7 pragma shim installation failed" >&2
  exit 1
fi
echo "VC7/DirectX 8 environment ready under $vc7_root"
