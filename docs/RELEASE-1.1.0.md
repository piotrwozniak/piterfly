# piterfly 1.1.0

Released 2026-11-13. Tested on the physical M-VAVE FM-1 and accepted by the project owner.

## What changes

- **Seven synth tracks plus drums**, compared with the original three synth tracks plus drums. Each synth part has its own engine, presets, sequence, effects and MIDI routing. Eight sounding synth voices are shared across the seven parts.
- **24 complete patterns**, A1–F4; **128 SONG positions**, with 1–64 bars per position. SONG REC and the eight-entry quick CHAIN are retained.
- **Clearer 64-step views**, distinct playback/edit markers, track-coloured selection, recording indicators and page-position dots. The 64-step note capacity is inherited; its complete visualisation is improved.
- **Live granular INSERT**, with SIZE/DENSITY, PITCH/MIX, POS/SPREAD and LIVE/FREEZE. Active effect settings and frozen history survive sound changes.
- **Per-synth DUCK switches** select which parts respond to the kick. Overall depth remains under MASTER → DUCK.
- **Measured dual-core balancing** distributes track rendering by recent cost. GRANUL at MIX 0 pauses capture after its fade and retains earlier history.
- **Per-track MIDI THRU** connects live keyboard input to external instruments, including the Behringer D MINI, through USB MIDI and a host. The web editor supports the expanded tracks, patterns and arrangement.

The USB MIDI port is **piterfly**; update mode is **piterfly Update**. The application and package retain the Piterfly identity **FM-1_1120101** introduced in 1.0.1; the loader reports **ota-FM-1_1120101**.

## Install and migrate

Back up before upgrading an older layout. **USR1 remains an 80 KiB sample bank; USR2/3/4 are used for patterns.** Factory samples and SYN1–SYN4 remain available.

Projects use **FUN9, 6,016 bytes**. FUN1–FUN7 import retains the original synth parts, moves drums to track 8, and starts the added synth parts empty. Settings use **PER4** and the editor protocol is **17**. Notes, sounds, MIDI settings and saved patterns survive restart. Live effect audio histories are temporary.

Song-file import replaces all 24 slots and the working loop, including empty slots. Use the matching 1.1.0 editor. Retain an older backup for returning to a firmware with another storage layout.

## Release package

`build/releases/piterfly-1.1.0/` contains:

- `piterfly-1.1.0.fwsc` — firmware and update loader, identity **FM-1_1120101**;
- app and OTA binaries;
- `piterfly-1.1.0-source.tar.gz` — exact build sources and source manifest;
- `piterfly-1.1.0-licences.zip` — notices and licence texts;
- `BUILD_INFO.json`, `SHA256SUMS` and the build log.

The package uses the accepted implementation from `ee3969b`, with release version 1.1.0. Full source hashes identify the frozen release snapshot. Historical SLOOP artwork, comparison screenshots and promotional audio/video archives are excluded from the firmware source bundle; current firmware uses the approved piterfly artwork.

```sh
PYTHON=/path/to/python-with-pillow \
JIELI_TOOLCHAIN=/path/to/pinned/toolchain \
AC79_SDK=/path/to/sdk \
/path/to/python-with-pillow tools/prepare_release.py

python3 web/make_site.py \
  build/releases/piterfly-1.1.0/piterfly-1.1.0.fwsc 1.1.0 build/site-1.1.0 \
  --release-dir build/releases/piterfly-1.1.0
```

The tool uses the pinned JieLi toolchain and AC79NN SDK inputs, builds from the copied source snapshot, and records source and output hashes. It prepares files locally and does not install firmware on the device.

## Documentation and films

[Illustrated Polish guide](PITERFLY.md) · [Screen gallery](SCREENSHOTS-1.1.0.md) · [Seven-track controls](SEVEN-SYNTH-TRACKS.md) · [Granular controls](LIVE-GRANULAR.md) · [Performance implementation](GRANULAR-PERFORMANCE.md) · [Build instructions](../BUILDING.md)

Promotional media is maintained separately from the public firmware source distribution.

SLOOP/Felucca authorship and GPL-3.0 notices remain intact. See [licensing](../LICENSING.md) and [third-party notices](../THIRD_PARTY_NOTICES.md).
