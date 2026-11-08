# piterfly 1.0.0 — sanitized local distribution candidate

Prepared locally on 2026-11-08 from private main `8d9ac74`. Not published,
or flashed as a firmware Release. The initial export contained no Git repository.
On 2026-11-09 the owner requested a fresh private source repository,
`piotrwozniak/piterfly`, with no inherited history. The original private project
and its history are unchanged.

See [provenance](../PROVENANCE.md), [export inventory](../PUBLIC_EXPORT.json),
[icon rights](ICON-RIGHTS.md), [licensing](../LICENSING.md) and
[third-party notices](../THIRD_PARTY_NOTICES.md). Historical artwork and generated
upstream releases are omitted; no permission to redistribute them is claimed.

## Build and complete source

The software version in `firmware/src/version.h` remains **1.0.0**, with updater
identity **FM-1_910**. The export changes distribution documentation and tooling,
not firmware, HAL, loader, DSP, sample data or engine parameters.

Use Python with `tools/requirements-build.txt`, the pinned JieLi toolchain and
SDK tag AC79NN_SDK_V1.2.1_2023-12-13. The build pins the Docker image and hashes
of the three embedded Apache-2.0 SDK files. The toolchain itself is not bundled.

```sh
PYTHON=/path/to/python-with-pillow \
JIELI_TOOLCHAIN=/path/to/toolchain \
AC79_SDK=/path/to/sdk \
/path/to/python-with-pillow tools/prepare_release.py
```

After reviewing the sources, seal their inventory without creating Git metadata:

```sh
python3 tools/prepare_release.py --seal-sources
```

The tool verifies SOURCE_INPUTS.json and rejects changed or unlisted sources,
nested Git metadata, excluded paths, symlinks and known excluded image copies.
The new root `.git` directory is omitted from the inventory and archives. It freezes
all sealed inputs and builds from that snapshot, never from the private checkout.
The three verified SDK files and full licence are included under `vendor/ac79`.
Set AC79_SDK to this copy's vendor/ac79 to build without a full external SDK.

Outputs under `build/releases/piterfly-1.0.0/`: firmware `.fwsc`, app and OTA
binaries, corresponding `-source.tar.gz`, full `-licences.zip`, BUILD_INFO.json,
SHA256SUMS and build.log. The source archive includes preferred editable assets,
all generators, scripts, tests, licence texts, PUBLIC_EXPORT.json, SOURCE_INPUTS.json, PROVENANCE.md
and SOURCE_MANIFEST.json with hashes/executable bits for every build input.
The manifest records the original main commit and a source-tree hash;
no new Git commit is created or claimed.
An existing output directory is never overwritten; use `--out` for another run.

## Site and comparisons

```sh
python3 web/make_site.py \
  build/releases/piterfly-1.0.0/piterfly-1.0.0.fwsc 1.0.0 build/site \
  --release-dir build/releases/piterfly-1.0.0
```

Only listed, included screenshots are copied. The before/after feature explanations
remain; the before images are labelled schematic legacy-layout reconstructions
using new GPL icons. `tools/render_ui_docs.c` provides their source.
The old checked-in `docs/webapp`, old SLOOP firmware and prebuilt WASM are absent.
Rebuild browser audio from this same sealed tree using Emscripten or clang/wasm-ld:

```sh
PATH=/path/to/wasi-sdk/bin:$PATH python3 tools/build_preview.py
```

This must be run after sealing, before preparing the release. The preview builder
records the source-inventory hash and compiler version; preparation refuses a
stale/mismatched preview. The release includes the new WASM and its build metadata,
and the site checks that the preview belongs to that release.

The generator checks the firmware/source version and hash before linking the
archives. The standalone editor retains full software/font notices. The site
base URL is `https://piotrwozniak.github.io/piterfly/`, including links in
the downloaded editor. This destination has been selected but not deployed.
Preparing a site does not publish it; see DEPLOYMENT.md.

## Behaviour and installation limits

24 patterns, 128 SONG entries, three 80 KiB user sample banks, MIDI THRU and live
granular INSERT remain. Projects are FUN7, settings PER4, protocol 15; earlier
FUN1–FUN6/PER3 imports remain supported. The old USR4 bank is pattern storage.
Back up before installing; retain an old-firmware backup before migration.
Do not restore FUN7/PER4 into unsupported older firmware.

No hardware installation, audio timing or physical power-cut recovery test is
performed by local release preparation. See PATTERNS-24.md and LIVE-GRANULAR.md.
