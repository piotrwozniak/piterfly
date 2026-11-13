# Live granular INSERT — piterfly 1.1.0

Included in release 1.1.0, tested and accepted by the project owner on a physical FM-1.

GRANUL is a live effect available independently on the seven synth tracks and the drum track. It processes the part's playing audio; it is separate from the inherited GRAIN instrument engine.

## Controls

Select a track with PRESETS on an ordinary parameter page. Open **FX → INSERT** and turn **knob 1 / INSRT** to **GRANUL**. Knob 2 selects a parameter pair; knobs 3 and 4 edit it. SELECT continues to navigate FX pages.

| Pair | Knob 3 | Knob 4 |
| --- | --- | --- |
| 1 | SIZE | DENSITY |
| 2 | PITCH | MIX |
| 3 | POS | SPREAD |
| 4 | CAPTURE: LIVE / FREEZE | BUF: available history, read-only |

The assigned buffer capacity and capture state remain visible. Available history is dynamic: the shared allocator lends unused engine/effect pages and preserves recent history when pages are reclaimed. Read BUF for the actual duration rather than assuming one fixed duration for a given track count.

## Playing and changing sounds

LIVE captures the part's current sound. FREEZE holds the captured source while the grains keep playing, including after the original synth notes end. Mute still mutes the track.

Changing a factory sound, engine or user preset keeps an already active GRANUL insert, its controls and frozen history. LIVE continues with the new source. Loading a project replaces settings and clears live history.

At **MIX 0**, the wet signal fades to dry and processing parks: capture, FIR filtering, history writes and grains stop. Earlier history remains in memory; audio played during the pause is not captured. Increasing MIX resumes the effect, with a fresh capture-filter state to avoid filtering across the gap. TYPE OFF releases its memory share after the transition.

## Sound and processing

The source history is mono at 22.05 kHz, encoded with 8-bit mu-law. Dry audio retains its existing format. Four grains per track bound overlap. The original 23-tap anti-alias filter is calculated with twelve symmetric products, producing identical integer results; every mu-law input retains its original encoded value.

Both processor cores share whole track-rendering jobs according to measured duration. A track's synth, insert and related processing remain one unit. The scheduler retains shared-RNG and voice-transition restrictions. [Implementation and measured host counts](GRANULAR-PERFORMANCE.md).

## Storage

Notes and effect settings are persistent. Captured effect audio is a live buffer and is not saved into projects; it is cleared on power-off or project load. A restored FREEZE therefore starts with an empty source until LIVE captures audio.

Projects are **FUN9, 6,016 bytes**; legacy FUN1–FUN7 import is supported. Settings use **PER4** and INFO reports protocol **17**, 74 sound parameters and engine parameters starting at 66. USR1 remains for samples; USR2/3/4 hold persistent patterns.

## Verification

The owner confirmed operation on the physical FM-1. Automated coverage includes exact filter/codec comparisons, dynamic history relocation, LIVE/FREEZE and bypass/resume, eight simultaneous insert instances, serial/parallel audio equality, ASan/UBSan history bounds, ThreadSanitizer and browser/native audio parity. The figures in the performance notes are host instruction counts, not device CPU percentages.

GRANUL and DUCK process internal FM-1 audio. An external instrument’s separate audio output is mixed downstream; MIDI THRU does not bring it into the FM-1 audio path.
