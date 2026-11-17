# Track controls — piterfly 1.2.4

The current layout is **six synth parts + two drum tracks**. The published 1.1.0 release used 7+1; its hardware verification does not cover version 1.2.4.

| Tracks | Instrument | Default MIDI channels | Voices |
| --- | --- | --- | --- |
| 1–6 | Synth, with independent engine/preset | 1–6 | Eight synth voices shared across all six parts |
| 7 | Drum part 1; set its channel with CH7 | 7 | Six drum voices |
| 8 | Drum part 2; set its channel with DRUM CH | 10 | Six drum voices |

PRESETS selects the track on ordinary parameter pages. ALGORITHM changes the synth sound or drum kit. In NOTES, PRESETS changes zoom and ALGORITHM selects the track. HOME and REC show groups 1–4 and 5–8. GLO-held mute/solo keys and the four level knobs operate on the visible group; GLO plus the second black key switches groups.

## Drum tracks 7 and 8

Select the desired drum track before opening DRUMS. Each part has its own kit selection, 16 lanes, 64 steps, levels, ratchets, nudges, fills, locks, per-lane TUNE/DECAY, pan, sends, filter, slicer and insert. Their voices and hi-hat choke are independent. Sample libraries and SYN1–SYN4 kit definitions are shared presets; selecting and tuning a kit on one part does not select or tune it on the other.

Track 7 uses its own LEVEL, REV and DLY track parameters. Track 8 retains the existing global drum LEVEL, REV and DLY controls. The on-device pages and protocol-19 editor route these controls to the correct part.

## DUCK and MIDI

FX → FILTER → knob 2 enables DUCK on each synth track 1–6. MASTER → DUCK controls depth. Kicks from either drum part trigger it; neither drum part is a DUCK target.

Each track has its own MIDI channel, THRU switch and MIDI Learn routing. Track 7 uses CH7/THRU7; track 8 retains DRUM CH and its existing THRU control. For an external-only part, keep its MIDI channel enabled, enable THRU and set internal LEVEL to zero. OUT = SEQ also sends recorded patterns. Connect external audio separately.

## Storage and validation

24 pattern slots and 128 SONG positions remain. A pattern holds all eight tracks. **Create new projects:** FUNB (6,160 bytes), editor protocol 19, settings PER4. Earlier FUN1–FUNA projects are not imported. All four former user-sample banks join the 352 KiB pattern/song pool; factory samples remain. Eight named song slots support save, load from a list, delete and export on the Mac, subject to available space. See [SONGS.md](SONGS.md).

[Build and release notes](RELEASE-1.2.4.md) · [Polska instrukcja](PITERFLY.md) · [Implementation and verification](SIX-SYNTH-TWO-DRUMS.md)
