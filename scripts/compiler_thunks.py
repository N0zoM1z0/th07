"""Recover a CRT forwarding entry by complete, unique tail-thunk replay."""
import struct

def recover(oracle):
    mappings={};audit=[]
    # The original friendly CSV calls both atol's implementation and atoi's
    # forwarding entry 'atol'. Native VC7 symbols distinguish the identities.
    natives=[r for r in oracle.symbols if r['type'] in ('function','thunk') and r['name'].lstrip('_')=='atoi' and r.get('size')==5]
    for native in natives:
        address=native['address'];code=oracle.generated.at(address,5)
        if code[0]!=0xe9:continue
        destination=(address+5+struct.unpack_from('<i',code,1)[0])&0xffffffff
        target,evidence=oracle.resolve_external(destination)
        if target is None:continue
        candidates=[]
        for start,row in oracle.original_ledger.items():
            if row['status']!='library' or int(row['size'])!=5:continue
            expected=b'\xe9'+struct.pack('<I',(target-start-5)&0xffffffff)
            if oracle.target.at(start,5)==expected:candidates.append(start)
        if len(candidates)!=1:continue
        mappings[address]=candidates[0]
        audit.append({'name':native['name'],'generated':address,'target':candidates[0],'size':5,'generated_callee':destination,'target_callee':target,'callee_evidence':evidence,'evidence':'complete-unique-CRT-tail-thunk-replay'})
    return mappings,audit
