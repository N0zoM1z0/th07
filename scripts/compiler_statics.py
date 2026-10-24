"""Resolve VC7 local-static guards from typed, adjacent BSS contributions."""
import tomllib

def recover(oracle,profiles,type_sizes):
    result={};audit=[]
    for profile in profiles:
        def native(name):
            return [r for r in oracle.symbols if r['type']=='data' and r['name']==name and r['module'].replace('\\','/').rsplit('/',1)[-1].lower()==profile['module'].lower()]
        objects=native(profile['object']);guards=native(profile['guard'])
        originals=[(a,size) for a,size,name in oracle.original_globals if name==profile['object']]
        if len(objects)!=1 or len(guards)!=1 or len(originals)!=1:continue
        obj,guard=objects[0],guards[0];target,size=originals[0]
        if type_sizes.get(obj['type_index'])!=size or guard['type_index']!=0x75:continue
        if obj['section']!=guard['section'] or obj['address']+size!=guard['address']:continue
        candidate=target+size
        if candidate%4 or any(a<candidate+4 and candidate<a+n for a,n,name in oracle.original_globals):continue
        def unbacked_writable(pe,address):
            return any(s['flags']&0x80000000 and pe.base+s['rva']+s['raw_size']<=address and address+4<=pe.base+s['rva']+s['virtual_size'] for s in pe.sections)
        if not unbacked_writable(oracle.generated,guard['address']) or not unbacked_writable(oracle.target,candidate):continue
        if oracle.generated.at(guard['address'],4,allow_bss=True)!=bytes(4) or oracle.target.at(candidate,4,allow_bss=True)!=bytes(4):continue
        if guard['address'] in result and result[guard['address']]!=candidate:raise ValueError('conflicting compiler static identity')
        result[guard['address']]=candidate
        audit.append({**profile,'generated_object':obj['address'],'target_object':target,'object_size':size,'generated_guard':guard['address'],'target_guard':candidate,'evidence':'typed-adjacent-VC7-local-static-BSS-contribution'})
    return result,audit
