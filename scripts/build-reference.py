#!/usr/bin/env python3
"""Serial headless reproduction of the pinned some100 TH07 matching profile."""
from __future__ import annotations
import argparse,ast,fcntl,hashlib,json,os,shutil,subprocess,sys,tomllib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
PIN='4878bf0e42a9280ef44735eb0517d4fda7cb6e04'
HACKERY_SHA='9d94512d1db673bcb91b34ee0d4ba5ca3ee2e01fd7ce80d09eaabdae2a1623fb'

def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def win(path):return 'Z:'+str(Path(path).resolve()).replace('/','\\')
def env():
    result=os.environ.copy();vc=ROOT/'.tools/msvc700/Vc7';sdk=ROOT/'.tools/dx8sdk'
    result.update(DISPLAY='',WAYLAND_DISPLAY='',WINEDEBUG='-all',WINEARCH='win32',WINEPREFIX=str(ROOT.parent/'.th07-wine-vc7'),WINEDLLOVERRIDES='winemenubuilder.exe=d')
    result['INCLUDE']=';'.join(win(p) for p in [vc/'include',vc/'PlatformSDK/include',sdk/'include'])
    result['LIB']=';'.join(win(p) for p in [vc/'lib',vc/'PlatformSDK/lib',sdk/'lib'])
    return result

def run(command,log,cwd,environment):
    cmd=['taskset','-c',str(max(os.sched_getaffinity(0))),'nice','-n','15',*map(str,command)]
    with log.open('wb') as stream:r=subprocess.run(cmd,cwd=cwd,env=environment,stdout=stream,stderr=subprocess.STDOUT)
    if r.returncode:
        sys.stderr.write(log.read_text(errors='replace')[-8000:]);raise RuntimeError(f'headless tool failed: {log}')

