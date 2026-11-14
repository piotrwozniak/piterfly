# piterfly 1.2.0 — 6+2

Release 1.2.0, 2026-11-14. Source branch: `feature/six-synth-two-drums`. Software checks pass; full physical FM-1 validation of 6+2 remains pending.

## Changes from 1.1.0

- Tracks 1–6 remain synths, sharing eight synth voices.
- Tracks 7 and 8 are independent 16-lane drum parts, with six voices each. Kit selection, patterns, tuning/decay, mixer settings and effects are independent. Hi-hat choke stays inside its part.
- Eight named song slots: save, load from a list, delete, free-space/error reporting and per-song export to the Mac. Sparse snapshots preserve only nonempty stored patterns plus the working loop and order.
- All four user-sample banks are reclaimed, giving a shared 352 KiB song/pattern pool.
- Both drum parts trigger DUCK on the six synths.
- The device UI, MIDI, editor and browser audio preview support 6+2.
- SONG shows every BARS value, separate EMPTY/READY status and a yellow selected-duration counter in the bottom-right corner.

Default MIDI channels: synths 1–6, drum track 7 on channel 7 (CH7), drum track 8 on channel 10 (DRUM CH). Every track has independent THRU routing. Shared sample libraries and SYN kit definitions keep their existing roles.

## Breaking compatibility — back up before installation

**1.2.0 does not migrate older projects. Export/back up using the older editor before updating; retain that backup with its matching firmware.** All user-sample banks are overwritten by song/pattern storage.

This build uses **FUNB**, 6,160 bytes, and **editor protocol 19**. It does not import FUN1–FUNA projects. Settings remain PER4. All former user-sample banks store patterns and named songs; factory samples remain. Capacity remains 24 patterns and 128 SONG entries. Importing a song replaces all 24 patterns and the working loop, including empty slots.

## Build

```sh
sh build.sh --release 1.2.0
sh tests/run_tests.sh
```

Use the prerequisites and environment overrides in [BUILDING.md](../BUILDING.md). The local output is `build/felucca-1.2.0.fwsc`, updater identity **FM-1_1120101**. Use the matching 1.2.0 editor. Release assets include the `.fwsc` package, app/loader binaries, exact source archive, licences, matching browser preview, BUILD_INFO and SHA256SUMS.

[Install 1.2.0](https://piotrwozniak.github.io/piterfly/webapp/installer/) · [Release assets](https://github.com/piotrwozniak/piterfly/releases/tag/1.2.0)

The full host suite, dedicated 6+2 ASan/UBSan and ThreadSanitizer checks, and native/WASM audio comparisons pass. Main RAM and low RAM have little remaining space. These results do not establish worst-case real-time performance; playback with both drum parts, effects, MIDI and save/restart still needs a physical FM-1 test.

[Named songs and memory](SONGS.md) · [Track controls](TRACKS.md) · [Polska instrukcja](PITERFLY.md) · [Detailed verification](SIX-SYNTH-TWO-DRUMS.md) · [Historical release 1.1.0](RELEASE-1.1.0.md)
