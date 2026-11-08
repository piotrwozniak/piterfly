#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Version transport compatibility and source-manifest identity (no build/flash)."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from build import release_identity
from prepare_release import tree_hash

for version, identity in [("1.0.0", "FM-1_910"), ("1.0.1", "FM-1_910"),
                          ("1.0.0-beta.2", "FM-1_910"), ("0.9-beta", "FM-1_909")]:
    assert release_identity(version) == identity
for bad in ["1", "10.0.0", "1.01.0", "1.0.01", "1.0.0/../x", "1.0.0-"]:
    try:
        release_identity(bad)
    except ValueError:
        pass
    else:
        raise AssertionError(bad)
files = {"a": {"sha256": "a" * 64, "executable": False}, "b": {"sha256": "b" * 64, "executable": False}}
original = tree_hash(files)
assert original == tree_hash(dict(reversed(list(files.items()))))
files["b"]["executable"] = True
assert original != tree_hash(files)
files["b"]["executable"] = False
files["b"]["sha256"] = "c" * 64
assert original != tree_hash(files)
print("release version and source identity tests passed")
