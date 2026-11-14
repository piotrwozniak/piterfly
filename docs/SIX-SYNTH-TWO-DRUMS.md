# Piterfly 1.2.0: six synth parts and two drum tracks

Release 1.2.0, source branch `feature/six-synth-two-drums`. Full physical FM-1 validation remains pending.

Tracks 1–6 are synth parts sharing the existing eight-voice synth budget. Tracks 7 and 8 are independent drum parts, each with six percussion voices, a kit, 16 lanes, 64 steps, per-lane tuning/decay, level, pan, sends, filter, slicer and insert. Hi-hat choking stays inside its own track. Kicks from either drum part trigger DUCK on the six synth parts.

Track 7 uses CH7 (default MIDI channel 7) and THRU7. Track 8 retains DRUM CH (default channel 10) and its existing THRU setting. Use distinct channels to address them separately.

The on-device drum pages edit the selected drum track. Track 7 stores level/reverb/delay in its own track parameters; track 8 retains the existing global drum controls. The editor recognizes both drum engines and routes their controls separately.

## Storage

This branch deliberately starts a new project format, **FUNB**, 6,160 bytes. It does not import previous project formats. Previous patterns and the previous working project will not load; create new patterns. Device settings remain PER4. All four user-sample banks are reclaimed into the 352 KiB pattern/song pool. Factory kits and SYN1–SYN4 remain. SYN kit definitions are shared library presets; each track independently selects its kit and keeps its lane tuning.

## Editor protocol 19

INFO reports protocol 19. Track replies identify both drum parts with engine = NENGINES.

DRUM_STEP accepts an optional track byte after its existing payload: `[index, track]` to read; `[index, on(3), levels(5), ratchets(5), track]` to write. Only drum-track indices are accepted. Without that byte, it addresses the selected drum track (or track 8 when a synth is selected).

LIVE op 12 accepts an optional track after lane/tune/decay. LIVE op 0 accepts an optional drum track to query that track's tuning in the existing state reply. Other LIVE replies return the selected drum's tuning, or track 8's tuning when a synth is selected.

Eight named snapshots support save, load from a list, delete, capacity errors and direct USB export. See [SONGS.md](SONGS.md) for controls, capacity and shared-bank limits. Protocol 19 adds SONG_LIBRARY (78); SMP_INFO reports zero writable sample slots.

## Verification

`tests/two_drums_test.c` checks isolated voices and hi-hat choke, both step patterns and lane tuning through capture/load, MIDI channels and volume, every insert type, eight simultaneous stutters and granular page ownership. Its mixed audio hash must match in one-thread and two-thread runs. Run ASan/UBSan and ThreadSanitizer on this test as well.

`web/test_two_drums.mjs` checks two drum engines, independent remote steps/tuning, six DUCK targets and the FUNB envelope. The local audio preview omits the hardware song-library controls; storage is exercised against real firmware in `test_device_songs.mjs`. The legacy 3+1 and 7+1 fixtures remain available for regression checks; they are not the default device layout.

A successful target build establishes memory fit, not worst-case real-time performance. Both drum tracks, effects, MIDI and a save/restart still require a physical FM-1 test before release.

The earlier 6+2 audio checks passed on 2026-11-14; the new named-song storage adds ASan/UBSan tests for sparse snapshots, cold scans, full capacity, deletion and injected flash-write failures. `web/test_device_songs.mjs` drives the real C firmware protocol through save-list/load/export/import checks, rather than a JavaScript storage mock. Browser visual/offline checks require Playwright and remain unverified when unavailable.

The target build with named songs is 581,488 bytes, with 97,848/98,304 bytes in RAM, 29,344/29,384 bytes in lowram, and 335,756/344,064 bytes in the pool. Memory fit is tight. The development package is `build/felucca-1.2.0.fwsc`. No hardware was flashed.

Verified on 2026-11-14: `sh tests/run_tests.sh` completed with `ALL HOST TESTS PASSED`, including the firmware-backed browser-control/export tests and rebuilt native/WASM comparisons. The dedicated 6+2 ThreadSanitizer run also passed. Chromium/PWA checks were skipped because Playwright is unavailable. Named-song base package (`ec2dddc`) SHA-256: `20822cecc06fb1810de8df7815c0168df3c899df152e6ece3a5eef2b3c1748eb`.

SONG display follow-up: BARS is always shown in each row, EMPTY/READY is separate, and the selected duration appears in yellow at bottom right. The 1–64 limits, overwrite confirmation, screen bounds and yellow counter were checked with native renderers, including ASan/UBSan; the target build passes. Updated package SHA-256: `5f494e8f427295728319ddeaa5dbeafa09626b1debff8395940e369ada14b39e`.
