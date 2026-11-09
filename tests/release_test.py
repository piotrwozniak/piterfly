#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Version transport compatibility and source-manifest identity (no build/flash)."""
import sys
from pathlib import Path
import re

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from build import release_identity, PRODUCT
from prepare_release import tree_hash

assert PRODUCT == "FM-1_1120101"
for version in ("1.0.0", "1.0.1", "1.0.0-beta.2", "0.9-beta"):
    assert release_identity(version) == PRODUCT
assert len(PRODUCT) <= 20 and len("ota-" + PRODUCT) <= 27
assert all((ord(c) + i + 1) & 0xFF != 0x7D for i, c in enumerate(PRODUCT))
for bad in ["1", "10.0.0", "1.01.0", "1.0.01", "1.0.0/../x", "1.0.0-"]:
    try:
        release_identity(bad)
    except ValueError:
        pass
    else:
        raise AssertionError(bad)
# The header is the app/loader fallback as well as the build input.
root = Path(__file__).resolve().parents[1]
assert '#define FELUCCA_ID PITERFLY_PRODUCT' in (root / 'firmware/src/felucca.c').read_text()
assert '#define FELUCCA_ID "ota-" PITERFLY_PRODUCT' in (root / 'firmware/loader/loader.c').read_text()
assert PRODUCT in (root / 'firmware/src/version.h').read_text()
assert PRODUCT.split('_')[1] in (root / 'web/fm1ota.js').read_text()
assert PRODUCT.split('_')[1] in (root / 'tools/fm1_install.py').read_text()
if len(sys.argv) == 2:
    from fm1_install import product_of
    assert product_of(Path(sys.argv[1]).read_bytes()) == PRODUCT
    print('Built package identity matches configured USB app/loader identity:', PRODUCT)
files = {"a": {"sha256": "a" * 64, "executable": False}, "b": {"sha256": "b" * 64, "executable": False}}
original = tree_hash(files)
assert original == tree_hash(dict(reversed(list(files.items()))))
files["b"]["executable"] = True
assert original != tree_hash(files)
files["b"]["executable"] = False
files["b"]["sha256"] = "c" * 64
assert original != tree_hash(files)
print("release version and source identity tests passed")
