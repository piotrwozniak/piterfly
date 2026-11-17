# Piterfly 1.2.4 publication record

Published on 2026-11-17 at <https://github.com/piotrwozniak/piterfly/releases/tag/1.2.4>.

- Frozen public tag: `1.2.4`, commit `70e7f1db222c4d3d719adb3202d8f21c83173653`.
- Private source origin: `8aca34147027d5dc588ff922a1acddd8beb98e93`.
- Firmware SHA-256: `e064e0404cde70005880872c1becad2e44dd558782c011035b770c40adbabd71`.
- Exact source tree SHA-256: `b0938c4c84d01c1f2c75c910cfa98f419de1763671d91a932beefae063a07199`.

This corrects the unrequested HOME layout regression in 1.2.2 and 1.2.3. Original 36-pixel rows begin at y=40; label/engine positions, step dimensions, LEVEL bars and controls at y=184 are restored from public 1.2.0. The extra 24-pixel name/counter strip and per-row sound numbers are removed. Only the instrument indicator occupies formerly empty header rows 35–37. Font definitions/generators are unchanged.

A direct per-pixel audit compared the restored renderer with the original 1.2.0 function (identical to private fe74608) outside those three rows. The stored 64-case regression covers eight tracks, two palettes, STOP/PLAY/REC-with-pending-save/SOLO. A separate header-space probe covers visible PENDING and click labels. ASan/UBSan HOME and LEVEL/STEPS/PAN recall tests pass, together with full panel bounds/fuzz rendering, full-loop switching, native/WASM audio and site checks. DSP WASM remains byte-identical to 1.2.3. Physical FM-1 testing remains pending.

Target image 580972 bytes; RAM 97848/98304, low RAM 29344/29384, pool 335756/344064. FUNB (6160 bytes), PER4, protocol 19 and FM-1_1120101 remain unchanged; parameter-retention fixes remain included.

Nine uploaded assets were downloaded and compared byte-for-byte and all checksums verified. Preferred sources match the frozen public commit, allowing declared Windows script line endings. Release descriptions for 1.2.2 and 1.2.3 now explicitly identify their HOME regression and point to 1.2.4; their tags and binaries are retained.

Pages deployment: <https://github.com/piotrwozniak/piterfly/actions/runs/44192185930>.
Installer: <https://piotrwozniak.github.io/piterfly/webapp/installer/>.
Documentation: <https://piotrwozniak.github.io/piterfly/webapp/changes/>.

Both Pages jobs succeeded. Live installer 1.2.4, firmware SHA256, checksum file, restored HOME screenshot and updated documentation were verified against the published assets. This record follows the frozen release tag without changing its binaries or source archive.
