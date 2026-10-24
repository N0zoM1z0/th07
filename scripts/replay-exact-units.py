#!/usr/bin/env python3
"""Replay complete accepted function extents; never normalize or mask bytes."""
import argparse, hashlib, importlib.util, json, subprocess, sys
from pathlib import Path
from coff import Object
ROOT=Path(__file__).resolve().parents[1]
def module(name, filename):
    spec=importlib.util.spec_from_file_location(name, ROOT/'scripts'/filename)
    result=importlib.util.module_from_spec(spec);spec.loader.exec_module(result);return result
def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def legacy_replay(record, builder, comparator):
    unit=builder.load_manifest()['units'][record['legacy_unit']]
    obj=ROOT/unit['object'];receipt=json.loads(obj.with_suffix(obj.suffix+'.provenance.json').read_text())
    inputs=receipt['inputs']
    expected={'source_sha256':digest(ROOT/unit['source']), 'manifest_sha256':digest(builder.MANIFEST),
              'compile_script_sha256':digest(ROOT/'scripts/compile-unit.sh'), 'runner_sha256':digest(ROOT/'scripts/vc7run.bat')}
    for key,value in expected.items():
        if inputs[key]!=value:raise ValueError('stale legacy build input: '+key)
    headers={str(p.relative_to(ROOT)):digest(p) for p in sorted((ROOT/'src').glob('*.h*')) if p.is_file()}
    if inputs.get('headers')!=headers:raise ValueError('stale legacy headers; rebuild required')
    if receipt['object_sha256']!=digest(obj):raise ValueError('legacy object changed')
    if inputs['compiler_sha256']!={p.name:digest(p) for p in builder.compiler_paths()}:raise ValueError('legacy compiler changed')
    if inputs['profile']!=unit['profile'] or receipt['source']!=record['source_file']:raise ValueError('legacy source/profile differs')
    function=next(f for f in unit['functions'] if f['address']==record['address'])
    symbols=[s for s in Object(obj).functions() if s['name']==function['symbol_base']]
    if len(symbols)!=1 or symbols[0]['size']!=record['compare_size']:raise ValueError('complete COFF extent changed')
    rel32={k:int(v,0) for k,v in (x.split('=',1) for x in function.get('rel32_targets',[]))}
    dir32=dict(x.split('=',1) for x in function.get('dir32_targets',[]))
    data=comparator.coff_symbol_bytes(obj,function['symbol_base'],int(record['address'],0),record['compare_size'],rel32,dir32)
    if len(data)<record['compare_size']:raise ValueError('truncated COFF replay')
    data=data[:record['compare_size']]
    if data!=comparator.target_bytes(int(record['address'],0),len(data)):raise ValueError('legacy complete-byte mismatch')
    return hashlib.sha256(data).hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--unit');parser.add_argument('--rebuild',action='store_true');args=parser.parse_args()
    manifest=json.loads((ROOT/'config/exact-units.json').read_text());units=manifest['units']
    if args.unit:
        if args.unit not in units:raise ValueError('unknown canonical unit')
        units={args.unit:units[args.unit]}
    from source_policy import exclusions
    denied=exclusions(ROOT)
    if any(int(record['address'],0) in denied for record in units.values()):raise ValueError('unit is excluded by current source policy')
    if args.rebuild:
        if any(r['kind']=='native-linked' for r in units.values()):subprocess.run([sys.executable,str(ROOT/'scripts/build-reference.py'),'--source-root',str(ROOT)],check=True)
        names=sorted({r['legacy_unit'] for r in units.values() if r['kind']=='legacy-coff'})
        if names:subprocess.run([sys.executable,str(ROOT/'scripts/replay-legacy-units.py'),'--units',*names],check=True)
    native={};oracle=None
    if any(r['kind']=='native-linked' for r in units.values()):
        oracle=module('native_oracle','reference-oracle.py').Oracle(ROOT/'build/authored')
        if Path(oracle.receipt['source_root']).resolve()!=ROOT:raise ValueError('canonical replay requires maintained sources')
        if manifest['target_sha256']!=digest(ROOT/'resources/th07.exe'):raise ValueError('manifest target identity differs')
        native={r['address']:r for r in oracle.measure()['functions']}
    builder=module('legacy_builder','build.py');comparator=module('legacy_comparator','compare-function.py');comparator.verify_target()
    results=[]
    for name,record in units.items():
        if record['kind']=='native-linked':
            result=native[int(record['address'],0)]
            if result['status']!='exact' or result['compare_size']!=record['compare_size'] or result['name']!=record['native_name']:raise ValueError('native full extent failed: '+name)
            actual=digest_bytes(oracle.target.at(int(record['address'],0),record['compare_size']))
        elif record['kind']=='legacy-coff':actual=legacy_replay(record,builder,comparator)
        else:raise ValueError('unknown replay kind')
        if actual!=record['target_compared_sha256']:raise ValueError('complete target extent digest differs: '+name)
        results.append({'unit':name,'address':record['address'],'origin':record['origin'],'kind':record['kind'],'size':record['size'],'compare_size':record['compare_size'],'result':'exact'})
    report={'schema_version':1,'target_sha256':manifest['target_sha256'],'functions':results,'exact_functions':len(results),'authored_exact_functions':sum(r['origin']=='authored' for r in results),'authored_exact_bytes':sum(r['size'] for r in results if r['origin']=='authored')}
    out=ROOT/'build'/('exact-replay.json' if not args.unit else args.unit+'-replay.json');out.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='functions'},indent=2))
def digest_bytes(data):return hashlib.sha256(data).hexdigest()
if __name__=='__main__':main()
