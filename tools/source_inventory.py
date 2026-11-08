# SPDX-License-Identifier: GPL-3.0-only
"""Seal and verify preferred sources without Git. Build products are not inputs."""
import hashlib
import json
import os
from pathlib import Path

MANIFEST = 'SOURCE_INPUTS.json'


def inventory(root):
    root = Path(root).resolve()
    result = {}
    for parent, dirs, files in os.walk(root, followlinks=False):
        base = Path(parent)
        for name in dirs:
            if (name == '.git' and base != root) or (base / name).is_symlink():
                raise ValueError(f'Git metadata or symlink directory is not permitted: {base / name}')
        dirs[:] = [n for n in dirs if n != '__pycache__' and not (base == root and n in ('build', '.git'))]
        for name in files:
            p = base / name
            if name == '.DS_Store' or (base == root and name in (MANIFEST, 'SOURCE_MANIFEST.json')):
                continue
            if p.is_symlink() or not p.is_file():
                raise ValueError(f'Not a regular preferred source: {p}')
            result[p.relative_to(root).as_posix()] = {
                'sha256': hashlib.sha256(p.read_bytes()).hexdigest(),
                'bytes': p.stat().st_size, 'executable': bool(p.stat().st_mode & 0o111)}
    return dict(sorted(result.items()))


def seal(root):
    root = Path(root)
    from check_public_export import validate
    files = inventory(root)
    validate(root, files)
    policy = json.loads((root / 'PUBLIC_EXPORT.json').read_text())
    if (root / MANIFEST).is_symlink():
        raise ValueError('Source manifest cannot be a symlink')
    (root / MANIFEST).write_text(json.dumps({
        'origin_commit': policy['origin_commit'],
        'tracking': 'sealed preferred-source files; Git metadata excluded',
        'files': files}, indent=2) + '\n')
    return files


def verified_files(root):
    root = Path(root)
    if not (root / MANIFEST).is_file():
        raise ValueError('Seal the reviewed sources first: python tools/prepare_release.py --seal-sources')
    expected = json.loads((root / MANIFEST).read_text())['files']
    actual = inventory(root)
    if expected != actual:
        changed = sorted(p for p in set(expected) | set(actual) if expected.get(p) != actual.get(p))
        raise ValueError('Preferred sources differ from the sealed manifest: ' + ', '.join(changed))
    return sorted([*expected, MANIFEST])
