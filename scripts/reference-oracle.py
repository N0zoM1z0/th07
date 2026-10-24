#!/usr/bin/env python3
"""Strict complete-function comparison of independently linked TH07 source.

DIR32 fields are enumerated by the generated PE's base-relocation directory.
Relative branch/call fields come from reachable generated x86 instructions.
Only original named symbols, PE import identities, independently mapped literal
contents and same-function offsets resolve destinations. No target instruction
operand is used to choose a destination, and no relocation bytes are masked.
"""
from __future__ import annotations
import argparse,bisect,collections,csv,hashlib,json,re,struct,subprocess,sys,tomllib
from pathlib import Path
import capstone
from capstone.x86 import X86_OP_IMM,X86_OP_MEM
from pe_image import PE
from msvc_demangle import demangle_msvc
ROOT=Path(__file__).resolve().parents[1]
ORACLE_VERSION=3

def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def canonical(n):
    if n.startswith('?'):
        try:n=demangle_msvc(n.encode()).decode()
        except (ValueError,IndexError):pass
    if n.startswith('::'):n=n[2:]
    return n

def aliases(n):
    yield n
    if n.startswith('_'):yield n.lstrip('_')
    if n.startswith('_') and '@' in n:yield n[1:].rsplit('@',1)[0]
    x=canonical(n)
    yield x
    yield x.replace('operator_new','operator new').replace('operator_delete','operator delete')
    yield x.replace('`scalar_deleting_destructor\'','`scalar deleting destructor\'')

def union_size(ranges):
    total=0;end=-1
    for a,b in sorted(ranges):total+=max(0,b-max(a,end));end=max(end,b)
    return total

