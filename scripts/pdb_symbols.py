"""Bounded MSF 7 / CodeView procedure reader for VC7 linker PDBs.

Layouts are independently documented by LLVM RawTypes.h (DbiStreamHeader,
ModuleInfoHeader) and SymbolRecord.h (ProcSym/DataSym/PublicSym32). This reads
compiler metadata, never original target operands or disassembler hypotheses.
"""
from __future__ import annotations
import json,struct,sys
from pathlib import Path
MAGIC=b'Microsoft C/C++ MSF 7.00\r\n\x1aDS\0\0\0'

def u16(data,offset):return struct.unpack_from('<H',data,offset)[0]
def u32(data,offset):return struct.unpack_from('<I',data,offset)[0]
def checked(data,offset,size):
    if offset<0 or size<0 or offset+size>len(data):raise ValueError('truncated or out-of-range MSF/CodeView data')
    return data[offset:offset+size]
def zstring(data,offset):
    end=data.find(b'\0',offset)
    if offset<0 or end<offset:raise ValueError('unterminated CodeView name')
    return data[offset:end].decode('utf-8',errors='replace'),end+1

class MSF:
    def __init__(self,data):
        if checked(data,0,32)!=MAGIC:raise ValueError('expected MSF 7 linker PDB')
        self.data=data;self.block_size,self.free_map,self.block_count,ds,_,bm=struct.unpack('<IIIIII',checked(data,32,24))
        if self.block_size not in (512,1024,2048,4096) or self.block_count*self.block_size>len(data):raise ValueError('invalid MSF block geometry')
        if ds>len(data):raise ValueError('MSF directory size exceeds file')
        block_count=(ds+self.block_size-1)//self.block_size
        if block_count*4>self.block_size:raise ValueError('unsupported multilevel MSF directory block map')
        indices=struct.unpack('<'+'I'*block_count,checked(data,bm*self.block_size,block_count*4))
        directory=self.blocks(indices,ds)
        count=u32(checked(directory,0,4),0)
        if count>len(directory)//4:raise ValueError('invalid MSF stream count')
        sizes=struct.unpack('<'+'I'*count,checked(directory,4,count*4));offset=4+count*4;self.streams=[]
        for size in sizes:
            if size==0xffffffff:self.streams.append(None);continue
            if size>len(data):raise ValueError('MSF stream size exceeds file')
            n=(size+self.block_size-1)//self.block_size
            ids=struct.unpack('<'+'I'*n,checked(directory,offset,n*4));offset+=n*4
            self.streams.append(self.blocks(ids,size))
    def blocks(self,indices,size):
        if any(i>=self.block_count for i in indices):raise ValueError('MSF block index outside file')
        return b''.join(checked(self.data,i*self.block_size,self.block_size) for i in indices)[:size]
    def stream(self,index):
        if index<0 or index>=len(self.streams) or self.streams[index] is None:raise ValueError('invalid or absent PDB stream')
        return self.streams[index]

def records(data,start=0,limit=None):
    end=len(data) if limit is None else limit
    if end>len(data):raise ValueError('symbol stream extent exceeds stream')
    off=start
    while off<end:
        length,kind=struct.unpack('<HH',checked(data,off,4))
        if length<2 or off+length+2>end:raise ValueError('invalid CodeView record extent')
        yield off,kind,checked(data,off+4,length-2)
        off+=length+2

def name_at(body,offset,kind):
    if kind<0x1100:
        n=checked(body,offset,1)[0];return checked(body,offset+1,n).decode('utf-8',errors='replace')
    return zstring(body,offset)[0]

