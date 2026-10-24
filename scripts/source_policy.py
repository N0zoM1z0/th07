"""Keep the user-approved x87 primitive exception narrow and source-bound."""
import hashlib,re,tomllib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
ADDRESS=0x4325c0
APPROVAL_NOTE='User-approved 32-byte x87 primitive exception; target and both source bodies bound by config/source-policy.toml (2026-10-24).'
REASON='Assembler-only Float2/Float3 field-writing helper; unaccepted without the explicit x87 primitive exception.'

def primitive_bodies(text):
    bodies=[]
    for match in re.finditer(r'void FromAngleMagnitude\(f32 angle, f32 magnitude\)\s*\{',text):
        begin=match.start();depth=1;cursor=match.end()
        while depth and cursor<len(text):
            if text[cursor]=='{':depth+=1
            elif text[cursor]=='}':depth-=1
            cursor+=1
        if depth:raise ValueError('unbalanced x87 primitive definition')
        bodies.append(text[begin:cursor])
    return bodies

def exclusions(root=ROOT):
    path=root/'config/source-policy.toml'
    if not path.is_file():return {ADDRESS:REASON}
    policy=tomllib.loads(path.read_text())
    if set(policy)!={'x87_math_primitive'}:raise ValueError('unsupported source-policy exception')
    entry=policy['x87_math_primitive']
    if not entry.get('user_authorized',False):return {ADDRESS:REASON}
    if entry['address']!=ADDRESS or entry['compare_size']!=32 or entry['source']!='src/th07/Global.hpp':raise ValueError('x87 exception scope changed')
    target=tomllib.loads((root/'config/target.toml').read_text())['target']['sha256']
    if entry['target_sha256']!=target:raise ValueError('x87 exception is bound to another target')
    bodies=primitive_bodies((root/entry['source']).read_text())
    if len(bodies)!=2 or any(hashlib.sha256(body.encode()).hexdigest()!=entry['body_sha256'] for body in bodies):raise ValueError('reviewed x87 primitive source changed')
    return {}
