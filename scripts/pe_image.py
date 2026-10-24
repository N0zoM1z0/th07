"""Strict PE32 file mappings, import identities and linker base-relocation fields."""
from __future__ import annotations
import struct
from pathlib import Path
from pdb_symbols import checked,u16,u32
class PE:
    def __init__(self,path):
        self.path=Path(path);self.data=self.path.read_bytes();d=self.data
        if checked(d,0,2)!=b'MZ':raise ValueError('expected DOS header')
        pe=u32(d,60)
        if checked(d,pe,4)!=b'PE\0\0' or u16(d,pe+4)!=0x14c:raise ValueError('expected i386 PE')
        self.optional=pe+24
        if u16(d,self.optional)!=0x10b:raise ValueError('expected PE32 optional header')
        self.base=u32(d,self.optional+28);self.image_size=u32(d,self.optional+56);self.sections=[]
        table=self.optional+u16(d,pe+20)
        for i in range(u16(d,pe+6)):
            row=checked(d,table+i*40,40);name,vs,rva,rs,raw=struct.unpack_from('<8sIIII',row)
            self.sections.append({'name':name.rstrip(b'\0').decode('ascii'),'virtual_size':vs,'rva':rva,'raw_size':rs,'raw':raw,'flags':u32(row,36)})
    def address(self,section,offset):
        if not 1<=section<=len(self.sections):raise ValueError('symbol section outside PE')
        s=self.sections[section-1]
        if not 0<=offset<max(s['virtual_size'],s['raw_size']):raise ValueError('symbol offset outside PE section')
        return self.base+s['rva']+offset
    def section(self,name):
        result=[s for s in self.sections if s['name']==name]
        if len(result)!=1:raise ValueError('missing/ambiguous PE section '+name)
        return result[0]
    def section_bytes(self,name):
        s=self.section(name);return self.at(self.base+s['rva'],s['virtual_size'])
    def at(self,address,size,*,allow_bss=False):
        rva=address-self.base
        for s in self.sections:
            offset=rva-s['rva']
            if 0<=offset and offset+size<=max(s['virtual_size'],s['raw_size']):
                raw_count=max(0,min(size,s['raw_size']-offset))
                if raw_count<size and not allow_bss:raise ValueError('read exceeds file-backed PE bytes')
                raw=checked(self.data,s['raw']+offset,raw_count) if raw_count else b''
                return raw+bytes(size-raw_count)
        raise ValueError(f'unmapped PE range {address:#x}+{size:#x}')
    def cstring(self,address,limit=65536):
        result=bytearray()
        for i in range(limit):
            c=self.at(address+i,1)
            if c==b'\0':return bytes(result)
            result+=c
        raise ValueError('unterminated PE string')
    def directory(self,index):return struct.unpack('<II',checked(self.data,self.optional+96+index*8,8))
    def imports(self):
        rva,size=self.directory(1);out={}
        if not rva:return out
        for i in range(size//20):
            orig,_,_,dll,iat=struct.unpack('<IIIII',self.at(self.base+rva+i*20,20))
            if not any([orig,dll,iat]):return out
            library=self.cstring(self.base+dll).decode('ascii').lower()
            for j in range(65536):
                value=struct.unpack('<I',self.at(self.base+(orig or iat)+j*4,4))[0]
                if not value:break
                name='#'+str(value&0xffff) if value&0x80000000 else self.cstring(self.base+value+2).decode('ascii')
                out[self.base+iat+j*4]=library+'!'+name
            else:raise ValueError('unterminated PE import thunk array')
        raise ValueError('unterminated PE import directory')
    def relocations(self):
        rva,size=self.directory(5);off=0;out=set()
        while off<size:
            page,blocksize=struct.unpack('<II',self.at(self.base+rva+off,8))
            if blocksize<8 or blocksize%2 or off+blocksize>size:raise ValueError('invalid PE relocation block')
            entries=self.at(self.base+rva+off+8,blocksize-8)
            for i in range(0,len(entries),2):
                value=u16(entries,i);kind=value>>12
                if kind==0:continue
                if kind!=3:raise ValueError('unsupported PE base relocation kind')
                address=self.base+page+(value&0xfff)
                if address in out:raise ValueError('duplicate PE relocation field')
                self.at(address,4);out.add(address)
            off+=blocksize
        return out
