#!/usr/bin/env python3
"""Admit independently proven source while preserving the previous TH07 work."""
import csv,hashlib,importlib.util,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def load_module(name,file):
    s=importlib.util.spec_from_file_location(name,ROOT/'scripts'/file);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def csv_write(path,fields,rows):
    with path.open('w',newline='') as stream:
        writer=csv.DictWriter(stream,fieldnames=fields,lineterminator='\n');writer.writeheader();writer.writerows(rows)
def main():
    module=load_module('reference_oracle','reference-oracle.py');oracle=module.Oracle(ROOT/'build/authored');current=oracle.measure()
    baseline=module.Oracle(ROOT/'build/reference').measure()
    (ROOT/'build/authored-comparison.json').write_text(json.dumps(current,indent=2)+'\n');(ROOT/'build/reference-baseline.json').write_text(json.dumps(baseline,indent=2)+'\n')
    legacy=json.loads((ROOT/'build/legacy-replay.json').read_text())
    legacy_good={int(r['address'],0):r for r in legacy['results'] if r['report'].get('complete_extent_result')=='exact'}
    old=[]
    with (ROOT/'config/legacy/functions.csv').open() as stream:
        reader=csv.reader(stream);fields=next(reader)
        for values in reader:
            if len(values)!=len(fields):
                # Historical ledger contains five unquoted comma-bearing fields.
                owner=values.index('root',12)
                values=values[:12]+[','.join(values[12:owner]),values[owner],','.join(values[owner+1:])]
            if len(values)!=len(fields):raise ValueError('invalid historical row')
            old.append(dict(zip(fields,values)))
    old_by={int(r['address'],0):r for r in old};fields=list(old[0]);rows={a:r.copy() for a,r in old_by.items()}
    sdk={f['address'] for f in current['functions'] if f['name'].startswith('D3DX')}
    generated={f['address'] for f in current['functions'] if 'deleting destructor' in f['name']}
    from source_policy import ADDRESS,APPROVAL_NOTE,exclusions
    policy=exclusions(ROOT)
    application=[f for f in current['functions'] if f['address'] not in sdk|generated]
    source_for_object={Path(r['object']).name:r['source'] for r in oracle.receipt['commands']}
    accepted=[];remaining=[];source_rows=[]
    for f in current['functions']:
        a=f['address'];origin='library' if a in sdk else 'compiler' if a in generated else 'authored'
        exact=f['status']=='exact' and a not in policy
        fallback=not exact and a in legacy_good and a not in policy
        native_name=f['name'];emission=source_for_object.get(f.get('module','').replace('\\','/').rsplit('/',1)[-1],'')
        if a not in rows:
            template={k:'' for k in fields};template.update(address=f'0x{a:08X}',size=str(f['size']),span_end=f'0x{a+f["size"]-1:08X}',current_name=f'FUN_{a:08x}',calling_convention='unknown',is_thunk='false',match_percent='0.00');rows[a]=template
        row=rows[a];row['size']=str(f['size']);row['span_end']=f'0x{a+f["size"]-1:08X}';row['proposed_name']=native_name;row['owner']='root';row['module']=emission or origin
        if exact or fallback:
            kind='legacy-coff' if fallback else 'native-linked';unit=f'f_{a:08x}'
            source=legacy_good[a]['source'] if fallback else emission
            if not source:raise ValueError('accepted function lacks an emission-owner source')
            compare_size=legacy_good[a]['report']['compared_size'] if fallback else f['compare_size']
            expected=oracle.target.at(a,compare_size)
            record={'unit':unit,'address':f'0x{a:08X}','size':f['size'],'compare_size':compare_size,'name':native_name,'origin':origin,'kind':kind,'source_file':source,'target_compared_sha256':hashlib.sha256(expected).hexdigest()}
            if fallback:record['legacy_unit']=legacy_good[a]['unit']
            else:record['native_name']=native_name
            accepted.append(record)
            row.update(status='matching' if origin=='authored' else 'library',match_percent='100.00',source_file=source,evidence=f'python3 scripts/replay-exact-units.py --unit {unit}; complete extent {compare_size} bytes; {kind}',notes='Independent full-extent acceptance; original source/ledger preserved in history and config/legacy.')
        else:
            reason=policy.get(a,f['status']);remaining.append({'address':f'0x{a:08X}','name':native_name,'size':f['size'],'reason':reason,'source_file':emission})
            row.update(status='compiles',match_percent='0.00',source_file=emission,evidence='build/authored-comparison.json; not accepted as exact',notes=reason)
        if a==ADDRESS and exact:row['notes']=APPROVAL_NOTE
        source_rows.append({'address':f'0x{a:08X}','origin':origin,'evidence':'source annotation + target boundary + generated full procedure extent','notes':APPROVAL_NOTE if a==ADDRESS and exact else policy.get(a,'Prebuilt SDK inline' if origin=='library' else 'Compiler-generated deleting wrapper' if origin=='compiler' else 'Application/PBG source-owned function')})
    # Preserve the historical runtime inventory. These regions are outside the
    # complete application source region; classification grants no new exactness.
    app_addresses={f['address'] for f in current['functions']}
    for a,row in rows.items():
        if a in app_addresses:continue
        origin='compiler' if a>=0x48c3a0 else 'library'
        row['status']='library';row['notes']=(row.get('notes','')+'; '+('Compiler EH/startup region' if origin=='compiler' else 'Linked SDK/CRT region')).strip('; ')
        row['span_end']=f'0x{a+int(row["size"])-1:08X}'
        source_rows.append({'address':row['address'],'origin':origin,'evidence':'application-region closure + historical target inventory','notes':'Historical exact evidence retained where present; no new byte credit from region classification.'})
    # Account for every main-region byte outside full compiler extents.
    cursor=0x401000;gaps=[];overlaps=[]
    for f in sorted(current['functions'],key=lambda f:f['address']):
        a=f['address'];end=a+f.get('compare_size',f['size'])
        if a<cursor:overlaps.append([a,cursor])
        if a>cursor:
            data=oracle.target.at(cursor,a-cursor)
            if any(b not in (0x90,0xcc) for b in data):raise ValueError(f'unreviewed application gap at {cursor:#x}')
            gaps.append({'address':cursor,'size':a-cursor,'origin':'alignment'})
        cursor=max(cursor,end)
    if overlaps:raise ValueError('complete application procedures overlap')
    extras=[r for a,r in oracle.ghidra.items() if 0x401000<=a<cursor and a not in app_addresses]
    if extras:raise ValueError('Ghidra has unmapped application entries')
    formats=[]
    for addr in [0x48d46c,0x48d674]:
        size,obj_size,flags,data_size,count,ptr=struct.unpack('<6I',oracle.target.at(addr,24))
        if size!=24 or obj_size!=16:raise ValueError('DInput data-format layout differs')
        formats.append({'descriptor':addr,'data_start':ptr,'data_end':ptr+obj_size*count,'object_count':count,'object_stride':obj_size})
    if formats[0]['data_start']<cursor or formats[0]['data_end']!=formats[1]['data_start'] or formats[1]['data_end']!=0x461a90:raise ValueError('SDK data tables do not close application/runtime boundary')
    if any(b not in (0x90,0xcc) for b in oracle.target.at(cursor,formats[0]['data_start']-cursor)):raise ValueError('unreviewed bytes before SDK object tables')
    csv_write(ROOT/'config/functions.csv',fields,[rows[a] for a in sorted(rows)])
    csv_write(ROOT/'config/function-origins.csv',['address','origin','evidence','notes'],sorted(source_rows,key=lambda r:r['address']))
    csv_write(ROOT/'config/matches.csv',['unit','address','size','compare_size','name','origin','kind','source_file'],[{k:r[k] for k in ['unit','address','size','compare_size','name','origin','kind','source_file']} for r in accepted])
    csv_write(ROOT/'config/remaining.csv',['address','name','size','reason','source_file'],remaining)
    manifest={'schema_version':1,'target_sha256':current['target_sha256'],'reference_commit':current['reference_commit'],'units':{r['unit']:r for r in accepted}}
    (ROOT/'config/exact-units.json').write_text(json.dumps(manifest,indent=2)+'\n')
    baseline_rows=[{'address':f'0x{f["address"]:08X}','name':f['name'],'size':f['size'],'compare_size':f.get('compare_size',''),'status':f['status']} for f in baseline['functions']]
    csv_write(ROOT/'config/reference-baseline.csv',['address','name','size','compare_size','status'],baseline_rows)
    accepted_app=[r for r in accepted if r['origin']=='authored']
    review={'target_sha256':current['target_sha256'],'reference_commit':current['reference_commit'],'reference_named_functions':baseline['function_count'],'reference_named_bytes':baseline['function_bytes'],'reference_named_exact_bytes':baseline['exact_bytes'],'authored_functions':len(application),'authored_bytes':module.union_size((r['address'],r['address']+r['size']) for r in application),'authored_exact_functions':len(accepted_app),'authored_exact_bytes':module.union_size((int(r['address'],0),int(r['address'],0)+r['size']) for r in accepted_app),'application_start':0x401000,'application_end_exclusive':cursor,'runtime_start':0x461a90,'sdk_data_formats':formats,'unreviewed_application_gap_bytes':0,'unmapped_ghidra_application_entries':0,'alignment_ranges':gaps,'compiler_eh':current['compiler_eh'],'compiler_statics':current['compiler_statics'],'compiler_thunks':current['compiler_thunks'],'native_named_exact_functions':sum(f['status']=='exact' for f in current['functions']),'native_named_exact_bytes':current['exact_bytes'],'oracle_version':2,'source_policy_exclusions':{hex(k):v for k,v in policy.items()}}
    (ROOT/'config/boundary-review.json').write_text(json.dumps(review,indent=2)+'\n')
    print(json.dumps({k:v for k,v in review.items() if k not in ['alignment_ranges','compiler_eh']},indent=2))
if __name__=='__main__':main()
