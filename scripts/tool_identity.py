"""Pin the complete native compiler pipeline, including both C2 load paths."""
import hashlib

TOOL_FILES = {
    'compiler_sha256': 'cl.exe',
    'frontend_sha256': 'c1xx.dll',
    'backend_sha256': 'c2.dll',
    'linker_sha256': 'link.exe',
}
NATIVE_RECEIPT_KEYS = ('compiler_sha256', 'backend_sha256', 'linker_sha256')


def stock_identity(directory):
    return {key: hashlib.sha256((directory / name).read_bytes()).hexdigest()
            for key, name in TOOL_FILES.items()}


def require_tool_identity(measured, expected, keys=NATIVE_RECEIPT_KEYS):
    for key in keys:
        if not expected.get(key) or measured.get(key) != expected[key]:
            raise ValueError('unrecognized or unrecorded toolchain component: ' + key)
