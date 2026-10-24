#!/usr/bin/env python3
"""Rebuild preserved accepted VC7 probes serially, retaining compact receipts."""
import argparse,csv,importlib.util,json,sys
from pathlib import Path
from coff import Object
import hashlib
import tomllib
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('legacy_build',ROOT/'scripts/build.py');b=importlib.util.module_from_spec(spec);spec.loader.exec_module(b)
def main():
    p=argparse.ArgumentParser();p.add_argument('--units',nargs='*');p.add_argument('--output',type=Path,default=ROOT/'build/legacy-replay.json');args=p.parse_args()
    manifest=b.load_manifest();ledger={r['address']:r for r in csv.DictReader((ROOT/'config/legacy/functions.csv').open())};rows=[];built=[]
    for name,unit in manifest['units'].items():
        if args.units is not None and name not in args.units:continue
        selected=[f for f in unit['functions'] if ledger[f['address']]['match_percent']=='100.00' and ledger[f['address']]['status'] in ['matching','library']]
        if not selected:continue
        print('legacy:',name,flush=True)
        receipt=b.build_unit(name,unit,True);filtered={**unit,'functions':selected};status,reports=b.compare_unit(filtered,True)
        compare_spec=importlib.util.spec_from_file_location('legacy_compare',ROOT/'scripts/compare-function.py');comparison=importlib.util.module_from_spec(compare_spec);compare_spec.loader.exec_module(comparison)
        comparison.verify_target();obj=Object(ROOT/unit['object'])
        for f,r in zip(selected,reports):
            if r.get('result')=='exact' and unit['kind']=='probe':
                symbols=[s for s in obj.functions() if s['name']==f['symbol_base']]
                if len(symbols)!=1:raise ValueError('cannot independently establish complete COFF function extent')
                extent=symbols[0]['size'];address=int(f['address'],0)
                if extent<int(ledger[f['address']]['size']):raise ValueError('legacy COFF extent truncates accepted coverage')
                rel32={k:int(v,0) for k,v in (x.split('=',1) for x in f.get('rel32_targets',[]))}
                dir32=dict(x.split('=',1) for x in f.get('dir32_targets',[]))
                actual=comparison.coff_symbol_bytes(ROOT/unit['object'],f['symbol_base'],address,extent,rel32,dir32)[:extent]
                target=comparison.target_bytes(address,extent)
                if len(actual)!=extent or actual!=target:raise ValueError('legacy function/tables fail complete COFF-extent replay')
                r['compared_size']=extent;r['complete_extent_result']='exact';r['complete_extent_sha256']=hashlib.sha256(actual).hexdigest()
            rows.append({'unit':name,'address':f['address'],'source':unit['source'],'report':r})
        built.append({'unit':name,'receipt':receipt})
        if status:raise ValueError('previously accepted legacy unit failed: '+name)
        output=ROOT/unit['object']
        output.with_suffix('.obj.pdb').unlink(missing_ok=True)
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps({'builds':built,'results':rows},indent=2)+'\n');print('Legacy exact comparisons:',len(rows))
if __name__=='__main__':main()
