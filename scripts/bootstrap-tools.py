#!/usr/bin/env python3
"""Prepare pinned tools and reference sources; no GUI applications are launched."""
import argparse,hashlib,os,shutil,subprocess,tarfile,tomllib,urllib.request,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def clone(url,commit,dest):
    if not dest.exists():subprocess.run(['git','clone','--quiet',url,str(dest)],check=True)
    current=subprocess.check_output(['git','-C',str(dest),'rev-parse','HEAD'],text=True).strip()
    if current!=commit:
        if dest.is_symlink():raise ValueError('shared checkout pin mismatch')
        subprocess.run(['git','-C',str(dest),'checkout','--quiet','--detach',commit],check=True)
    if subprocess.check_output(['git','-C',str(dest),'rev-parse','HEAD'],text=True).strip()!=commit:raise ValueError('Git pin mismatch')
def fetch(spec):
    cache=ROOT/'.tools/downloads';cache.mkdir(parents=True,exist_ok=True)
    p=cache/spec['asset']
    if not p.exists():urllib.request.urlretrieve(spec['url'],p)
    if hashlib.sha256(p.read_bytes()).hexdigest()!=spec['sha256']:raise ValueError(f'download checksum mismatch: {p}')
    return p
def link_external(dest,source):
    if source and not dest.exists():dest.symlink_to(source.resolve(),target_is_directory=True)
def main():
    p=argparse.ArgumentParser();p.add_argument('--reference-only',action='store_true');p.add_argument('--msvc700',type=Path);p.add_argument('--ghidra-home',type=Path);p.add_argument('--jdk-home',type=Path);p.add_argument('--dx8sdk',type=Path);a=p.parse_args()
    lock=tomllib.loads((ROOT/'config/tools.lock.toml').read_text());ref=lock['reference']
    clone(ref['repository'],ref['commit'],ROOT/'_reference/some100-th07')
    tools=ROOT/'.tools';tools.mkdir(exist_ok=True)
    shim=tools/'hackery.cpp'
    if not shim.exists():urllib.request.urlretrieve(ref['frontend_url'],shim)
    if hashlib.sha256(shim.read_bytes()).hexdigest()!=ref['frontend_source_sha256']:raise ValueError('frontend source pin mismatch')
    if a.reference_only:return
    tools=ROOT/'.tools';tools.mkdir(exist_ok=True)
    link_external(tools/'msvc700',a.msvc700)
    clone(lock['msvc70']['repository'],lock['msvc70']['commit'],tools/'msvc700')
    from tool_identity import TOOL_FILES,stock_identity,require_tool_identity
    require_tool_identity(stock_identity(tools/'msvc700/Vc7/bin'),lock['msvc70'],TOOL_FILES)
    link_external(tools/'ghidra',a.ghidra_home);link_external(tools/'jdk',a.jdk_home);link_external(tools/'dx8sdk',a.dx8sdk)
    if not (tools/'ghidra').exists():
        archive=fetch(lock['ghidra'])
        with zipfile.ZipFile(archive) as z:z.extractall(tools)
        (tools/'ghidra').symlink_to('ghidra_'+lock['ghidra']['version']+'_PUBLIC',target_is_directory=True);archive.unlink()
    if not (tools/'jdk').exists():
        archive=fetch(lock['temurin_jdk'])
        with tarfile.open(archive) as t:
            top=t.getmembers()[0].name.split('/')[0];t.extractall(tools,filter='data')
        (tools/'jdk').symlink_to(top,target_is_directory=True);archive.unlink()
    if not (tools/'dx8sdk').exists():
        archive=fetch(lock['dx8sdk']);(tools/'dx8sdk').mkdir()
        with zipfile.ZipFile(archive) as z:z.extractall(tools/'dx8sdk')
        archive.unlink()
    for name,expected in lock['dx8sdk']['files'].items():
        if hashlib.sha256((tools/'dx8sdk'/name).read_bytes()).hexdigest()!=expected:raise ValueError(f'DirectX SDK identity mismatch: {name}')
    subprocess.run([os.sys.executable,str(ROOT/'scripts/build-reference.py'),'--prepare-only'],check=True)
    # Build an isolated case-correct legacy layout without changing shared tools.
    bridge=tools/'legacy-vc7';vc=bridge/'PROGRAM FILES/MICROSOFT VISUAL STUDIO .NET/VC7'
    bin_dir=vc/'BIN';bin_dir.mkdir(parents=True,exist_ok=True)
    for file in (tools/'vc70-reference/bin').iterdir():
        if file.suffix.lower() in ('.exe','.dll'):link_external(bin_dir/file.name.upper(),file)
    stock=tools/'msvc700/Vc7'
    for name,source in [('INCLUDE',stock/'include'),('LIB',stock/'lib'),('PlatformSDK',stock/'PlatformSDK')]:link_external(vc/name,source)
    link_external(bridge/'mssdk',tools/'dx8sdk');link_external(tools/'vc7',bridge)
    print('Pinned tools ready; all invocations are headless.')
if __name__=='__main__':main()
