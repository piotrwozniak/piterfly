# Piterfly 1.2.2 publication record

Published on 2026-11-17 at <https://github.com/piotrwozniak/piterfly/releases/tag/1.2.2>.

- Frozen public tag: `1.2.2`, commit `0171d7803aaac4b2514def3910443f5a732668b4`.
- Private source origin: `4396f5dd49179ec7675a7af6a8760d128075b588`.
- Firmware SHA-256: `f04e0c881f1619f0103309f1a23695262d6355b41d5c5bf34b49a456d47f78b6`.
- Exact source tree SHA-256: `9b26e2cc2c6db484f57237767b2eb4d1bf0a51eb54c880e02bb43bbb0aa547f6`.
- Product: `FM-1_1120101`; project: FUNB, 6160 bytes; settings: PER4; editor protocol: 19.

The release adds the approved HOME sound identity/number and thin position rail, preserves main-panel LEVEL/STEPS/PAN across ordinary LOAD and live selection, and respects the full longest LEN/DIV pattern cycle. It remains compatible with 1.2.0 and local 1.2.1 projects. All prior public releases and tags remain unchanged.

Nine uploaded assets were downloaded and compared byte-for-byte with the verified bundle. All 436 non-vendor source inputs were compared against the frozen public commit, accounting for three declared Windows script line endings; pinned vendor inputs and all SHA256SUMS entries were verified. Application image: 581384 bytes. RAM 97848/98304; low RAM 29344/29384; pool 335756/344064 bytes, unchanged from 1.2.0.

Native host, storage, ASan/UBSan, threaded render/race and golden-audio checks passed. Real main-panel LEVEL/STEPS/PAN input was tested on every track, through stopped LOAD and live switching. HOME tests cover sound identity, rail endpoints, redraw, shared sound numbers and both drum kits. The initial web portion hit a broken system Node 26 (missing simdjson); all affected checks were rerun successfully on Node 22.23.2, including the freshly built release WASM/native comparisons. Chromium/PWA offline testing was skipped because Playwright is unavailable. Physical FM-1 validation remains pending.

Pages serves the published assets without recompiling firmware. Deployment: <https://github.com/piotrwozniak/piterfly/actions/runs/44180442246>.

Installer: <https://piotrwozniak.github.io/piterfly/webapp/installer/>.
Documentation: <https://piotrwozniak.github.io/piterfly/webapp/changes/>.
Polish manual: <https://github.com/piotrwozniak/piterfly/blob/1.2.2/docs/PITERFLY.md>.

The Pages build and deployment both succeeded. The live installer reports 1.2.2; its downloaded firmware, served SHA256SUMS, BUILD_INFO, HOME screenshot and documentation were verified against the published release assets. This publication record follows the frozen release tag and does not change its firmware or preferred-source archive.
