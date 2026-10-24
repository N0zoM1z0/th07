"""Independent metadata anchors and stale-build negative controls."""
import importlib.util,json,struct,sys,unittest
from pathlib import Path
from types import SimpleNamespace
from test_headless_oracle import Fixture,image,msf,record
from pdb_symbols import constant_float_types,data_type_sizes
from compiler_statics import recover
from pe_image import PE
ROOT=Path(__file__).resolve().parents[1]
def load(name,file):
    spec=importlib.util.spec_from_file_location(name,ROOT/'scripts'/file);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def tpi(entries):
    content=b''.join(record(kind,body) for kind,body in entries)
    header=struct.pack('<IIIII',20040203,56,0x1000,0x1000+len(entries),len(content)).ljust(56,b'\0')
    return msf([None,None,header+content])

class TypeTests(Fixture):
    def test_const_float_width_from_tpi(self):
        p=self.write('t.pdb',tpi([(0x1001,struct.pack('<IH',0x40,1)),(0x1001,struct.pack('<IH',0x41,1))]))
        self.assertEqual(constant_float_types(p),{0x1000:4,0x1001:8})
    def test_mutable_float_is_not_a_constant_anchor(self):
        p=self.write('t.pdb',tpi([(0x1001,struct.pack('<IH',0x40,0))]))
        self.assertEqual(constant_float_types(p),{})
    def test_cyclic_modifier_rejected(self):
        p=self.write('t.pdb',tpi([(0x1001,struct.pack('<IH',0x1000,1))]))
        with self.assertRaises(ValueError):constant_float_types(p)
    def test_missing_modifier_type_rejected(self):
        p=self.write('t.pdb',tpi([(0x1001,struct.pack('<IH',0x1010,1))]))
        with self.assertRaises(ValueError):constant_float_types(p)
    def test_class_size_excludes_forward_declaration(self):
        body=struct.pack('<HHIIIH',0,0,0,0,0,23556)+b'Room\0'
        forward=bytearray(body);struct.pack_into('<H',forward,2,0x80)
        p=self.write('t.pdb',tpi([(0x1505,body),(0x1505,forward)]))
        sizes=data_type_sizes(p);self.assertEqual(sizes[0x1000],23556);self.assertNotIn(0x1001,sizes)

class StaticTests(Fixture):
    profile={'module':'Unit.obj','object':'s_Object','guard':'S1'}
    def fixture(self):
        pe=PE(self.write('i.exe',image(b'\xc3')))
        symbols=[{'type':'data','name':'s_Object','module':'Z:\\Unit.obj','address':0x403000,'section':3,'type_index':0x1000},{'type':'data','name':'S1','module':'Z:\\Unit.obj','address':0x403010,'section':3,'type_index':0x75}]
        return SimpleNamespace(generated=pe,target=pe,symbols=symbols,original_globals=[(0x403040,16,'s_Object'),(0x403054,4,'next')])
    def test_guard_comes_from_object_extent(self):
        result,audit=recover(self.fixture(),[self.profile],{0x1000:16})
        self.assertEqual(result,{0x403010:0x403050});self.assertEqual(audit[0]['object_size'],16)
    def test_nonadjacent_guard_rejected(self):
        o=self.fixture();o.symbols[1]['address']+=4
        self.assertEqual(recover(o,[self.profile],{0x1000:16})[0],{})
    def test_size_disagreement_rejected(self):
        self.assertEqual(recover(self.fixture(),[self.profile],{0x1000:20})[0],{})
    def test_guard_from_other_module_rejected(self):
        o=self.fixture();o.symbols[1]['module']='Else.obj'
        self.assertEqual(recover(o,[self.profile],{0x1000:16})[0],{})
    def test_guard_cannot_overlap_named_global(self):
        o=self.fixture();o.original_globals.append((0x403050,4,'other'))
        self.assertEqual(recover(o,[self.profile],{0x1000:16})[0],{})
    def test_signed_or_untyped_guard_rejected(self):
        o=self.fixture();o.symbols[1]['type_index']=0x74
        self.assertEqual(recover(o,[self.profile],{0x1000:16})[0],{})

