#!/usr/bin/env python3
"""Verify the current linked product and report unfinished exact-source work."""
import argparse,collections,importlib.util,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--require-all-exact',action='store_true');args=parser.parse_args()
    spec=importlib.util.spec_from_file_location('linked_oracle',ROOT/'scripts/reference-oracle.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    o=module.Oracle(ROOT/'build/authored');pe=o.generated
    from pe_image import PE
    faithful=PE(ROOT/'build/authored/th07.faithful.exe')
    entry=pe.base+struct.unpack_from('<I',pe.data,pe.optional+16)[0]
    if struct.unpack_from('<H',pe.data,pe.optional+68)[0]!=2:raise ValueError('product is not a Windows GUI executable')
    startup={s['address'] for s in o.native_by_name.get('WinMainCRTStartup',[])}
    if startup!={entry}:raise ValueError('PE entry does not bind uniquely to WinMainCRTStartup')
    if not o.native_by_name.get('WinMain'):raise ValueError('product lacks its reconstructed WinMain')
    if not pe.imports():raise ValueError('product lacks its import identities')
    for log in ['link.log','link-relocations.log']:
        if 'LNK4099' in (ROOT/'build/authored'/log).read_text(errors='replace'):raise ValueError('link lost required compiler debug metadata')
    for argument in o.receipt['link_command']:
        if argument.upper().startswith(('/FORCE','/IGNORE')):raise ValueError('link errors must not be suppressed')
    measurements=o.measure();globals_report=[];excluded=[]
    for target,size,name in o.original_globals:
        aliases=set(module.aliases(name))|{name+"::`vftable'", "??_7"+name+"@@6B@"}
        records=[r for alias in aliases for r in o.native_by_name.get(alias,[]) if r['type']=='data' or (alias.startswith('??_7') and r['type']=='public')]
        native={r['address'] for r in records}
        if len(native)!=1:
            if name.startswith('g_') or any(s['type']=='function' and s['name'].startswith(name+'::') for s in o.symbols):raise ValueError('missing application global identity: '+name)
            excluded.append({'name':name,'reason':'historical CRT placeholder; no unique source identity'});continue
        address=next(iter(native))
        if name in ['c_dfDIJoystick2','c_dfDIKeyboard']:
            excluded.append({'name':name,'reason':'SDK object-format descriptor; reviewed separately in boundary-review.json'});continue
        data=bytearray(pe.at(address,size,allow_bss=True));expected=o.target.at(target,size,allow_bss=True)
        if data!=faithful.at(address,size,allow_bss=True):raise ValueError('metadata link changed a source global: '+name)
        unresolved=[]
        for field in o.native_relocations:
            if not address<=field<address+size:continue
            off=field-address
            if off+4>size:raise ValueError('global relocation crosses its complete extent')
            destination=struct.unpack_from('<I',data,off)[0];resolved,evidence=o.resolve(destination,address,target,size)
            if resolved is None:unresolved.append({'offset':off,'destination':destination,'reason':evidence})
            else:struct.pack_into('<I',data,off,resolved)
        different=sum(a!=b for a,b in zip(data,expected))
        globals_report.append({'name':name,'target':target,'size':size,'status':'exact' if not different and not unresolved else 'unresolved' if unresolved else 'mismatch','different_bytes':different,'unresolved':unresolved})
    from reconstruction_state import state
    summary=state();global_failures=[r for r in globals_report if r['status']!='exact']
    result={'entry_point':entry,'entry_symbol':'WinMainCRTStartup','import_count':len(pe.imports()),'native_procedure_records':sum(s['type']=='function' for s in o.symbols),'native_named_exact_functions':sum(r['status']=='exact' for r in measurements['functions']),'native_named_functions':measurements['function_count'],'native_named_exact_bytes':measurements['exact_bytes'],'authored_exact_functions':summary['authored_exact_functions'],'authored_functions':summary['authored_functions'],'authored_exact_bytes':summary['authored_exact_bytes'],'authored_bytes':summary['authored_bytes'],'remaining_authored':summary['remaining_functions'],'globals':globals_report,'excluded_globals':excluded,'global_statuses':dict(collections.Counter(r['status'] for r in globals_report)),'faithful_text_equals_metadata_text':True,'link_errors_suppressed':False,'runtime_tested':False}
    result['build_link_verified']=not global_failures
    result['exact_source_complete']=not summary['remaining_functions'] and all(r['status']=='exact' for r in measurements['functions'])
    (ROOT/'build/link-verification.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ('globals','excluded_globals')},indent=2))
    if global_failures or (args.require_all_exact and not result['exact_source_complete']):return 1
    return 0
if __name__=='__main__':raise SystemExit(main())
