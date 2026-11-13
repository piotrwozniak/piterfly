# Seven synth parts and one drum track

Included in `master`; originally developed on `feature/seven-synth-tracks`.

Tracks 1–7 are normal, internally sounding synth parts. Each has the same engine selection, presets, sound parameters, voice modes, arpeggiator, 64-step sequencer, micro timing, fill conditions, parameter locks, filter, inserts and MIDI routing as the original three parts. Track 8 is the existing 16-lane drum part.

The synth parts share **eight sounding voices**, with a shared pool of fade tails. The drum engine retains its own voices. Seven tracks do not multiply the synth polyphony by seven.

## Controls and external instruments

PRESETS selects any of the eight tracks on ordinary pages; ALGORITHM changes its sound. HOME and REC show the selected group of four: 1–4 or 5–8. GLO-held mute/solo keys and four level knobs operate on that group. The second black key while GLO is held switches groups. The four theme colours repeat in the second group.

GLO → MIDI assigns the original channels; MIDI 4–7 assigns the added synth parts. Defaults are channels 1–7 for synths and channel 10 for drums. MIDI THRU has independent switches for every part. MIDI Learn also addresses all eight parts.

A part sounds internally without any external instrument. To sequence an external instrument alone, leave its MIDI channel enabled and set the part's LEVEL to zero. MUTE and solo continue to control the part's performance, including note generation.

## Per-track DUCK

`master` includes one DUCK switch for each synth part, originally developed on
`feature/per-track-duck`. The previous baseline without these switches is
preserved at `4ff0fb0`. Select a synth track,
open FX → FILTER, and turn **knob 2: DUCK ON/OFF**. MASTER → DUCK and FX-held knob 3
still set the overall depth. The kick trigger and tempo-dependent eighth-note
recovery are unchanged. The drum track remains the source and has no target switch.
The web editor offers the same switch next to FILTER and a DUCK button on each
synth strip in LIVE. ON means the kick can duck that part; depth zero still disables
the effect globally. Target changes ramp over the existing audio control block.

Every target starts ON, preserving the previous sound. The flags belong to the
pattern/working project and survive saves and restarts. They are not sound-preset
parameters, MIDI Learn targets or parameter locks. Texture's existing DUCK switch
continues to work independently.

The build stores a seven-bit exclusion mask in FUN9's existing spare
bytes (mask plus `0xD1` marker), leaving the 6,016-byte size and checksum method
unchanged. Old projects restore all targets ON. The editor discovers globals 62–68
through the existing protocol; INFO reports 69 globals, with no change to protocol
17, sound parameter IDs, firmware version or updater identity.

For source rollback to the baseline without per-track DUCK, commit local work and
run `git switch --detach 4ff0fb0`; `git switch master` returns to the accepted version.
The previous DUCK version without granular performance changes remains on
`feature/per-track-duck`. Switching Git branches does not change firmware installed
on a device. The baseline package is
`build/seven-synth-tracks-logo/felucca-1.0.0-7tracks.fwsc`; the previous DUCK package
is kept separately under `build/feature-per-track-duck-logo/`.
The unmodified baseline reader was tested against a FUN9 file containing DUCK flags: it
loads the musical data and ignores the switches. Saving again with baseline
firmware clears the extra switches, so returning to the version with switches will restore all targets ON.

The 1.1.0 release uses 581,500 bytes of its 581,564-byte app
slot, leaving 64 bytes; main RAM usage is 96,292/98,304 bytes and low RAM is
28,832/29,384 bytes. POOL usage remains unchanged. See
[granular performance](GRANULAR-PERFORMANCE.md) for the package and validation.
The approved arcade splash is restored pixel-for-pixel using lossless packing;
build overflow now fails instead of silently substituting a text logo. This
boot-image repair does not optimize audio performance. See the
[audio slowdown investigation](audits/2026-11-13-audio-performance/REPORT.md)
for measured host costs and the recorded host measurements.
`tests/duck_targets_test.c` checks audible exclusion on every synth part, all 128
masks through simulated flash, legacy defaults, autosave restore, native controls,
live kick triggering and eight-track playback/overdub while targets change. The
feature test also passed ASan/UBSan and TSan with two host render threads. Browser
preview comparisons include DUCK ON/OFF against native DSP. The user tested the
granular performance build on the FM-1 and accepted it on 2026-11-13; this accepted implementation is included in release 1.1.0.