def main():
    p=argparse.ArgumentParser();p.add_argument('--source-root',type=Path);p.add_argument('--prepare-only',action='store_true');p.add_argument('--no-gl',action='store_true');a=p.parse_args()
    compile_lock=(ROOT.parent/'.th07-vc7-compile.lock').open('a')
    fcntl.flock(compile_lock,fcntl.LOCK_EX)
    ref=ROOT/'_reference/some100-th07'
    if subprocess.check_output(['git','-C',str(ref),'rev-parse','HEAD'],text=True).strip()!=PIN:raise ValueError('reference commit mismatch')
    if subprocess.check_output(['git','-C',str(ref),'status','--porcelain','--untracked-files=no'],text=True).strip():raise ValueError('reference checkout has tracked changes')
    source=a.source_root.resolve() if a.source_root else ref
    tag='authored' if a.source_root else 'reference'
    if a.no_gl:tag+='-coff'
    out=ROOT/'build'/tag;out.mkdir(parents=True,exist_ok=True);environment=env()
    stock=ROOT/'.tools/msvc700/Vc7/bin';patched=ROOT/'.tools/vc70-reference/bin'
    lock=tomllib.loads((ROOT/'config/tools.lock.toml').read_text())['msvc70']
    if digest(stock/'cl.exe')!=lock['compiler_sha256'] or digest(stock/'c1xx.dll')!=lock['frontend_sha256']:raise ValueError('stock compiler is not pinned VC7')
    if digest(ROOT/'.tools/hackery.cpp')!=HACKERY_SHA:raise ValueError('upstream var_order source mismatch')
    if not (patched/'frontend.json').exists():
        shutil.copytree(stock,patched,dirs_exist_ok=True)
        wrapper=out/'hackery.dll'
        run(['wine',stock/'cl.exe','/nologo','/LD',win(ROOT/'.tools/hackery.cpp'),'/Fo'+win(out/'hackery.obj'),'/Fe'+win(wrapper),'/link','/implib:'+win(out/'hackery.lib')],out/'hackery.log',out,environment)
        if not wrapper.is_file():raise ValueError('compiler did not produce frontend wrapper')
        (patched/'c1xx.dll').rename(patched/'c1xxorig.dll');shutil.copyfile(wrapper,patched/'c1xx.dll')
        (patched/'frontend.json').write_text(json.dumps({'source_sha256':HACKERY_SHA,'frontend_sha256':digest(patched/'c1xx.dll'),'original_sha256':digest(patched/'c1xxorig.dll')},indent=2)+'\n')
    wrapper=json.loads((patched/'frontend.json').read_text())
    if digest(patched/'c1xx.dll')!=wrapper['frontend_sha256'] or digest(patched/'c1xxorig.dll')!=lock['frontend_sha256']:raise ValueError('frontend receipt mismatch')
    if a.prepare_only:
        print('Pinned VC7 frontend prepared (headless).');return
    compiler=patched/'cl.exe';linker=stock/'link.exe';generated=out/'generated';generated.mkdir(exist_ok=True)
    run([sys.executable,ref/'scripts/generate_i18n.py',source/'resources/csv/i18n.csv',generated/'i18n.hpp'],out/'i18n.log',ROOT,environment)
    # Extract literal build lists without executing upstream bootstrap/downloads.
    parsed=ast.parse((ref/'scripts/build.py').read_text());variables={}
    for n in parsed.body:
        if isinstance(n,ast.Assign) and isinstance(n.value,ast.List):
            for t in n.targets:
                if isinstance(t,ast.Name):
                    try:variables[t.id]=ast.literal_eval(n.value)
                    except (ValueError,TypeError):pass
    files=variables['TH07_SOURCES'];small=set(variables['TH07_SMALL']);nonintrinsic=set(variables['TH07_SMALL_NON_INTRINSIC'])
    tree=[{'path':str(p.relative_to(source)),'sha256':digest(p)} for base in [source/'src/th07',source/'resources/csv'] for p in sorted(base.rglob('*')) if p.is_file()]
    commands=[];objects=[]
    for name in files:
        obj=out/(name.replace('/','_')[:-4]+'.obj');pdb=obj.with_suffix('.pdb')
        flags=['/Os' if name in small else '/Od']
        if name not in nonintrinsic:flags+=['/Oi']
        flags+=['/nologo','/W3','/GF','/Zi','/Ob1','/MT','/EHsc','/Gr','/Gy','/DNDEBUG','/wd4060','/wd4101','/wd4244']
        if not a.no_gl:flags+=['/GL']
        flags+=['/I'+win(generated),'/I'+win(source/'src/th07')]
        cmd=['wine',compiler,*flags,'/c',win(source/'src/th07'/name),'/Fo'+win(obj),'/Fd'+win(pdb)]
        obj.unlink(missing_ok=True)
        print(f'{tag}: {name}',flush=True);run(cmd,out/(obj.stem+'.log'),out,environment)
        if not obj.is_file():raise ValueError('compiler returned without fresh object')
        commands.append({'source':'src/th07/'+name,'object':str(obj.relative_to(ROOT)),'sha256':digest(obj),'command':list(map(str,cmd))});objects.append(obj)
    exe=out/'th07.exe';pdb=out/'th07.pdb';mapfile=out/'th07.map'
    flags=['/INCREMENTAL:NO','/MAP:'+win(mapfile),'/DEBUG','/PDB:'+win(pdb),'/OPT:REF','/OPT:ICF','/OUT:'+win(exe)]
    if not a.no_gl:flags+=['/LTCG']
    libs=['dinput8.lib','user32.lib','dsound.lib','d3d8.lib','d3dx8.lib','dxguid.lib','gdi32.lib','winmm.lib','ole32.lib']
    cmd=['wine',linker,*map(win,objects),*flags,*libs]
    exe.unlink(missing_ok=True)
    print(f'{tag}: link',flush=True);run(cmd,out/'link.log',out,environment)
    if not exe.is_file() or not pdb.is_file():raise ValueError('linker returned without fresh executable/PDB')
    # Keep a byte-for-byte faithful no-icon image, then ask the same linker
    # for base-relocation metadata without changing its output paths/code.
    faithful=out/'th07.faithful.exe';shutil.copyfile(exe,faithful)
    if not a.no_gl:
        exe.unlink(missing_ok=True)
        run([*cmd,'/FIXED:NO'],out/'link-relocations.log',out,environment)
        from pe_image import PE
        before=PE(faithful);after=PE(exe)
        if before.section_bytes('.text')!=after.section_bytes('.text'):raise ValueError('relocation metadata link changed code')
    from pdb_symbols import symbols,verify_image_binding
    from pe_image import PE
    verify_image_binding(pdb,PE(exe))
    (out/'pdb-symbols.json').write_text(json.dumps(symbols(pdb),indent=2)+'\n')
    receipt={'upstream_commit':PIN,'source_root':str(source),'source_tree':tree,'compiler_sha256':digest(compiler),'frontend_sha256':digest(patched/'c1xx.dll'),'wrapper_source_sha256':HACKERY_SHA,'linker_sha256':digest(linker),'commands':commands,'link_command':list(map(str,cmd)),'metadata_link_command':list(map(str,[*cmd,'/FIXED:NO'])),'image_sha256':digest(exe),'pdb_sha256':digest(pdb),'map_sha256':digest(mapfile),'pdb_symbols_sha256':digest(out/'pdb-symbols.json'),'pdb_parser_sha256':digest(ROOT/'scripts/pdb_symbols.py'),'faithful_image_sha256':digest(faithful),'profile':'upstream-no-GL-diagnostic' if a.no_gl else 'upstream-matching-GL-LTCG-no-icon'}
    (out/'build-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    for p in out.iterdir():
        if p.suffix.lower() in ('.pch','.pdb') and p.name.lower()!='th07.pdb':p.unlink()
    print('Receipt:',out/'build-receipt.json',flush=True)
if __name__=='__main__':main()