class Oracle:
    def __init__(self,build):
        self.build=Path(build);self.target=PE(ROOT/'resources/th07.exe');self.generated=PE(self.build/'th07.exe')
        expected=tomllib.loads((ROOT/'config/target.toml').read_text())['target']
        if digest(self.target.path)!=expected['sha256'] or len(self.target.data)!=expected['size']:raise ValueError('original target identity mismatch')
        self.receipt=json.loads((self.build/'build-receipt.json').read_text())
        lock=tomllib.loads((ROOT/'config/tools.lock.toml').read_text())['msvc70']
        if self.receipt['upstream_commit']!='4878bf0e42a9280ef44735eb0517d4fda7cb6e04':raise ValueError('unrecognized reference revision')
        from tool_identity import require_tool_identity
        require_tool_identity(self.receipt,lock)
        if self.receipt['wrapper_source_sha256']!='9d94512d1db673bcb91b34ee0d4ba5ca3ee2e01fd7ce80d09eaabdae2a1623fb':raise ValueError('unrecognized frontend source')
        source=Path(self.receipt['source_root'])
        if source.resolve()==ROOT and self.receipt.get('build_config_sha256')!=digest(ROOT/'config/native-build.toml'):raise ValueError('maintained compiler configuration changed since build')
        actual={str(p.relative_to(source)):digest(p) for base in [source/'src/th07',source/'resources/csv'] for p in base.rglob('*') if p.is_file()}
        if actual!={r['path']:r['sha256'] for r in self.receipt['source_tree']}:raise ValueError('source tree changed since build; rebuild required')
        for command in self.receipt['commands']:
            if digest(ROOT/command['object'])!=command['sha256']:raise ValueError('compiled object changed')
        for name,key in [('th07.faithful.exe','faithful_image_sha256'),('th07.map','map_sha256')]:
            if digest(self.build/name)!=self.receipt[key]:raise ValueError('build artifact changed: '+name)
        for name,key in [('th07.exe','image_sha256'),('th07.pdb','pdb_sha256'),('pdb-symbols.json','pdb_symbols_sha256')]:
            if digest(self.build/name)!=self.receipt[key]:raise ValueError('generated evidence digest mismatch: '+name)
        if self.generated.section_bytes('.text')!=PE(self.build/'th07.faithful.exe').section_bytes('.text'):raise ValueError('metadata link changed faithful code')
        from pdb_symbols import symbols,verify_image_binding,constant_float_types,data_type_sizes
        self.pdb_binding=verify_image_binding(self.build/'th07.pdb',self.generated)
        recorded=json.loads((self.build/'pdb-symbols.json').read_text())
        if symbols(self.build/'th07.pdb')!=recorded:raise ValueError('cached PDB metadata differs from current bounded parser')
        self.symbols=recorded['symbols']
        float_types=constant_float_types(self.build/'th07.pdb')
        self.const_float_widths={}
        for s in self.symbols:
            if s['name']=='__except_list' and s['offset']==0:
                s['address']=0  # VC7's absolute FS:[0] linker symbol.
            else:s['address']=self.generated.address(s['section'],s['offset']) if s['section'] else s['offset']
        checkout=ROOT/'_reference/some100-th07'
        if subprocess.check_output(['git','-C',str(checkout),'rev-parse','HEAD'],text=True).strip()!=self.receipt['upstream_commit']:raise ValueError('reference pin differs')
        if subprocess.check_output(['git','-C',str(checkout),'status','--porcelain','--untracked-files=no'],text=True).strip():raise ValueError('reference checkout has tracked changes')
        self.const_float_widths={s['address']:float_types[s['type_index']] for s in self.symbols if s['type']=='data' and s.get('type_index') in float_types and s['section']>0 and self.generated.sections[s['section']-1]['name']=='.rdata'}
        ref=checkout/'resources/csv'
        self.refs=list(csv.DictReader((ref/'funcs.csv').open(),delimiter='|'))
        self.named=collections.defaultdict(set);self.original_functions={};self.original_globals=[]
        for r in self.refs:
            a=int(r['address'],16);self.original_functions[a]=r
            for n in aliases(r['name']):self.named[n].add(a)
        for r in csv.DictReader((ref/'globals.csv').open(),delimiter='|'):
            a=int(r['address'],16);size=int(r['size'],0)
            for n in aliases(r['name']):self.named[n].add(a)
            self.original_globals.append((a,size,r['name']))
        for file in ['known-symbols.csv','known-globals.csv']:
            for r in csv.DictReader((ROOT/'config'/file).open()):
                for n in aliases(r['name']):self.named[n].add(int(r['address'],0))
        # Source annotations independently reconcile stale friendly CSV names.
        self.function_aliases=collections.defaultdict(set)
        for source in sorted((ROOT/'_reference/some100-th07/src/th07').rglob('*')):
            if source.suffix not in ('.hpp','.h','.cpp'):continue
            text=source.read_text(errors='replace')
            for m in re.finditer(r'// (FUNCTION|VTABLE|GLOBAL): TH07 (0x[0-9a-fA-F]+)[^\n]*\n',text):
                a=int(m[2],0);tail=text[m.end():m.end()+1200]
                tail=re.sub(r'//[^\n]*','',tail);tail=re.sub(r'/\*.*?\*/','',tail,flags=re.S)
                tail=re.sub(r'^\s*#pragma[^\n]*','',tail,flags=re.M)
                if m[1]=='FUNCTION':
                    decl=re.search(r'([~A-Za-z_$][\w:$~<>]*)\s*\(',tail)
                    if decl:
                        n=decl[1]
                        if n not in ('if','for','while','sizeof','switch'):
                            self.function_aliases[a].add(n)
                            for alias in aliases(n):self.named[alias].add(a)
                elif m[1]=='VTABLE':
                    decl=re.search(r'(?:class|struct)\s+([\w:]+)',tail)
                    if decl:
                        n=decl[1];self.named[n+"::`vftable'"].add(a);self.named['??_7'+n+'@@6B@'].add(a)
                else:
                    declaration=re.split(r'[;{]',tail,maxsplit=1)[0]
                    decl=re.search(r'\b(g_\w+|S\d+)\s*(?:\[|=|$)',declaration.strip())
                    if decl:self.named[decl[1]].add(a)
        for r in csv.DictReader((ROOT/'config/reccmp-relocations.csv').open()):
            a=int(r['address'],0);blob=bytes.fromhex(r['data_hex'])
            # Addend-specific legacy literal probes do not establish a base anchor.
            if blob and self.target.at(a,len(blob),allow_bss=True)!=blob:continue
            self.named[r['coff_symbol']].add(a)
        for file in ['vc7-prebuilt-library.csv','vc7-relocated-library.csv']:
            for r in csv.DictReader((ROOT/'config'/file).open()):self.named[r['symbol']].add(int(r['address'],0))
        self.native_by_name=collections.defaultdict(list);self.native_names=collections.defaultdict(set)
        for s in self.symbols:
            for n in aliases(s['name']):
                self.native_by_name[n].append(s);self.native_names[s['address']].add(n)
        self.native_imports=self.generated.imports();self.original_imports={n:a for a,n in self.target.imports().items()}
        self.native_relocations=sorted(self.generated.relocations())
        if not self.native_relocations:raise ValueError('generated image lacks complete base-relocation metadata')
        self.annotated_strings=collections.defaultdict(set)
        for source in (ROOT/'_reference/some100-th07/src/th07').rglob('*'):
            if source.suffix not in ('.cpp','.hpp','.h'):continue
            for match in re.finditer(r'// STRING: TH07 (0x[0-9a-fA-F]+)',source.read_text(errors='replace')):
                a=int(match[1],0);self.annotated_strings[self.target.cstring(a)+b'\0'].add(a)
        self.literal_maps={};self.literal_candidates=collections.defaultdict(set)
        for file in ['floats.csv','strings.csv']:
            # The upstream string export has an unescaped literal pipe in one
            # string. Its address is the first field and type/size are the last
            # two; display text is never used to recover target bytes.
            for line in (ref/file).read_bytes().splitlines()[1:]:
                if not line:continue
                prefix,kind,length=line.rsplit(b'|',2)
                a=int(prefix.split(b'|',1)[0],16);size=int(length,0)
                if kind not in (b'float',b'string'):raise ValueError('invalid literal metadata row')
                if size:self.literal_candidates[self.target.at(a,size,allow_bss=True)].add(a)
        self.string_anchors=[]
        for symbol in self.symbols:
            if symbol['type']!='public' or not symbol['name'].startswith('??_C@'):continue
            base=symbol['address'];blob=self.generated.cstring(base)+b'\0'
            annotated=self.annotated_strings.get(blob,set())
            candidates=annotated if len(annotated)==1 else self.literal_candidates.get(blob,set())
            target=next(iter(candidates)) if len(candidates)==1 else base if base in candidates else None
            if target is not None:self.string_anchors.append((base,target,len(blob)))
        self.original_ledger={int(r['address'],0):r for r in csv.DictReader((ROOT/'config/functions.csv').open())}
        self.ghidra={int(r['address'],0):r for r in csv.DictReader((ROOT/'config/ghidra-functions.csv').open())}
        self.md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);self.md.detail=True
        self.function_starts=sorted(self.original_functions)
        self.global_anchors=[]
        for a,size,name in self.original_globals:
            native={s['address'] for n in aliases(name) for s in self.native_by_name.get(n,[]) if s['type']=='data'}
            if len(native)==1:self.global_anchors.append((next(iter(native)),a,size,name))
        self.resolution_cache={};self.eh_map={}
        from compiler_statics import recover as static_guards
        self.static_map,self.static_audit=static_guards(self,tomllib.loads((ROOT/'config/compiler-statics.toml').read_text())['guards'],data_type_sizes(self.build/'th07.pdb'))
        from compiler_thunks import recover as crt_thunks
        self.thunk_map,self.thunk_audit=crt_thunks(self)
        from compiler_eh import recover
        self.eh_audit=recover(self)
    def name_targets(self,names):return set(a for n in names for a in self.named.get(n,()))
    def resolve(self,address,owner,target,size):
        if owner<=address<owner+size:return target+address-owner,'same-function-offset'
        if address in self.resolution_cache:return self.resolution_cache[address]
        result=self.resolve_external(address);self.resolution_cache[address]=result;return result
    def resolve_external(self,address):
        if address in self.eh_map:return self.eh_map[address],'validated-compiler-EH-graph'
        if address in getattr(self,'thunk_map',{}):return self.thunk_map[address],'complete-unique-CRT-tail-thunk-replay'
        if address in getattr(self,'static_map',{}):return self.static_map[address],'typed-adjacent-VC7-local-static-BSS-contribution'
        if address in self.native_imports:
            a=self.original_imports.get(self.native_imports[address])
            return a,'PE-import-identity' if a is not None else 'unresolved-import'
        names=self.native_names.get(address,set());choices=self.name_targets(names)
        if len(choices)==1:return next(iter(choices)),'independent-symbol-map'
        if address in choices:return address,'generated-linker-selects-mapped-symbol'
        if len(choices)>1:
            native_sizes={r['size'] for r in self.symbols if r['type']=='function' and r['address']==address}
            sized={a for a in choices if int((self.original_ledger.get(a) or self.ghidra.get(a) or {}).get('size',0)) in native_sizes}
            if len(sized)==1:return next(iter(sized)),'named-function-complete-extent'
        for n in names:
            if n.startswith('S') and n[1:].isdigit():
                possible=self.named.get('$'+n,set())
                if len(possible)==1:return next(iter(possible)),'mapped-static-initialization-guard'
            if n.startswith('__real@'):
                h=n[7:]
                if len(h) in (8,16) and re.fullmatch('[0-9a-fA-F]+',h):
                    blob=bytes.fromhex(h)[::-1];possible=self.literal_candidates.get(blob,set())
                    if len(possible)==1:return next(iter(possible)),'mapped-float-content'
                    if address in possible:return address,'generated-mapped-float-address'
            if n.startswith('??_C@'):
                blob=self.generated.cstring(address)+b'\0';annotated=self.annotated_strings.get(blob,set())
                possible=annotated if len(annotated)==1 else self.literal_candidates.get(blob,set())
                if len(possible)==1:return next(iter(possible)),'mapped-string-content'
                if address in possible:return address,'generated-mapped-string-address'
        # A named const f32/f64 object is not necessarily emitted under a
        # __real@ name. Compiler TPI proves its width/constness; only a unique,
        # aligned original typed literal can independently establish its anchor.
        width=getattr(self,'const_float_widths',{}).get(address)
        if width:
            blob=self.generated.at(address,width)
            section=self.target.section('.rdata');start=self.target.base+section['rva']
            choices={a for a in self.literal_candidates.get(blob,set()) if a%width==0 and start<=a and a+width<=start+section['raw_size']}
            if len(choices)==1:return next(iter(choices)),'PDB-const-scalar-unique-typed-literal'
        literal_offsets={target+address-base for base,target,length in getattr(self,'string_anchors',[]) if base<=address<base+length}
        if len(literal_offsets)==1:return next(iter(literal_offsets)),'named-string-literal-bounded-offset'
        anchors={(a+address-native,name) for native,a,size,name in self.global_anchors if native<=address<native+size}
        values={a for a,n in anchors}
        if len(values)==1:return next(iter(values)),'named-global-offset'
        return None,'unresolved:'+','.join(sorted(names)[:4])
    def reachable(self,address,size):
        code=self.generated.at(address,size);queue=[address];seen=set();instructions=[];unknown=[]
        relocset=set(self.native_relocations)
        while queue:
            pc=queue.pop()
            while address<=pc<address+size and pc not in seen:
                seen.add(pc);off=pc-address;ins=next(self.md.disasm(code[off:off+15],pc,count=1),None)
                if ins is None or pc+ins.size>address+size:unknown.append(pc);break
                instructions.append(ins);nxt=pc+ins.size
                jump=capstone.CS_GRP_JUMP in ins.groups;call=capstone.CS_GRP_CALL in ins.groups
                if jump:
                    if ins.operands and ins.operands[0].type==X86_OP_IMM:
                        dest=ins.operands[0].imm&0xffffffff
                        if address<=dest<address+size:queue.append(dest)
                    elif ins.operands and ins.operands[0].type==X86_OP_MEM:
                        table=ins.operands[0].mem.disp&0xffffffff
                        for i in range(4096):
                            at=table+i*4
                            if at not in relocset:break
                            dest=struct.unpack('<I',self.generated.at(at,4))[0]
                            if address<=dest<address+size:queue.append(dest)
                            else:break
                    if ins.mnemonic=='jmp':break
                if capstone.CS_GRP_RET in ins.groups or ins.mnemonic in ('int3','ud2'):break
                pc=nxt
        return instructions,unknown
    def compare(self,original,native,coverage_size):
        target=original;address=native['address'];size=native['size'];expected=self.target.at(target,size);code=bytearray(self.generated.at(address,size))
        row={'address':target,'name':native['name'],'native_address':address,'size':coverage_size,'compare_size':size,'module':native['module'],'status':'blocked','relocations':[],'unresolved':[]}
        if size<coverage_size:
            row['status']='extent-too-short';return row
        next_start=next((a for a in self.function_starts if a>target),target+size)
        extent_overlap=target+size>next_start
        if not extent_overlap and address==target and bytes(code)==expected:
            row['status']='exact';row['method']='linked-raw-full-extent'
            row['byte_differences']=0;row['structural_differences']=0
            return row
        row['method']='native-relocation-replay'
        instructions,unknown=self.reachable(address,size)
        if unknown:row['unresolved']+=['undecodable generated instruction '+hex(a) for a in unknown]
        lo=bisect.bisect_left(self.native_relocations,address);hi=bisect.bisect_left(self.native_relocations,address+size)
        occupied=set()
        for field in self.native_relocations[lo:hi]:
            off=field-address
            if off+4>size:row['unresolved'].append('DIR32 crosses complete procedure extent');continue
            destination=struct.unpack_from('<I',code,off)[0];resolved,evidence=self.resolve(destination,address,target,size)
            occupied.update(range(off,off+4))
            if resolved is None:row['unresolved'].append({'offset':off,'kind':'DIR32','generated_destination':destination,'reason':evidence});continue
            struct.pack_into('<I',code,off,resolved)
            row['relocations'].append({'offset':off,'kind':'DIR32','generated_destination':destination,'target':resolved,'evidence':evidence})
        for ins in instructions:
            if not any(g in ins.groups for g in [capstone.CS_GRP_JUMP,capstone.CS_GRP_CALL]):continue
            if not ins.operands or ins.operands[0].type!=X86_OP_IMM:continue
            off=ins.address-address+ins.imm_offset;width=ins.imm_size
            if width not in (1,2,4) or off+width>size:row['unresolved'].append('unsupported relative-branch field');continue
            destination=ins.operands[0].imm&0xffffffff;resolved,evidence=self.resolve(destination,address,target,size)
            occupied.update(range(off,off+width))
            if resolved is None:row['unresolved'].append({'offset':off,'kind':'REL','generated_destination':destination,'reason':evidence});continue
            value=resolved-(target+ins.address-address+ins.size)
            if not -(1<<(width*8-1))<=value<(1<<(width*8-1)):row['unresolved'].append('relative branch range overflow');continue
            code[off:off+width]=value.to_bytes(width,'little',signed=True)
            row['relocations'].append({'offset':off,'kind':'REL','width':width,'generated_destination':destination,'target':resolved,'evidence':evidence})
        differences=[i for i,(a,b) in enumerate(zip(code,expected)) if a!=b]
        row['structural_differences']=sum(i not in occupied for i in differences)
        row['byte_differences']=len(differences)
        row['status']='exact' if not differences and not row['unresolved'] else 'mismatch' if row['structural_differences'] else 'unresolved' if row['unresolved'] else 'relocation-mismatch'
        row['first_structural_differences']=[{'offset':i,'generated':code[i],'target':expected[i]} for i in differences if i not in occupied][:16]
        if extent_overlap:row['status']='extent-overlap'
        row['first_differences']=[{'offset':i,'generated':code[i],'target':expected[i]} for i in differences[:16]]
        return row
    def measure(self):
        rows=[]
        for r in self.refs:
            if r['type']!='function':continue
            original=int(r['address'],16);observed=self.original_ledger.get(original) or self.ghidra.get(original)
            size=int(observed['size']) if observed else 0
            wanted=self.function_aliases.get(original,set())|{r['name']}
            candidates=[s for n in wanted for s in self.native_by_name[n] if s['type']=='function' and s['size']]
            candidates=list({(s['address'],s['size']):s for s in candidates}.values());reports=[]
            for candidate in candidates:
                try:reports.append(self.compare(original,candidate,size or candidate['size']))
                except (ValueError,struct.error) as e:reports.append({'address':original,'name':r['name'],'size':size,'status':'error','error':str(e)})
            rank={'exact':5,'unresolved':4,'relocation-mismatch':3,'mismatch':3,'extent-too-short':2,'extent-overlap':1,'error':0}
            row=max(reports,key=lambda f:(rank[f['status']],-f.get('byte_differences',10**9))) if reports else {'address':original,'name':r['name'],'size':size,'status':'missing-symbol'}
            rows.append(row)
        total=union_size((r['address'],r['address']+r['size']) for r in rows)
        exact=union_size((r['address'],r['address']+r['size']) for r in rows if r['status']=='exact')
        return {'build_kind':'unmodified-reference' if Path(self.receipt['source_root']).resolve()==(ROOT/'_reference/some100-th07').resolve() else 'maintained-source','oracle_version':ORACLE_VERSION,'reference_commit':self.receipt['upstream_commit'],'target_sha256':digest(self.target.path),'profile':self.receipt['profile'],'scope':'upstream function annotations; provisional origin/boundary review separate','functions':rows,'function_count':len(rows),'function_bytes':total,'exact_bytes':exact,'exact_percent':100*exact/total,'statuses':dict(collections.Counter(r['status'] for r in rows)),'compiler_eh':self.eh_audit,'compiler_statics':self.static_audit,'compiler_thunks':self.thunk_audit}

def main():
    p=argparse.ArgumentParser();p.add_argument('--build-dir',type=Path,default=ROOT/'build/reference');p.add_argument('--output',type=Path,default=ROOT/'build/reference-baseline.json');a=p.parse_args();result=Oracle(a.build_dir).measure();a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='functions'},indent=2))
if __name__=='__main__':main()
