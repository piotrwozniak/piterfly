#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Release boundaries: clean tracked sources, no old images or symlink escapes."""
import hashlib
import json
import sys
import tempfile
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from check_public_export import validate
import prepare_release
from source_inventory import seal, verified_files


def refused(fn):
    try:
        fn()
    except (ValueError, SystemExit):
        return
    raise AssertionError('Unsafe export was accepted')


with tempfile.TemporaryDirectory() as tmp:
    root = Path(tmp)
    old = b'excluded historical image fixture'
    policy = {'origin_commit': 'test-origin', 'excluded_paths': ['old.png'], 'excluded_prefixes': ['assets/logo/'],
              'excluded_files': {'old.png': {'sha256': hashlib.sha256(old).hexdigest()}}}
    (root / 'PUBLIC_EXPORT.json').write_text(json.dumps(policy))
    (root / 'LICENSE').write_text('GPL fixture; preserve notices')
    validate(root, ['PUBLIC_EXPORT.json', 'LICENSE'])
    (root / 'renamed.png').write_bytes(old)
    refused(lambda: validate(root, ['old.png']))
    refused(lambda: validate(root, ['renamed.png']))
    refused(lambda: validate(root, ['assets/logo/new-name.png']))
    refused(lambda: validate(root, ['../escape']))
    (root / 'link').symlink_to(root / 'LICENSE')
    refused(lambda: validate(root, ['link']))
    (root / 'renamed.png').unlink()
    (root / 'link').unlink()
    seal(root)
    prepare_release.ROOT = root
    assert prepare_release.source_files() == ['LICENSE', 'PUBLIC_EXPORT.json', 'SOURCE_INPUTS.json']
    assert not (root / '.git').exists()
    (root / 'unlisted').write_text('must not enter a release')
    refused(prepare_release.source_files)
    (root / 'unlisted').unlink()
    (root / 'LICENSE').write_text('unsealed change')
    refused(prepare_release.source_files)
    seal(root)
    prepare_release.source_files()
    (root / '.git').mkdir()
    (root / '.git/config').write_text('local repository metadata; never export')
    assert prepare_release.source_files() == ['LICENSE', 'PUBLIC_EXPORT.json', 'SOURCE_INPUTS.json']
    (root / 'nested').mkdir()
    (root / 'nested/.git').mkdir()
    refused(prepare_release.source_files)
    (root / 'nested/.git').rmdir()
    (root / '.git/config').unlink()
    (root / '.git').rmdir()
    (root / '.git').symlink_to(root / 'nested', target_is_directory=True)
    refused(prepare_release.source_files)
print('Public export: inventory, renamed historical image, symlink, path escape, sealed-source changes, unlisted files and Git-metadata guards PASS')
