# Piterfly 1.2.3 publication record

Published on 2026-11-17 at <https://github.com/piotrwozniak/piterfly/releases/tag/1.2.3>.

- Frozen public tag: `1.2.3`, commit `b55350c3617f7c80a7c8463a527389a49f4ee4a6`.
- Private source origin: `b8c02573e9e6abda397dddc7978f657fee83a286`.
- Firmware SHA-256: `0ee7170cf1d500f80bad85c82e478d5517332f98861bf4458811d87f602b8434`.
- Exact source tree SHA-256: `56d0389b96abb821ce9d7f7bce6871a427edec4db038f3507489c8ad06496384`.

This visual patch replaces the HOME instrument outline with the approved 240-pixel central line and raises the position block three pixels clear of the first track frame. Controls, audio, MIDI, FUNB/PER4/protocol-19 formats and updater identity FM-1_1120101 are unchanged. Earlier releases and tags remain intact.

ASan/UBSan HOME, complete panel rendering, pattern switching and main LEVEL/STEPS/PAN recall tests passed. Rendering matches the approved preview pixel-for-pixel. The target app is 581384 bytes with unchanged RAM usage. Fresh native/WASM comparisons and static site/export checks passed; DSP WASM is byte-identical to 1.2.2. Unrelated exhaustive legacy protocol tests were not repeated for this two-line drawing change. Physical FM-1 testing remains pending.

Nine uploaded assets were downloaded and compared byte-for-byte, including all SHA256SUMS entries. Preferred sources were checked against the frozen public commit, allowing declared Windows script line endings. The public site is built from these published assets without recompiling firmware.

Pages deployment: <https://github.com/piotrwozniak/piterfly/actions/runs/44184474704>.
Installer: <https://piotrwozniak.github.io/piterfly/webapp/installer/>.
Documentation: <https://piotrwozniak.github.io/piterfly/webapp/changes/>.

Both Pages jobs succeeded. The live installer reports 1.2.3; its firmware, SHA256SUMS, BUILD_INFO and HOME screenshot match the verified release. This record follows the frozen tag and does not alter its source archive or binaries.

## Documentation correction

Current guide headings, build examples, licensing summary and current-version pointers now consistently identify 1.2.3. Historical release descriptions, SDK tags and compatibility references remain unchanged. Updated documentation is provided separately as piterfly-1.2.3-documentation.zip with its own checksum; frozen firmware and original source-build assets are unchanged.
