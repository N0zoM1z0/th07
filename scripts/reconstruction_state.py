"""Validate public reconstruction accounting without proprietary inputs."""
import csv,hashlib,json,tomllib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def rows(name):return list(csv.DictReader((ROOT/'config'/name).open()))
def state():
    from source_policy import exclusions
    denied=exclusions(ROOT)
    ledger={r['address']:r for r in rows('functions.csv')}
    origins={r['address']:r['origin'] for r in rows('function-origins.csv')}
    manifest=json.loads((ROOT/'config/exact-units.json').read_text())
    target=tomllib.loads((ROOT/'config/target.toml').read_text())['target']
    if manifest['target_sha256']!=target['sha256']:raise ValueError('exact manifest target differs')
    if set(origins)!=set(ledger):raise ValueError('origin ledger is incomplete')
    units=manifest['units'];matched=rows('matches.csv');remaining=rows('remaining.csv')
    if {r['unit'] for r in matched}!=set(units) or len(matched)!=len(units):raise ValueError('matches/manifest differ')
    addresses=set()
    for key,unit in units.items():
        a=unit['address'];row=ledger[a]
        if key!=unit['unit'] or key!=f'f_{int(a,0):08x}' or a in addresses:raise ValueError('invalid canonical unit')
        addresses.add(a)
        if int(a,0) in denied:raise ValueError('source-policy exclusion was granted exact credit')
        if origins[a]!=unit['origin'] or int(row['size'])!=unit['size']:raise ValueError('unit/ledger scope differs')
        if row['match_percent']!='100.00' or row['status']!=('matching' if unit['origin']=='authored' else 'library'):raise ValueError('unproven accepted unit')
        if unit['compare_size']<unit['size'] or len(unit['target_compared_sha256'])!=64:raise ValueError('truncated exact extent')
        source=(ROOT/unit['source_file']).resolve()
        if not source.is_relative_to(ROOT) or not source.is_file():raise ValueError('missing owned source')
        other=next(r for r in matched if r['unit']==key)
        if any(str(unit[k])!=v for k,v in other.items()):raise ValueError('matches content differs')
    authored=[r for a,r in ledger.items() if origins[a]=='authored']
    exact=[r for r in authored if r['status']=='matching']
    if {r['address'] for r in exact}!={r['address'] for r in units.values() if r['origin']=='authored'}:raise ValueError('authored exact ledger/manifest differ')
    if {r['address'] for r in remaining}!={r['address'] for r in authored if r['status']!='matching'}:raise ValueError('remaining scope differs')
    for r in remaining:
        if int(r['size'])!=int(ledger[r['address']]['size']) or not r['reason']:raise ValueError('invalid remaining row')
    baseline=rows('reference-baseline.csv');review=json.loads((ROOT/'config/boundary-review.json').read_text())
    result={'authored_functions':len(authored),'authored_bytes':sum(int(r['size']) for r in authored),'authored_exact_functions':len(exact),'authored_exact_bytes':sum(int(r['size']) for r in exact)}
    if any(review[k]!=v for k,v in result.items()):raise ValueError('boundary review accounting differs')
    result['authored_exact_percent']=100*result['authored_exact_bytes']/result['authored_bytes']
    result.update(reference_named_functions=len(baseline),reference_named_bytes=sum(int(r['size']) for r in baseline),reference_exact_functions=sum(r['status']=='exact' for r in baseline),reference_exact_bytes=sum(int(r['size']) for r in baseline if r['status']=='exact'),remaining_functions=len(remaining),legacy_complements=sum(r['kind']=='legacy-coff' and r['origin']=='authored' for r in units.values()))
    if result['reference_named_bytes']!=review['reference_named_bytes'] or result['reference_exact_bytes']!=review['reference_named_exact_bytes']:raise ValueError('baseline accounting differs')
    for r in json.loads((ROOT/'config/maintained-source-provenance.json').read_text())['files']:
        if hashlib.sha256((ROOT/r['path']).read_bytes()).hexdigest()!=r['sha256']:raise ValueError('maintained source provenance differs: '+r['path'])
    result['native_named_exact_functions']=review['native_named_exact_functions']
    result['native_named_exact_bytes']=review['native_named_exact_bytes']
    result['native_named_exact_percent']=100*review['native_named_exact_bytes']/result['reference_named_bytes']
    return result
