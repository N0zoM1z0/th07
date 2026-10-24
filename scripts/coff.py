"""Small bounded i386 COFF reader. No target bytes are used to infer relocation values."""
import struct
from pathlib import Path
from msvc_demangle import demangle_msvc

class Object:
    def __init__(self,path):
        self.path=Path(path); self.data=self.path.read_bytes();d=self.data
        machine,count,_,ptr,ns,opt,_=struct.unpack_from('<HHIIIHH',d)
        if machine!=0x14c or opt:raise ValueError('expected i386 COFF')
        strings=d[ptr+ns*18:];self.sections=[]
        for i in range(count):
            n,_,_,size,raw,rel,_,nr,_,flags=struct.unpack_from('<8sIIIIIIHHI',d,20+i*40)
            self.sections.append({'name':n.rstrip(b'\0').decode(),'size':size,'data':d[raw:raw+size] if raw else b'', 'flags':flags,'relocations':[struct.unpack_from('<IIH',d,rel+j*10) for j in range(nr)]})
        self.symbols={};i=0
        while i<ns:
            n,v,sec,t,storage,aux=struct.unpack_from('<8sIhHBB',d,ptr+i*18)
            if n[:4]==b'\0'*4:
                off=struct.unpack_from('<I',n,4)[0];n=strings[off:].split(b'\0',1)[0]
            else:n=n.rstrip(b'\0')
            name=n.decode('ascii',errors='replace');dem=demangle_msvc(n).decode('ascii',errors='replace')
            for prefix,label in [('??_G', "`scalar_deleting_destructor'"),('??_7', "`vftable'")]:
                if name.startswith(prefix):
                    scope=name[len(prefix):].split('@@',1)[0].split('@')
                    dem='::'.join(scope[::-1])+'::'+label
            if name.startswith('??2@'):dem='operator_new'
            if name.startswith('??3@'):dem='operator_delete'
            if name.startswith('?') and '@?1?' in name:dem=name[1:].split('@',1)[0]

            if dem.startswith('::'):dem=dem[2:]
            size=struct.unpack_from('<I',d,ptr+(i+1)*18+4)[0] if aux and t==0x20 else None
            self.symbols[i]={'name':name,'demangled':dem,'value':v,'section':sec,'type':t,'storage':storage,'size':size,'index':i}
            i+=aux+1
        # Inline COMDAT definitions can omit function auxiliary records. Their
        # complete section is admissible only when it contains one function.
        for sym in self.symbols.values():
            if sym['type']!=0x20 or sym['section']<=0 or sym['size'] is not None:continue
            sec=self.sections[sym['section']-1]
            peers=[s for s in self.symbols.values() if s['type']==0x20 and s['section']==sym['section']]
            if len(peers)==1 and sym['value']==0 and sec['flags']&0x1000 and sec['flags']&0x20:
                sym['size']=sec['size']
    def functions(self):
        return [s for s in self.symbols.values() if s['type']==0x20 and s['section']>0 and s['size']]
    def body(self,s):
        sec=self.sections[s['section']-1];start=s['value'];size=s['size']
        if start+size>len(sec['data']):raise ValueError('function exceeds section')
        rel=[{'offset':o-start,'symbol':self.symbols[idx],'type_id':t} for o,idx,t in sec['relocations'] if start<=o<start+size]
        return bytearray(sec['data'][start:start+size]),rel
