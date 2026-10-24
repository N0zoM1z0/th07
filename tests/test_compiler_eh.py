"""Complete synthetic EH capsules, including mixed COFF/LTCG aliases."""
import importlib.util,struct,sys,unittest
from pathlib import Path
import capstone
from test_headless_oracle import Fixture,image
from pe_image import PE
from compiler_eh import recover,MAGIC
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('eh_test_oracle',ROOT/'scripts/reference-oracle.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)

class CapsuleTests(Fixture):
    def make_image(self,name,count,target=False,changed_cleanup=None,wrong_handler=None):
        code=bytearray(0x200);readonly=bytearray(0x200)
        for i in range(count):
            handler=0x401080+i*0x20 if target else 0x401000+i*0x20
            info=0x402100+i*0x40 if target else 0x402000+i*0x40
            action=0x401140+i*0x10 if target else 0x401100+i*0x10
            frame=0x4011e4 if i==wrong_handler else 0x4011e0
            code[handler-0x401000:handler-0x401000+10]=b'\xb8'+struct.pack('<I',info)+b'\xe9'+struct.pack('<i',frame-handler-10)
            # Real cleanup code: free the saved allocation and return.
            displacement=0xec if i==changed_cleanup else 0xf0
            cleanup=b'\xff\x75'+bytes([displacement])+b'\xe8'+struct.pack('<i',0x4011d0-action-8)+b'\x59\xc3'
            code[action-0x401000:action-0x401000+10]=cleanup
            offset=info-0x402000
            struct.pack_into('<7I',readonly,offset,MAGIC,1,info+0x20,0,0,0,0)
            struct.pack_into('<iI',readonly,offset+0x20,-1,action)
        code[0x1d0]=0xc3;code[0x1e0]=0xc3
        data=bytearray(image(code));data[0x98+224+40:0x98+224+48]=b'.rdata\0\0'
        struct.pack_into('<I',data,0x98+224+40+8,0x200)
        struct.pack_into('<II',data,0x98+96+5*8,0,0)
        data[0x600:0x800]=readonly
        return PE(self.write(name,data))
    def oracle(self,native=2,original=1,**changes):
        o=module.Oracle.__new__(module.Oracle)
        o.generated=self.make_image('native.exe',native,**changes)
        o.target=self.make_image('original.exe',original,target=True)
        o.md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32);o.md.detail=True
        o.native_relocations=[0x401001+i*0x20 for i in range(native)]
        o.function_starts=sorted([0x401000+i*0x20 for i in range(native)]+[0x401100+i*0x10 for i in range(native)]+[0x4011d0,0x4011e0])
        o.native_by_name={'__CxxFrameHandler':[{'address':0x4011e0}]}
        o.native_names={0x4011d0:{'operator delete'},0x4011e0:{'__CxxFrameHandler'}}
        o.named={'operator delete':{0x4011d0},'__CxxFrameHandler':{0x4011e0}}
        o.resolution_cache={};o.eh_map={};o.native_imports={};o.original_imports={}
        o.global_anchors=[];o.original_ledger={};o.ghidra={};o.symbols=[]
        return o
    def test_equal_count_complete_capsule(self):
        o=self.oracle(native=1);audit=recover(o)
        self.assertEqual(len(audit['accepted']),1);self.assertEqual(audit['blocked'],[])
    def test_two_complete_native_capsules_can_alias_one_original(self):
        o=self.oracle();audit=recover(o)
        self.assertEqual(len(audit['accepted']),2);self.assertEqual(audit['blocked'],[])
        self.assertEqual(o.eh_map[0x402000],0x402100)
        self.assertEqual(o.eh_map[0x402040],0x402100)
        self.assertEqual(audit['descriptor_aliases'][0]['generated_infos'],[0x402000,0x402040])
    def test_different_cleanup_home_cannot_be_an_alias(self):
        o=self.oracle(changed_cleanup=1);audit=recover(o)
        self.assertEqual(audit['accepted'],[]);self.assertEqual(o.eh_map,{})
    def test_missing_handler_identity_is_rejected(self):
        o=self.oracle(wrong_handler=1);audit=recover(o)
        self.assertEqual(audit['accepted'],[]);self.assertEqual(o.eh_map,{})
    def test_unknown_cleanup_callee_cannot_be_solved_from_original(self):
        o=self.oracle();del o.named['operator delete'];audit=recover(o)
        self.assertEqual(audit['accepted'],[]);self.assertEqual(o.eh_map,{})
    def test_unexplained_extra_descriptor_is_rejected(self):
        o=self.oracle(native=3,changed_cleanup=2);audit=recover(o)
        self.assertEqual(audit['accepted'],[]);self.assertEqual(o.eh_map,{})
    def test_two_original_identities_remain_ambiguous(self):
        o=self.oracle(native=3,original=2);audit=recover(o)
        self.assertEqual(audit['accepted'],[]);self.assertEqual(o.eh_map,{})

if __name__=='__main__':unittest.main()
