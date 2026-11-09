#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Prepare a LOCAL piterfly build, exact source archive and notices. Never publish or flash.

Run with the same JIELI_TOOLCHAIN / AC79_SDK / PYTHON as build.sh.
The default output is build/releases/piterfly-VERSION (must not already exist).
"""
import argparse
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import zipfile
from datetime import datetime, timezone
from pathlib import Path

import build
from check_public_export import validate
from source_inventory import seal, verified_files

ROOT = Path(__file__).resolve().parents[1]
EXCLUDED = ("docs/webapp/", "docs/firmware/", "docs/index.html")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_files():
    names = verified_files(ROOT)
    validate(ROOT, names)
    return names


def tree_hash(files):
    return hashlib.sha256(''.join(f'{p}\0{meta["sha256"]}\0{meta["executable"]}\n' for p, meta in sorted(files.items())).encode()).hexdigest()


def main(out=None):
    version = re.search(r'^#define PITERFLY_VERSION "([^"]+)"', (ROOT / "firmware/src/version.h").read_text(), re.M)[1]
    product = build.release_identity(version)
    out = Path(out or ROOT / "build/releases" / f"piterfly-{version}").resolve()
    if out.exists():
        raise SystemExit(f"Output already exists: {out}; choose a new --out directory")
    tc = Path(os.environ.get("JIELI_TOOLCHAIN", ROOT / "build/deps/jieli/toolchain")).resolve()
    sdk = Path(os.environ.get("AC79_SDK", ROOT / "build/deps/ac79")).resolve()
    if not (tc / "pi32v2/bin/clang").is_file():
        raise SystemExit("Set JIELI_TOOLCHAIN to the existing vendor toolchain")
    for rel, expected in build.SDK_SHA256.items():
        if sha(sdk / "cpu/wl82/tools" / rel) != expected:
            raise SystemExit(f"SDK file differs from pinned tag: {rel}")
    files = source_files()
    provenance = json.loads((ROOT / "PUBLIC_EXPORT.json").read_text())
    if out.is_relative_to(ROOT) and any((ROOT / p).is_relative_to(out) for p in files):
        raise SystemExit("Output directory contains source inputs")
    source = out / "source"
    source.mkdir(parents=True)
    for name in files:
        dest = source / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / name, dest)
    for rel in build.SDK_SHA256:
        dest = source / "vendor/ac79/cpu/wl82/tools" / rel
        dest.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(sdk / "cpu/wl82/tools" / rel, dest)
    shutil.copy2(ROOT / "LICENSES/Apache-2.0-JieLi.txt", source / "vendor/ac79/LICENSE")
    inputs = {p.relative_to(source).as_posix(): {"sha256": sha(p), "bytes": p.stat().st_size,
              "executable": bool(p.stat().st_mode & 0o111)} for p in sorted(source.rglob("*")) if p.is_file()}
    source_id = tree_hash(inputs)
    base = provenance["origin_commit"]
    manifest = {"version": version, "base_commit": base, "source_tree_sha256": source_id,
                "git_status": None, "source_tracking": "SOURCE_INPUTS.json; Git metadata excluded",
                "origin_commit": provenance["origin_commit"], "export_policy": "PUBLIC_EXPORT.json",
                "excluded_generated_site": list(EXCLUDED), "files": inputs,
                "rebuild": "AC79_SDK=$PWD/vendor/ac79 JIELI_TOOLCHAIN=/path/to/pinned/toolchain PYTHON=/path/to/python-with-pillow sh build.sh --release " + version}
    (source / "SOURCE_MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
    env = dict(os.environ, JIELI_TOOLCHAIN=str(tc), AC79_SDK=str(source / "vendor/ac79"), PYTHON=sys.executable, PYTHONDONTWRITEBYTECODE="1")
    # A local caller's build flags must not silently alter the documented default configuration.
    for key in list(env):
        if key.startswith("FELUCCA_"):
            del env[key]
    with (out / "build.log").open("w") as log:
        subprocess.run(["sh", "build.sh", "--release", version], cwd=source, env=env, stdout=log, stderr=subprocess.STDOUT, check=True)
    for path, meta in inputs.items():
        if sha(source / path) != meta["sha256"]:
            raise SystemExit(f"Build changed a source input: {path}")
    package = out / f"piterfly-{version}.fwsc"
    shutil.copy2(source / "build" / f"felucca-{version}.fwsc", package)
    shutil.copy2(source / "build/felucca.bin", out / f"piterfly-{version}-app.bin")
    shutil.copy2(source / "build/loader/ota.bin", out / f"piterfly-{version}-ota.bin")
    preview = ROOT / "build/preview/sloop-preview.wasm"
    if preview.exists():
        preview_info = json.loads((preview.parent / "BUILD_INFO.json").read_text())
        if preview_info["source_inputs_sha256"] != sha(ROOT / "SOURCE_INPUTS.json") or preview_info["wasm_sha256"] != sha(preview):
            raise SystemExit("Browser preview differs from the sealed source/build metadata; rebuild it")
        shutil.copy2(preview, out / f"piterfly-{version}-preview.wasm")
        shutil.copy2(preview.parent / "BUILD_INFO.json", out / f"piterfly-{version}-preview-build.json")
    archive = out / f"piterfly-{version}-source.tar.gz"
    with tarfile.open(archive, "w:gz") as tar:
        for name in [*inputs, "SOURCE_MANIFEST.json"]:
            tar.add(source / name, arcname=f"piterfly-{version}/{name}", recursive=False)
    notices = out / f"piterfly-{version}-licences.zip"
    with zipfile.ZipFile(notices, "w", compression=zipfile.ZIP_DEFLATED) as z:
        for name in ["LICENSE", "LICENSING.md", "THIRD_PARTY_NOTICES.md", "PROVENANCE.md", "PUBLIC_EXPORT.json", "docs/ICON-RIGHTS.md",
                     "docs/RELEASE-1.0.0.md", "docs/RELEASE-1.0.1.md", "assets/icons-LICENSE.txt", "assets/piterfly/LICENSE.txt",
                     "assets/samples-cc0/ATTRIBUTION.txt", "assets/hiphop-pack/CREDITS.txt",
                     *[p.relative_to(source).as_posix() for p in sorted((source / "LICENSES").glob("*.txt"))]]:
            z.write(source / name, name)
    artifacts = {p.name: {"bytes": p.stat().st_size, "sha256": sha(p)} for p in sorted(out.iterdir()) if p.suffix in (".fwsc", ".bin", ".gz", ".zip", ".wasm", ".json")}
    info = {"version": version, "product": product, "prepared_utc": datetime.now(timezone.utc).isoformat(),
            "local_only": True, "git_repository_created": False, "distribution_status": "sanitized distribution candidate; excluded inherited artwork absent; local review only",
            "origin_commit": provenance["origin_commit"],
            "base_commit": base, "source_tree_sha256": source_id, "artifacts": artifacts,
            "protocol": 15, "project_format": "FUN7", "settings_format": "PER4",
            "configuration": "default; all FELUCCA_* environment overrides removed",
            "python": platform.python_version(), "docker_image": build.DOCKER_IMAGE,
            "toolchain_files": {n: sha(tc / n) for n in ["pi32v2/bin/clang", "pi32v2/bin/ld", "common/bin/objcopy", "common/bin/objdump"]},
            "sdk_tag": "AC79NN_SDK_V1.2.1_2023-12-13", "sdk_sha256": build.SDK_SHA256}
    (out / "BUILD_INFO.json").write_text(json.dumps(info, indent=2) + "\n")
    sums = {**artifacts, "BUILD_INFO.json": {"sha256": sha(out / "BUILD_INFO.json")}}
    (out / "SHA256SUMS").write_text(''.join(f'{meta["sha256"]}  {name}\n' for name, meta in sorted(sums.items())))
    print(f"Local piterfly {version}: {out}")
    print(f"Exact source tree SHA-256: {source_id}")
    print("Sanitized distribution candidate. Not published or flashed; review the manifest and test results before distribution.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--seal-sources", action="store_true", help="Record reviewed preferred-source hashes without creating Git metadata")
    args = parser.parse_args()
    if args.seal_sources:
        files = seal(ROOT)
        print(f"Sealed {len(files)} preferred sources in SOURCE_INPUTS.json; no Git repository created")
    else:
        main(args.out)
