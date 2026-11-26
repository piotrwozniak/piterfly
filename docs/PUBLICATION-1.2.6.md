# Piterfly 1.2.6 publication record

Published on 2026-11-26. [Release 1.2.6](https://github.com/piotrwozniak/piterfly/releases/tag/1.2.6)

- The tag points to public commit `66fe06e539c73efd133ef80785ee253485644e90` from private commit `a0509e725118090819784961669bf8d45a604418`.
- The source tree SHA-256 is `9bc5e509fc496cd34ca531d3385f90e8dfe34135912b9f045d98316637da10a5`. All 460 archive inputs match the frozen public commit, with the declared Windows newline normalization.
- The firmware SHA-256 is `a7dd7ac1fb857099c02c44594f32467ebc9a1aabb155a99c0927e0e893b4b556`. All nine uploaded files were downloaded and compared with the local files. SHA256SUMS and the installer package checks passed.
- The app is 581484 B. The build reports 97848/98304 B of RAM, 29344/29384 B of low RAM and 335756/344064 B of pool use.
- ASan/UBSan checks covered track clears through simulated power cycles, interrupted workspace replacement, saved-song preservation and the SONG outline colours. Browser audio passed native/WASM comparisons.
- The public installer, firmware, browser audio, checksums and SONG documentation were verified after deployment. [Pages deployment](https://github.com/piotrwozniak/piterfly/actions/runs/45842330102)

Physical FM-1 validation remains pending.
