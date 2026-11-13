# Audio slowdown investigation — 2026-11-13

Follow-up implementation: [granular performance branch](../../GRANULAR-PERFORMANCE.md).

The largest measured incremental workload is the live **GRANUL insert**, multiplied by the number of instances. A separate scheduling regression leaves the first six synth tracks on CPU0. These are confirmed code/host findings; the exact cause of the user's observed tempo slowdown still needs device timing and confirmation of the clock source.

No DSP, track count, effect behaviour or device data was changed during this investigation. The concurrent logo repair only restores the boot image and is not an audio optimization.

## Measurement

Compared source snapshots `6d592ba` (three synth tracks), `22355d9` (seven synth tracks, before per-track DUCK), and `025fd06` (seven synth tracks with DUCK). The later logo fix leaves their DSP unchanged.

The diagnostic includes the actual `tests/hostsim.c` renderer from each snapshot, builds with Apple clang `-O2`, settles for 4,000 blocks, then counts process instructions across 30,000 blocks. Results below are medians of three runs. Eight synth voices remain shared, rather than increasing with track count. ANALOG preset 0, sustained POLY notes, sustain 127, no distortion, neutral filters and zero sends are used unless the case says otherwise. Periodic kick hits are added except in idle tests. Shared effect buses still run. The inline CPU1 test double executes both cores' work serially: these counts measure **aggregate host work**, not target cycles, device CPU load or dual-core elapsed time.

The snapshots and diagnostic sources/scripts are retained in `build/performance-analysis/`. Raw counted results are in [host-results.json](host-results.json). Separate instrumented copies time calls and count dispatch masks; the table uses copies without those timers. One dispatch counter per block is present in every variant. Host instruction counts cannot model AC79 flash/cache contention, IRQ nesting or DMA deadlines.

## 1. Live granular insert is the largest measured added cost

Seven-track DUCK build; fixed eight ANALOG voices:

| Playing synth parts | GRANUL instances | Instructions / 32-frame block | Change from the matching dry case |
| --- | ---: | ---: | ---: |
| 3 | 0 | 61,478 | reference |
| 3 | 1 | 78,481 | +27.7% |
| 3 | 3 | 112,478 | +83.0% |
| 7 | 0 | 68,654 | reference |
| 7 | 7 | 187,592 | +173.2% (2.73× total) |
| 7 | 7, MIX = 0 | 114,789 | +67.2% |

In `firmware/src/granular.c:gf_insert`, LIVE capture maintains a 23-tap FIR at half output rate, encodes mu-law history and, when wet, interpolates/window-mixes up to four grains per instance. At seven instances the FIR alone performs about 3.55 million coefficient products per second, before buffer handling and grain playback. That is an operation count, not a target CPU percentage.

MIX = 0 skips wet grains after the fade, but **does not bypass capture, filtering and history writes**. This preserves pre-effect history; suspending it would be a deliberate behaviour change, not a free optimization.

`mix_part_run` explicitly continues processing a GRANUL part even when no voice sounds. Seven silent GRANUL instances cost 148,417 host instructions/block versus 18,675 with no instances; MIX = 0 still costs approximately 75,630. Voice shedding therefore cannot remove this fixed effect workload. Muting and changing TYPE to OFF are different operations; reducing only MIX is not a reliable CPU bypass.

These numbers concern the live insert. The separate **GRAIN instrument** was measured independently: the three-part, eight-voice preset-0 case costs approximately 85,679 instructions/block in the new build. Other sources, grain settings, engines and patches can change the ranking.

## 2. The default CPU split was not adapted to seven synth parts

`firmware/src/fx.c:units_split` defaults to `(1u << (NPART - 1u)) | U_DRUMS`.

* Old build: CPU1 receives synth track 3 plus drums; CPU0 receives synth tracks 1–2.
* New build: CPU1 receives synth track 7 plus drums; CPU0 receives synth tracks 1–6.

