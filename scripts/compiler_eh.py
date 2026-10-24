"""Independently pair VC7 FuncInfo/unwind graphs and validate complete helpers.

Pairing uses readonly metadata shapes and unique cleanup-graph identities. Acceptance
requires full helper-byte replay and full readonly graph equality. Original
user-function instruction operands are never used to select a destination.
"""
import struct
MAGIC=0x19930520

def descriptors(pe):
    section=pe.section('.rdata');data=pe.section_bytes('.rdata');result=[];offset=0
    while True:
        offset=data.find(struct.pack('<I',MAGIC),offset)
        if offset<0:break
        if offset+28<=len(data):
            words=struct.unpack_from('<7I',data,offset)
            if 0<words[1]<=32 and words[3:]==(0,0,0,0):
                try:
                    states=[struct.unpack('<iI',pe.at(words[2]+i*8,8)) for i in range(words[1])]
                    if all(-1<=state<words[1] for state,action in states):result.append({'address':pe.base+section['rva']+offset,'words':words,'states':states})
                except ValueError:pass
        offset+=4
    return result

def handler(pe,info,frame_handler):
    section=pe.section('.text');data=pe.section_bytes('.text');needle=b'\xb8'+struct.pack('<I',info);offset=0;matches=[]
    while True:
        offset=data.find(needle,offset)
        if offset<0:break
        address=pe.base+section['rva']+offset
        expected=needle+b'\xe9'+struct.pack('<I',(frame_handler-address-10)&0xffffffff)
        if pe.at(address,10)==expected:matches.append(address)
        offset+=1
    return matches[0] if len(matches)==1 else None

def action_size(oracle,address):
    pe=oracle.generated;off=0
    while off<128:
        ins=next(oracle.md.disasm(pe.at(address+off,15),address+off,count=1),None)
        if ins is None:raise ValueError('undecodable compiler cleanup helper')
        off+=ins.size
        if ins.mnemonic.startswith('ret') or ins.mnemonic=='jmp':return off
        if ins.mnemonic.startswith('j'):raise ValueError('branching cleanup requires separate review')
    raise ValueError('compiler cleanup helper exceeds bounded extent')

def recover(oracle):
    native=descriptors(oracle.generated);original=descriptors(oracle.target)
    names=['__CxxFrameHandler','_CxxFrameHandler','CxxFrameHandler']
    candidates={s['address'] for n in names for s in oracle.native_by_name.get(n,[])}
    targets={a for n in names for a in oracle.named.get(n,())}
    audit={'native_descriptors':len(native),'original_descriptors':len(original),'accepted':[],'blocked':[]};oracle.eh_map={}
    if len(candidates)!=1 or len(targets)!=1:return audit
    for desc in native:desc['handler']=handler(oracle.generated,desc['address'],next(iter(candidates)))
    for desc in original:desc['handler']=handler(oracle.target,desc['address'],next(iter(targets)))
    if len(native)!=len(original) or any(d['handler'] is None for d in native+original):return audit
    # Match complete graph capsules by shape and cleanup semantics. Readonly
    # contribution order can differ under LTCG/ICF, so it is never an address
    # oracle. Ambiguous graph identities remain blocked.
    options={};evidence={}
    for a in native:
        options[a['address']]=set()
        for b in original:
            if [st for st,x in a['states']]!=[st for st,x in b['states']]:continue
            if any(bool(x)!=bool(y) for (_,x),(_,y) in zip(a['states'],b['states'])):continue
            mapping={a['address']:b['address'],a['words'][2]:b['words'][2],a['handler']:b['handler']}
            for (_,x),(_,y) in zip(a['states'],b['states']):mapping[x]=y
            oracle.eh_map=mapping;oracle.resolution_cache.clear();okay=True;checks=[]
            words=list(a['words']);words[2]=b['words'][2]
            if struct.pack('<7I',*words)!=oracle.target.at(b['address'],28):continue
            for (state,x),(_,y) in zip(a['states'],b['states']):
                if not x and not y:continue
                try:
                    size=action_size(oracle,x)
                    r=oracle.compare(y,{'address':x,'size':size,'name':'compiler cleanup','module':'compiler-EH'},size)
                    checks.append({'generated':x,'target':y,'size':size,'result':r['status']})
                    if r['status']!='exact':okay=False
                except ValueError:okay=False
            try:
                r=oracle.compare(b['handler'],{'address':a['handler'],'size':10,'name':'compiler frame handler','module':'compiler-EH'},10)
                if r['status']!='exact':okay=False
            except ValueError:okay=False
            if okay:
                options[a['address']].add(b['address'])
                evidence[a['address'],b['address']]={'mapping':mapping,'row':{'generated_info':a['address'],'target_info':b['address'],'generated_handler':a['handler'],'target_handler':b['handler'],'cleanup_checks':checks}}
    accepted={};used=set()
    while True:
        additions={a:next(iter(candidates-used)) for a,candidates in options.items() if a not in accepted and len(candidates-used)==1}
        target_counts={b:sum(v==b for v in additions.values()) for b in additions.values()}
        additions={a:b for a,b in additions.items() if target_counts[b]==1}
        if not additions:break
        accepted.update(additions);used.update(additions.values())
    final={}
    for a,b in accepted.items():
        entry=evidence[a,b]
        if any(x in final and final[x]!=y for x,y in entry['mapping'].items()):
            audit['blocked'].append({'generated_info':a,'reason':'conflicting shared graph identity'});continue
        final.update(entry['mapping']);audit['accepted'].append(entry['row'])
    for a,choices in options.items():
        if a not in accepted:audit['blocked'].append({'generated_info':a,'candidate_infos':sorted(choices),'reason':'graph does not match uniquely'})
    oracle.eh_map=final;oracle.resolution_cache.clear()
    return audit
