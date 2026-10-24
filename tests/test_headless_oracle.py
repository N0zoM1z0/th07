"""Synthetic positive/negative controls; no proprietary target is needed."""
import importlib.util,json,struct,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'scripts'))
from pdb_symbols import MSF,MAGIC,symbols,records,verify_image_binding
from pe_image import PE

def record(kind,body):
    return struct.pack('<HH',len(body)+2,kind)+body

def msf(streams):
    block=512;directory=bytearray(struct.pack('<I',len(streams))+b''.join(struct.pack('<I',len(s) if s is not None else 0xffffffff) for s in streams));chunks=[];next_block=4
    for stream in streams:
        if stream is None:continue
        for start in range(0,len(stream),block):
            directory+=struct.pack('<I',next_block);next_block+=1;chunks.append(stream[start:start+block].ljust(block,b'\0'))
    if len(directory)>block:raise ValueError('fixture directory too large')
    header=(MAGIC+struct.pack('<IIIIII',block,1,next_block,len(directory),0,3)).ljust(block,b'\0')
    return header+bytes(block)+directory.ljust(block,b'\0')+struct.pack('<I',2).ljust(block,b'\0')+b''.join(chunks)

def pdb(proc_size=6,kind=0x1110,truncate_record=False):
    name=b'Example::Run'
    body=struct.pack('<IIIIIIIIHB',0,0,0,proc_size,0,proc_size,0x1000,0x10,1,0)
    body+=bytes([len(name)])+name if kind<0x1100 else name+b'\0'
    rec=record(kind,body);module=struct.pack('<I',4)+rec
    if truncate_record:module=module[:-1]
    modinfo=bytearray(64);struct.pack_into('<H',modinfo,34,4);struct.pack_into('<I',modinfo,36,4+len(rec))
    modinfo+=b'unit.obj\0unit.obj\0';modinfo+=bytes((-len(modinfo))%4)
    dbi=bytearray(64);struct.pack_into('<I',dbi,0,0xffffffff);struct.pack_into('<H',dbi,20,0xffff);struct.pack_into('<I',dbi,24,len(modinfo));dbi+=modinfo
    return msf([None,None,None,dbi,module])

def image(code,reloc_type=3,duplicate=False):
    data=bytearray(0xa00);data[:2]=b'MZ';struct.pack_into('<I',data,60,0x80);data[0x80:0x84]=b'PE\0\0'
    struct.pack_into('<HH',data,0x84,0x14c,3);struct.pack_into('<H',data,0x94,224);opt=0x98
    struct.pack_into('<H',data,opt,0x10b);struct.pack_into('<I',data,opt+28,0x400000);struct.pack_into('<I',data,opt+56,0x4000)
    struct.pack_into('<II',data,opt+96+5*8,0x2000,12)
    for i,(name,rva,vs,raw,rs,flags) in enumerate([(b'.text',0x1000,0x200,0x400,0x200,0x60000020),(b'.reloc',0x2000,12,0x600,0x200,0x42000040),(b'.data',0x3000,0x100,0,0,0xc0000080)]):
        struct.pack_into('<8sIIII',data,opt+224+i*40,name,vs,rva,rs,raw);struct.pack_into('<I',data,opt+224+i*40+36,flags)
    data[0x400:0x400+len(code)]=code;entry=(reloc_type<<12)|1
    struct.pack_into('<IIHH',data,0x600,0x1000,12,entry,entry if duplicate else 0)
    return bytes(data)

class Fixture(unittest.TestCase):
    def setUp(self):self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
    def tearDown(self):self.temp.cleanup()
    def write(self,name,data):p=self.root/name;p.write_bytes(data);return p

class ParserTests(Fixture):
    def test_msf_procedure_extent(self):
        r=symbols(self.write('one.pdb',pdb()));self.assertEqual(len(r['symbols']),1)
        self.assertEqual((r['symbols'][0]['name'],r['symbols'][0]['size'],r['symbols'][0]['offset']),('Example::Run',6,0x10))
    def test_vc7_pascal_symbol_name(self):
        self.assertEqual(symbols(self.write('old.pdb',pdb(kind=0x100b)))['symbols'][0]['name'],'Example::Run')
    def test_msf_truncated_header(self):
        with self.assertRaises(ValueError):MSF(MAGIC)
    def test_msf_invalid_magic(self):
        data=bytearray(pdb());data[0]^=1
        with self.assertRaises(ValueError):MSF(data)
    def test_msf_invalid_block_geometry(self):
        data=bytearray(pdb());struct.pack_into('<I',data,32,123)
        with self.assertRaises(ValueError):MSF(data)
    def test_msf_bad_directory_block(self):
        data=bytearray(pdb());struct.pack_into('<I',data,3*512,100000)
        with self.assertRaises(ValueError):MSF(data)
    def test_truncated_symbol_record(self):
        with self.assertRaises(ValueError):symbols(self.write('bad.pdb',pdb(truncate_record=True)))
    def test_record_length_fails_closed(self):
        with self.assertRaises(ValueError):list(records(struct.pack('<HH',0,0x1110)))
    def test_pe_mapping_and_base_relocations(self):
        p=PE(self.write('one.exe',image(b'\xb8\0\x21\x40\0\xc3')))
        self.assertEqual(p.at(0x401000,1),b'\xb8');self.assertEqual(p.relocations(),{0x401001})
    def test_pe_bss_is_explicit(self):
        p=PE(self.write('one.exe',image(b'\xc3')))
        with self.assertRaises(ValueError):p.at(0x403000,4)
        self.assertEqual(p.at(0x403000,4,allow_bss=True),bytes(4))
    def test_pe_cross_section_read_rejected(self):
        p=PE(self.write('one.exe',image(b'\xc3')))
        with self.assertRaises(ValueError):p.at(0x4011ff,4)
    def test_pe_unsupported_relocation(self):
        p=PE(self.write('one.exe',image(b'\xc3',reloc_type=5)))
        with self.assertRaises(ValueError):p.relocations()
    def test_pe_duplicate_relocation(self):
        p=PE(self.write('one.exe',image(b'\xc3',duplicate=True)))
        with self.assertRaises(ValueError):p.relocations()

