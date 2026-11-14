# Piterfly 1.2.0 publication record

Published on 2026-11-14 at <https://github.com/piotrwozniak/piterfly/releases/tag/1.2.0>.

- Frozen public tag: `1.2.0`, commit `3f103f129349ae23a6b9729836de33c7f23d0ae1`.
- Private source origin: `c51551312ea3bf7a99ec7872a8e65c121dadcb5c`; distribution adaptations are identified by the sealed source manifests.
- Firmware SHA-256: `0533fec923659f0e9aa4ea4b54de32ce2fe945a8848224bc93c5d58c28a2f083`.
- Exact source tree SHA-256: `fa056c0ba96d60e800cd3eaad880c2279ebc8e793139141ea2d8bc7d3b074c47`.
- Product: `FM-1_1120101`; loader: `ota-FM-1_1120101`; MIDI ports: **piterfly** / **piterfly Update**.
- Project format: **FUNB**, 6,160 bytes; settings: **PER4**; editor protocol: **19**.

Version 1.2.0 changes the default layout to six synths and two independent drum tracks. Up to eight named songs share a 352 KiB pattern/song pool. Save, load from a list, delete, free-space reporting and per-song computer export are available. SONG displays EMPTY/READY separately from BARS and highlights the selected duration in yellow at bottom right.

This release does not import FUN1–FUNA projects. All user-sample banks are reclaimed for song/pattern storage; factory samples and SYN kits remain. Export/back up older work with its matching firmware before updating. Earlier releases and tags are preserved.

All nine uploaded assets were downloaded and compared byte-for-byte with the locally verified release bundle. The archive's preferred sources were compared with the frozen public commit, accounting for declared Windows line endings; pinned vendor inputs were checked by the release builder. Native host tests passed, including storage/full-memory/interrupted-write checks. Editor/updater simulations, firmware-backed browser controls and song export/import, and all-engine native/WASM comparisons passed with Node 22.23.2. Actual Chromium/PWA checks were skipped because Playwright is unavailable. Full physical FM-1 validation of the 6+2 layout remains pending; the earlier 1.1.0 acceptance does not cover it.

Public installer: <https://piotrwozniak.github.io/piterfly/webapp/installer/>.
Documentation: <https://piotrwozniak.github.io/piterfly/webapp/changes/>.
Successful Pages deployment: <https://github.com/piotrwozniak/piterfly/actions/runs/43610330931>.

The live installer reports 1.2.0; its downloaded firmware and served SHA256SUMS were verified against the release assets. Pages downloaded the published assets without recompiling firmware. The site-only follow-up completed the Polish/Spanish screenshot alt translations without changing the frozen release tag or firmware. The public Hub card describes 6+2 and named songs; Hub ingestion is independent of GitHub publication.
