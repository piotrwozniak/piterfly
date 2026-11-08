#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check the distribution inventory; this command never publishes or builds."""
import hashlib
import json
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[1]


def validate(root, names):
    root = Path(root)
    policy = json.loads((root / 'PUBLIC_EXPORT.json').read_text())
    excluded = policy['excluded_files']
    forbidden_hashes = {v['sha256'] for p, v in excluded.items() if p.endswith(('.png', '.svg'))}
    forbidden_hashes.add('ec703d9bacef2d88656a7a439f6a6e2ab904cd7f245a004c77586d979defb944')
    for name in names:
        path = PurePosixPath(name)
        if path.is_absolute() or '..' in path.parts or '.git' in path.parts:
            raise ValueError(f'Unsafe export path: {name}')
        if name in policy['excluded_paths'] or name.startswith(tuple(policy['excluded_prefixes'])):
            raise ValueError(f'Excluded historical material: {name}')
        file = root / name
        if file.is_symlink() or not file.is_file():
            raise ValueError(f'Not a regular source file: {name}')
        if file.suffix in ('.png', '.svg') and hashlib.sha256(file.read_bytes()).hexdigest() in forbidden_hashes:
            raise ValueError(f'Historical artwork under another filename: {name}')


if __name__ == '__main__':
    from source_inventory import verified_files
    names = verified_files(ROOT)
    validate(ROOT, names)
    print('Public export inventory: sealed sources exclude Git metadata, historical paths, image copies and symlinks PASS')
