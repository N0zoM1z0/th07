"""Changing C2 must fail even with the driver/frontend/linker held constant."""
import hashlib
import sys
import tempfile
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from tool_identity import TOOL_FILES,stock_identity,require_tool_identity

class ToolIdentityTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
        for name in TOOL_FILES.values():(self.root/name).write_bytes(name.encode())
        self.expected=stock_identity(self.root)
    def tearDown(self):self.temp.cleanup()
    def test_complete_stock_pipeline_is_accepted(self):
        require_tool_identity(stock_identity(self.root),self.expected,TOOL_FILES)
    def test_changed_backend_with_unchanged_other_components_is_rejected(self):
        (self.root/'c2.dll').write_bytes(b'other machine-code generator')
        with self.assertRaisesRegex(ValueError,'backend_sha256'):
            require_tool_identity(stock_identity(self.root),self.expected,TOOL_FILES)
    def test_changed_private_backend_copy_is_rejected(self):
        private={'backend_sha256':hashlib.sha256(b'changed copied backend').hexdigest()}
        with self.assertRaisesRegex(ValueError,'backend_sha256'):
            require_tool_identity(private,self.expected,('backend_sha256',))
    def test_legacy_native_receipt_without_backend_fails_closed(self):
        receipt=dict(self.expected);del receipt['backend_sha256']
        with self.assertRaisesRegex(ValueError,'backend_sha256'):
            require_tool_identity(receipt,self.expected)
    def test_wrong_backend_receipt_fails_closed(self):
        receipt=dict(self.expected);receipt['backend_sha256']='0'*64
        with self.assertRaisesRegex(ValueError,'backend_sha256'):
            require_tool_identity(receipt,self.expected)
    def test_lock_without_backend_pin_fails_closed(self):
        expected=dict(self.expected);del expected['backend_sha256']
        with self.assertRaisesRegex(ValueError,'backend_sha256'):
            require_tool_identity(self.expected,expected)

if __name__=='__main__':unittest.main()