When only the first three synth tracks play, **all synth rendering now stays on CPU0**. Dispatch counters confirm this in the benchmark: approximately 99.91% of blocks give CPU1 tracks 7 + drums, and the remaining approximately 0.09% give it only drums during LFO restrictions. The active synth work is not balanced dynamically.

The seven-part safety scan additionally returns a zero worker mask on transition/pending-note blocks to avoid concurrent voice-allocation races. A dense SAMPLE test (240 BPM, seven four-note patterns with ratchets) runs completely on CPU0 for 6.97% of measured blocks. This is workload-dependent; ordinary sustained ANALOG cases do not show that serial fallback. Removing the guard without fixing ownership would reintroduce a race.

Aggregate work increases alone understate this regression: even equal total instructions can take longer when a previously parallel active part moves to CPU0. Actual speedup or lost deadline margin must be measured on the two-core device.

## 3. Track-count overhead exists, but is smaller than many active inserts

With identical eight-voice music on the first three parts, moving from the three-part to seven-part build raises aggregate host instructions by approximately **7.1%** for sustained ANALOG, **8.9%** for FM6, and **11.0%** for the simple three-part sequencer case. Idle work rises about **27.1%**, from a much smaller base. Distributing eight ANALOG voices across seven playing parts rather than three adds approximately **11.7%** in the current build.

Extra control work includes two `audio_prepare`/`gf_layout` checks per 32-frame block, seven parts' LFO/envelope/engine control ticks, indirect shared-voice lookup, MIDI/sequencer dispatch and the worker safety scan. At 44.1 kHz this control block repeats about 1,378 times/second. Pattern capacity itself does not multiply the per-sample synthesis cost.

## 4. Per-track DUCK is a small contributor in these cases

Comparing seven-track before-DUCK and DUCK snapshots on the same workloads changes aggregate host instructions by approximately **0.03–0.14%**. It adds a seven-bit target scan and block gain selection, not another synthesis/effect pass. These measurements do not support removing DUCK as the main performance remedy.

## Why overload can sound like tempo slowing

`audio.c:fm1_alnk0_irq` renders 256 frames per DMA half: approximately **5.805 ms** available. The internal sequencer in `seq.c:events_block` advances by rendered samples (`n * BPM`), not by elapsed wall time. If rendering misses DMA halves, repeated/stale audio and fewer sequencer advances per wall-clock second are a plausible cause of the reported slowdown. The IRQ counts late halves but has no corresponding missed-half sequencer catch-up.

This is a mechanism supported by the code, not a captured fault on the user's device. The user subsequently confirmed SYNC INT, so an external MIDI clock was not driving the reported session.

Ordinary autosave and deferred pattern/settings writes explicitly return while `song.playing` is true. They are not the leading explanation for continuous playback slowdown. Manual actions or section transitions remain separate peak-load cases. Boot-logo decompression happens before audio starts.

## Next changes, in order

1. Balance active render units between the cores while retaining transition/RNG/voice-ownership safety. This can preserve all seven tracks.
2. Profile and reduce live capture/grain cost, then establish a measured supported limit for simultaneous GRANUL instances. Decide explicitly whether MIX = 0 should continue pre-capture. Do not reduce anti-alias filtering silently or assume a smaller history buffer solves CPU cost.
3. Verify the user's exact pattern on hardware: internal/external clock, average and maximum half duration, late-half deltas, shed voices, CPU1 busy/wait/timeouts and granular timings. Existing console diagnostics expose these when built with CDC; the ordinary MIDI LIVE response exposes only averaged CPU, not enough to prove deadline safety.

The current Mac MIDI inventory contains only MidiPipe, so no FM-1 timing/state was captured. No device was flashed. The complete host/web suite for the restored-logo feature build passed, but its passing regression budgets do **not** establish seven-track worst-case real-time feasibility: the inherited audio-golden CPU harness deliberately retains the three-part configuration.