class CacheTests(Fixture):
    def setUp(self):
        super().setUp();(self.root/'src/th07').mkdir(parents=True)
        self.module=load('native_builder','build-reference.py')
        self.tree=[{'path':'src/th07/a.cpp','sha256':'a'},{'path':'src/th07/b.cpp','sha256':'b'},{'path':'src/th07/shared.hpp','sha256':'h'}]
        self.receipt={'source_root':str(self.root),'source_tree':self.tree,'generated_header_sha256':'header','compiler_sha256':'compiler','commands':[{'source':'src/th07/a.cpp'},{'source':'src/th07/b.cpp'}]}
    def cached(self,tree=None):return self.module.eligible_cached_commands(self.receipt,self.root,tree or self.tree,'header',{'compiler_sha256':'compiler'})
    def test_unchanged_inputs_eligible(self):self.assertEqual(len(self.cached()),2)
    def test_source_edit_invalidates_its_object(self):
        tree=[dict(r) for r in self.tree];tree[0]['sha256']='changed'
        self.assertEqual(set(self.cached(tree)),{'src/th07/b.cpp'})
    def test_header_edit_invalidates_every_object(self):
        tree=[dict(r) for r in self.tree];tree[2]['sha256']='changed'
        self.assertEqual(self.cached(tree),{})
    def test_different_compiler_invalidates_every_object(self):
        self.receipt['compiler_sha256']='different';self.assertEqual(self.cached(),{})
    def test_legacy_receipt_without_header_hash_rejected(self):
        del self.receipt['generated_header_sha256'];self.assertEqual(self.cached(),{})
    def test_cpp_inclusion_requires_full_build(self):
        (self.root/'src/th07/a.cpp').write_text('#include "b.cpp"\n');self.assertEqual(self.cached(),{})

class ObjectCacheTests(Fixture):
    def setUp(self):
        super().setUp();self.module=load('cache_builder','build-reference.py')
        (self.root/'unit.obj').write_bytes(b'compiled IL');(self.root/'unit.pdb').write_bytes(b'compiler debug metadata')
        self.receipt={'command':['cl','unit.cpp'],'object':'unit.obj','sha256':self.module.digest(self.root/'unit.obj'),'compiler_pdb':'unit.pdb','compiler_pdb_sha256':self.module.digest(self.root/'unit.pdb')}
    def reusable(self):return self.module.object_reusable(self.receipt,['cl','unit.cpp'],self.root)
    def test_object_and_compiler_pdb_pair_required(self):self.assertTrue(self.reusable())
    def test_missing_compiler_pdb_requires_recompile(self):
        (self.root/'unit.pdb').unlink();self.assertFalse(self.reusable())
    def test_changed_compiler_pdb_requires_recompile(self):
        (self.root/'unit.pdb').write_bytes(b'other');self.assertFalse(self.reusable())
    def test_changed_object_requires_recompile(self):
        (self.root/'unit.obj').write_bytes(b'other');self.assertFalse(self.reusable())
    def test_changed_flags_require_recompile(self):
        self.assertFalse(self.module.object_reusable(self.receipt,['cl','/GL','unit.cpp'],self.root))

class CrtThunkTests(Fixture):
    def fixture(self,duplicate=False):
        self.module=load('thunk_recovery','compiler_thunks.py')
        class Memory:
            def __init__(self,mapping):self.mapping=mapping
            def at(self,address,size):return self.mapping[address][:size]
        native=0x401000;callee=0x401080;target_callee=0x401180
        target={a:b'\xe9'+struct.pack('<i',target_callee-a-5) for a in ([0x401040,0x401060] if duplicate else [0x401040])}
        return SimpleNamespace(symbols=[{'type':'function','name':'atoi','size':5,'address':native}],generated=Memory({native:b'\xe9'+struct.pack('<i',callee-native-5)}),target=Memory(target),original_ledger={a:{'size':'5','status':'library'} for a in target},resolve_external=lambda a:(target_callee,'independent-symbol') if a==callee else (None,'unresolved'))
    def test_unique_complete_thunk(self):
        o=self.fixture();m,a=self.module.recover(o);self.assertEqual(m,{0x401000:0x401040})
    def test_duplicate_complete_thunks_remain_ambiguous(self):
        o=self.fixture(True);self.assertEqual(self.module.recover(o)[0],{})
    def test_unknown_callee_is_not_solved_from_original_operand(self):
        o=self.fixture();o.resolve_external=lambda a:(None,'unresolved');self.assertEqual(self.module.recover(o)[0],{})
    def test_wrong_callee_does_not_match(self):
        o=self.fixture();o.resolve_external=lambda a:(0x401184,'wrong-symbol');self.assertEqual(self.module.recover(o)[0],{})

if __name__=='__main__':unittest.main()
