# Piterfly 1.2.5 publication record

Published 2026-11-18: <https://github.com/piotrwozniak/piterfly/releases/tag/1.2.5>.

- Frozen public source commit: `fcd65a1` (tag `1.2.5`). Private source origin: `6e3f735`.
- Source tree SHA256: `7837b7367c637f680f93f933f0bdd13c65aef9027fa42acff993d961b18b51f0`.
- Firmware SHA256: `70906d9658b2d89779db94bcca35d19436825ac02b43f9911f3c06afc5d85535`.
- App 581176 B; RAM 97848/98304 B; low RAM 29344/29384 B; pool 335756/344064 B.
- All nine uploaded assets downloaded and compared byte-for-byte; SHA256SUMS verified. Source archive inputs match the public commit, with declared Windows newline normalization.
- ASan/UBSan: HOME pixel comparison outside approved labels, sound numbers and all-track engine labels, LEVEL/STEPS/PAN recall, full-length pattern switching. UI suite with 20000-frame fuzz passed. Native/WASM audio comparisons passed.
- Approved behavior: REC/SOLO/MUTE temporarily replaces #XXX. No extra current/total counter, strip or layout resizing.
- Initial Pages deployment: `44199675446`. Live 1.2.5 installer, firmware bytes, both new screenshots and documentation verified.

Physical FM-1 not tested. Frozen release files are unchanged by subsequent site-only documentation corrections (including the dynamic version heading on the licence page).
