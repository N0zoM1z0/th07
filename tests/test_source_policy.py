"""The explicit math exception cannot become a general assembly bypass."""
import hashlib,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'scripts'))
from source_policy import exclusions,primitive_bodies
class SourcePolicyTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
        (self.root/'config').mkdir();(self.root/'src').mkdir(parents=True)
        self.body='void FromAngleMagnitude(f32 angle, f32 magnitude) { primitive(); }'
        (self.root/'src/Global.hpp').write_text(self.body+'\n'+self.body)
        (self.root/'config/target.toml').write_text('[target]\nsha256="'+64*'a'+'"\n')
        self.config='''[x87_math_primitive]
user_authorized = true
address = 0x4325C0
compare_size = 32
source = "src/Global.hpp"
body_sha256 = "'''+hashlib.sha256(self.body.encode()).hexdigest()+'''"
target_sha256 = "'''+64*'a'+'''"
'''
        self.path=self.root/'config/source-policy.toml';self.path.write_text(self.config)
    def tearDown(self):self.temp.cleanup()
    def test_authorized_identical_bodies_are_allowed(self):self.assertEqual(exclusions(self.root),{})
    def test_no_policy_retains_exclusion(self):
        self.path.unlink();self.assertIn(0x4325c0,exclusions(self.root))
    def test_explicit_revocation_retains_exclusion(self):
        self.path.write_text(self.config.replace('user_authorized = true','user_authorized = false'));self.assertIn(0x4325c0,exclusions(self.root))
    def test_unreviewed_body_change_fails_closed(self):
        (self.root/'src/Global.hpp').write_text((self.body+'\n'+self.body).replace('primitive();','other();',1))
        with self.assertRaises(ValueError):exclusions(self.root)
    def test_widened_address_fails_closed(self):
        self.path.write_text(self.config.replace('0x4325C0','0x4325C1'))
        with self.assertRaises(ValueError):exclusions(self.root)
    def test_widened_extent_fails_closed(self):
        self.path.write_text(self.config.replace('compare_size = 32','compare_size = 33'))
        with self.assertRaises(ValueError):exclusions(self.root)
    def test_other_target_fails_closed(self):
        self.path.write_text(self.config.replace('target_sha256 = "'+64*'a','target_sha256 = "'+64*'b'))
        with self.assertRaises(ValueError):exclusions(self.root)
    def test_extra_exception_is_rejected(self):
        self.path.write_text(self.config+'\n[other]\nauthorized=true\n')
        with self.assertRaises(ValueError):exclusions(self.root)
    def test_missing_shared_definition_is_rejected(self):
        (self.root/'src/Global.hpp').write_text(self.body)
        with self.assertRaises(ValueError):exclusions(self.root)
if __name__=='__main__':unittest.main()
