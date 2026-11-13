# Granular performance branch

Branch: `feature/granular-performance`, based on `feature/per-track-duck` at `982dcb0`.

Accepted into `master` on 2026-11-13 after the user tested `d7c23c7` on the FM-1 and reported that it works well. This is a user listening/playback check, not a captured timing benchmark. The feature branch remains available.

The seven synth tracks, drum track, shared eight-voice synth budget, per-track DUCK, 24 patterns, USR1 and FUN9 storage layout are retained. There is no new project format, parameter or editor protocol. The user confirmed that the reported slowdown uses the internal clock.

## Changes

* Every render unit records its duration using the existing crystal-derived 24 MHz timer. After joining the previous block, CPU0 assigns the longest recent units first to the less loaded core. A unit is a whole synth track with its inserts, or the drum track with its insert. Multiple GRANUL inserts can therefore run on different cores. A single insert is not split between cores. Automatic balancing replaces the fixed track-7-plus-drums worker mask; explicit diagnostic masks and the 2ND CORE OFF setting retain their meaning.
* Existing restrictions remain: voice-allocation/retrigger transitions run serially; shared-RNG LFO/PHYS work and multiple FORMANT tracks stay on CPU0 as required. Ownership changes and granular history relocation finish before either core renders. Per-unit timings are read only after the previous job joins.
* MIX 0 parks GRANUL after its wet ramp reaches zero. It stops FIR capture, history writes and grain playback. Existing history, including FREEZE, remains available when MIX rises again. Audio during bypass is not recorded into that history. The capture FIR starts clean when processing resumes so it does not filter across the pause.
* Symmetric capture taps require twelve products instead of 23, with the exact same integer result. Mu-law encoding uses leading-zero count instead of a shift loop; all 65,536 input values produce the same bytes. Cutoff, sample rate, codec precision and four-grain overlap remain unchanged.

The unchanged arcade image uses three colour bits per RLE token instead of four, allowing 32-pixel runs. This saves 109 bytes of flash with **identical approved RGB565 pixels**. Logo unpacking happens before audio starts; it is not an audio performance change. There is no fallback to another logo.

## Evidence and limits

In repeatable host tests with all work executed serially, seven active inserts reduced aggregate render instructions by approximately **3.7%**. With seven configured inserts at MIX 0, aggregate work fell approximately **39.7%**, leaving about 1% above the matching dry case. These compare the same eight-voice scene, compiler, dispatch mode and three 30,000-block runs. Timers are zero in that aggregate host fixture, so it does not claim to measure real timer-access overhead. The additional benefit from using both cores is not included in those percentages.

These are host instruction counts, not FM-1 CPU percentages or a measured two-core speedup. Balancing also cannot parallelize one indivisible heavy track, and safety-restricted blocks still run on CPU0. No new fixed instance cap is imposed; available history follows the shared memory allocator. Per-unit timer reads and scheduling themselves add block-rate work.

The [before/after counts](audits/2026-11-13-audio-performance/host-comparison-after.json) are saved with the audit. Logs are under `build/granular-performance/`, including `host-instruction-comparison.json`, the full host/web suite, sanitizer runs, the fresh target budget check and the exact-pixel splash test. The static target proxy for `gf_insert` is 1,218 against the existing 1,120 budget, within its existing 10% tolerance. It counts loop structure rather than the changed 23-to-11 paired-loop trip count and cannot establish target cycle savings. Existing engine budgets were not raised.

`tests/granular_performance_test.c` checks 512,000 cost/eligibility combinations, 115,000 FIR rings, every codec input, parked dry history and resumed FREEZE/LIVE. Eight simultaneous granular instances with mixed engines, RNG-sensitive engines, note stealing, engine switches, page reclamation and bypass changes produce identical audio for serial, automatic and explicit splits on real host threads. ASan/UBSan includes history bounds; TSan includes real timing and randomized worker delays. Browser/native parity also exercises MIX 0 followed by FREEZE and LIVE resume.

The 1.1.0 release package is `build/releases/piterfly-1.1.0/piterfly-1.1.0.fwsc`, identity `FM-1_1120101`. The app uses **581,504 / 581,564 bytes** (60 free), main RAM **96,292 / 98,304**, low RAM **28,832 / 29,384**, and POOL **335,180 / 344,064**. Linker regions are unchanged. The public release retains the Piterfly updater identity introduced in 1.0.1. The small flash margin is a maintenance limitation, not spare CPU capacity.

The owner tested and accepted this implementation on the physical FM-1. It is included in release 1.1.0. The instruction-count figures above remain explicitly host measurements; they are not presented as device CPU percentages.

## Rollback

The previous corrected-logo DUCK package is preserved at `build/feature-per-track-duck-logo/felucca-1.0.0-7tracks.fwsc`. Reinstall it to return to the previous behaviour. The baseline without per-track DUCK is separately preserved at `build/seven-synth-tracks-logo/felucca-1.0.0-7tracks.fwsc`. Switching Git branches alone does not change installed firmware. Pattern and sound storage did not change in this branch.