def symbols(path):
    pdb=MSF(Path(path).read_bytes());dbi=pdb.stream(3);checked(dbi,0,64)
    mod_size=u32(dbi,24);module_end=64+mod_size;checked(dbi,64,mod_size)
    modules=[];off=64
    while off<module_end:
        header=checked(dbi,off,64);stream=u16(header,34);sym_size=u32(header,36)
        name,next_off=zstring(dbi,off+64);obj,next_off=zstring(dbi,next_off)
        if next_off>module_end:raise ValueError('module name escapes DBI module region')
        modules.append({'module':name,'object':obj,'stream':stream,'sym_size':sym_size});off=(next_off+3)&~3
    out=[]
    def scan(data,start,limit,module):
        for offset,kind,body in records(data,start,limit):
            row={'record_offset':offset,'kind':kind,'module':module}
            if kind in (0x110f,0x1110,0x100a,0x100b):
                checked(body,0,35);row.update(type='function',size=u32(body,12),offset=u32(body,28),section=u16(body,32),name=name_at(body,35,kind))
            elif kind in (0x110c,0x110d,0x1007,0x1008):
                checked(body,0,10);row.update(type='data',type_index=u32(body,0),offset=u32(body,4),section=u16(body,8),name=name_at(body,10,kind))
            elif kind in (0x110e,0x1009):
                checked(body,0,10);row.update(type='public',offset=u32(body,4),section=u16(body,8),name=name_at(body,10,kind))
            elif kind in (0x1102,0x0206):
                checked(body,0,21);row.update(type='thunk',size=u16(body,18),offset=u32(body,12),section=u16(body,16),name=name_at(body,21,kind))
            else:continue
            out.append(row)
    for m in modules:
        if m['stream']==0xffff or not m['sym_size']:continue
        data=pdb.stream(m['stream'])
        if u32(checked(data,0,4),0)!=4:raise ValueError('unsupported CodeView module signature')
        scan(data,4,m['sym_size'],m['module'])
    index=u16(dbi,20)
    if index!=0xffff:scan(pdb.stream(index),0,None,'<global-records>')
    return {'modules':modules,'symbols':out}

if __name__=='__main__':
    result=symbols(Path(sys.argv[1]));Path(sys.argv[2]).write_text(json.dumps(result,indent=2)+'\n')
    print(f"PDB: {len(result['modules'])} modules, {len(result['symbols'])} procedure/data/public records")


def verify_image_binding(path,pe):
    """Require the PDB Info GUID/age to match the generated image's RSDS record."""
    pdb=MSF(Path(path).read_bytes());info=pdb.stream(1);checked(info,0,28)
    rva,size=pe.directory(6);records=[]
    if size%28:raise ValueError('malformed PE debug-directory extent')
    for off in range(0,size,28):
        entry=pe.at(pe.base+rva+off,28)
        if u32(entry,12)!=2:continue
        length=u32(entry,16);address=u32(entry,20);raw=u32(entry,24)
        data=pe.at(pe.base+address,length)
        if data!=checked(pe.data,raw,length):raise ValueError('debug RVA/file mapping disagrees')
        if data[:4]!=b'RSDS' or len(data)<24:raise ValueError('unsupported generated CodeView binding')
        records.append(data)
    if len(records)!=1:raise ValueError('missing or ambiguous generated PDB binding')
    cv=records[0]
    if cv[4:20]!=info[12:28] or u32(cv,20)!=u32(info,8):raise ValueError('PDB GUID/age does not match generated image')
    return {'guid':cv[4:20].hex(),'age':u32(cv,20)}


def type_records(path):
    """Read bounded compiler TPI records without consulting target instructions."""
    data=MSF(Path(path).read_bytes()).stream(2)
    checked(data,0,20)
    header,begin,end,size=struct.unpack_from('<IIII',data,4)
    if header<20 or begin>end:raise ValueError('invalid TPI header')
    types={}
    for index,(_,kind,body) in enumerate(records(data,header,header+size),begin):
        if index>=end:raise ValueError('excess TPI records')
        types[index]=(kind,body)
    if len(types)!=end-begin:raise ValueError('missing TPI records')
    return types


def constant_float_types(path):
    """Read const scalar floating types from compiler TPI metadata only."""
    types=type_records(path)
    result={}
    for index,(kind,body) in types.items():
        seen=set();at=index;is_const=False
        while at>=0x1000:
            if at in seen:raise ValueError('cyclic TPI modifier chain')
            seen.add(at);entry=types.get(at)
            if entry is None:raise ValueError('TPI modifier points outside type stream')
            leaf,payload=entry
            if leaf!=0x1001:break
            checked(payload,0,6);is_const |= bool(u16(payload,4)&1);at=u32(payload,0)
        if is_const and at in (0x40,0x41):result[index]=4 if at==0x40 else 8
    return result


def data_type_sizes(path):
    """Sizes needed for scalar and concrete class local-static storage."""
    result={0x22:4,0x74:4,0x75:4}
    for index,(kind,body) in type_records(path).items():
        offset=16 if kind in (0x1504,0x1505) else 8 if kind==0x1506 else None
        if offset is None:continue
        checked(body,0,offset+2)
        if u16(body,2)&0x80:continue  # forward reference, no concrete extent
        size=u16(body,offset)
        if size<0x8000:result[index]=size
        elif size==0x8004:result[index]=u32(checked(body,offset+2,4),0)
    return result