## Persistent data and compatibility

The build retains **24 stored patterns, 128 SONG positions and 64 note steps per track**. Four edited pattern snapshots can still be protected in RAM while playing. Patterns and the working project are saved to flash; notes, sounds, locks, drum tuning and device MIDI settings survive a restart.

USR1 remains an **80 KiB** user sample bank. The former USR2 and USR3 banks join the former USR4 area as pattern storage. Factory samples, user synth presets, FM6 banks, SYN drum kits and the update staging area retain their regions. Retired sample/kit IDs stay silent rather than being remapped to other material.

New projects use **FUN9**, a 6,016-byte, losslessly packed record. Synth steps retain all four notes, time, flags, velocity, levels and ratchets. The drum data and per-lane tuning retain their full precision. Old FUN1–FUN7 projects are imported; their drum part moves from track 4 to track 8, and the added synth parts start empty with their normal default sounds. Old drum-channel, THRU and MIDI Learn assignments follow the drum part.

The journal has 34 records of two sectors each. Twenty-four logical records hold patterns, two alternate the working-project roots, and the remaining records provide space to commit replacements before reclaiming old ones. Payload CRC is verified before publishing the header. Legacy records remain protected until their replacements commit.

Back up before installing or downgrading. This build reuses the sample contents of USR2/3; FUN9 requires the new firmware. The editor rejects incompatible restores before writing objects. The matching release package is `build/releases/piterfly-1.1.0/piterfly-1.1.0.fwsc`.

## RAM and live effects

The engine arenas, SLICER recordings and INSERT delay lines use fixed addresses within a shared 137 KiB audio pool. Unclaimed pages are loaned to the live granular INSERT. Its maximum shared history remains 72 KiB; the actual history gets shorter as other parts need memory. The GRANUL page shows the available duration. Page reclamation preserves recent history before an engine or effect can overwrite it.

All engines, inserts and 16-bit SLICER histories remain available. The worst-case allocation includes seven large engine arenas, eight SLICER histories, chorus/flanger fade tails and at least one granular history page per track. Synth INSERT lines fit in unused space at the ends of engine arenas.

Audio histories are live buffers. Musical events and sound settings are persistent; recordings inside live effects are not stored in project files.

The smaller canvas renders the same screens in bands. FFT calculations borrow the canvas only after the previous LCD DMA completes. Runtime voices and legacy conversion buffers no longer require permanently allocated per-track copies.

## Validation

`tests/seven_tracks_test.c` uses the actual firmware, UI, protocol and simulated NOR flash, compiled for seven synth parts. It checks every engine on every synth part, shared-voice stealing, engine changes, history relocation, simultaneous effect allocations, all 24 full-size patterns, restart, MIDI Learn migration, routing, backup staging ownership, USB high mask bits and all native track views. It runs with one and two host render threads, and under AddressSanitizer/UBSan.

`tests/project_codec_test.c` checks every supported parameter range, 200,000 step combinations and all parameter-lock positions/values. The journal test injects failures throughout page writes and commit headers. Existing legacy-format, DSP, UI, protocol and browser-preview tests remain in `tests/run_tests.sh`. The legacy three-part configuration is kept explicitly in the old audio-golden harness; the seven-part tests exercise the production allocator and shared buffers.

The firmware is compiled against the FM-1 linker limits. Static target checks retain the existing engine render-loop budgets. The audio interrupt's control dispatch and granular page-layout checks do more block-rate work for seven parts; their budgets document that increase. The loop analyzer distinguishes natural loops from backwards branches caused by cold-block layout.

Release 1.1.0 includes the granular performance implementation tested and accepted by the owner on the physical FM-1.