class BindingTests(Fixture):
    def pair(self,wrong_guid=False,wrong_age=False,wrong_mapping=False):
        guid=bytes(range(16));info=struct.pack('<III',20000404,0,2)+guid
        pdb_path=self.write('bind.pdb',msf([None,info]))
        data=bytearray(image(b'\xc3'));opt=0x98
        struct.pack_into('<II',data,opt+96+6*8,0x2020,28)
        cv=b'RSDS'+(bytes(16) if wrong_guid else guid)+struct.pack('<I',3 if wrong_age else 2)+b'th07.pdb\0'
        struct.pack_into('<IIHHIIII',data,0x620,0,0,0,0,2,len(cv),0x2060,0x661 if wrong_mapping else 0x660)
        data[0x660:0x660+len(cv)]=cv
        return pdb_path,PE(self.write('bind.exe',data))
    def test_pdb_image_binding(self):
        self.assertEqual(verify_image_binding(*self.pair())['age'],2)
    def test_wrong_pdb_guid_rejected(self):
        with self.assertRaises(ValueError):verify_image_binding(*self.pair(wrong_guid=True))
    def test_wrong_pdb_age_rejected(self):
        with self.assertRaises(ValueError):verify_image_binding(*self.pair(wrong_age=True))
    def test_debug_rva_file_disagreement_rejected(self):
        with self.assertRaises(ValueError):verify_image_binding(*self.pair(wrong_mapping=True))

class RelocationTests(Fixture):
    def oracle(self,generated_code,target_code):
        import capstone
        spec=importlib.util.spec_from_file_location('native_oracle',ROOT/'scripts/reference-oracle.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
        o=m.Oracle.__new__(m.Oracle);o.generated=PE(self.write('generated.exe',image(generated_code)));o.target=PE(self.write('target.exe',image(target_code)))
        o.native_relocations=sorted(o.generated.relocations());o.function_starts=[0x401000,0x401020]
        o.md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);o.md.detail=True
        o.resolution_cache={};o.eh_map={};o.native_imports={};o.original_imports={};o.native_names={0x402100:{'g_Value'}};o.named={'g_Value':{0x403000}};o.global_anchors=[];o.original_ledger={};o.ghidra={};o.symbols=[]
        return o
    def test_complete_dir32_replay(self):
        o=self.oracle(b'\xb8'+struct.pack('<I',0x402100)+b'\xc3',b'\xb8'+struct.pack('<I',0x403000)+b'\xc3')
        r=o.compare(0x401000,{'address':0x401000,'size':6,'name':'unit','module':'fixture'},6)
        self.assertEqual(r['status'],'exact');self.assertEqual(r['relocations'][0]['target'],0x403000)
    def test_wrong_named_destination_not_solved_from_target(self):
        o=self.oracle(b'\xb8'+struct.pack('<I',0x402100)+b'\xc3',b'\xb8'+struct.pack('<I',0x403000)+b'\xc3');o.named['g_Value']={0x403001}
        self.assertNotEqual(o.compare(0x401000,{'address':0x401000,'size':6,'name':'unit','module':'fixture'},6)['status'],'exact')
    def test_unresolved_relocation_has_no_credit(self):
        o=self.oracle(b'\xb8'+struct.pack('<I',0x402100)+b'\xc3',b'\xb8'+struct.pack('<I',0x403000)+b'\xc3');o.named={}
        r=o.compare(0x401000,{'address':0x401000,'size':6,'name':'unit','module':'fixture'},6)
        self.assertNotEqual(r['status'],'exact');self.assertTrue(r['unresolved'])
    def test_short_procedure_cannot_match_target_prefix(self):
        o=self.oracle(b'\xc3',b'\xc3')
        self.assertEqual(o.compare(0x401000,{'address':0x401000,'size':1,'name':'unit','module':'fixture'},2)['status'],'extent-too-short')
    def test_trailing_table_corruption_rejected(self):
        o=self.oracle(b'\xc3'+bytes(8),b'\xc3'+bytes(7)+b'\x01')
        r=o.compare(0x401000,{'address':0x401000,'size':9,'name':'table','module':'fixture'},1)
        self.assertNotEqual(r['status'],'exact')
    def test_complete_raw_linked_match(self):
        code=b'\xb8'+struct.pack('<I',0x402100)+b'\xc3';o=self.oracle(code,code)
        self.assertEqual(o.compare(0x401000,{'address':0x401000,'size':6,'name':'unit','module':'fixture'},6)['method'],'linked-raw-full-extent')

if __name__=='__main__':unittest.main()
