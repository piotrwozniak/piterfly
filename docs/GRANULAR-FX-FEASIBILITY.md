# Live granular FX feasibility — FM-1 / piterfly

Analysis date: 2026-11-08. Firmware baseline: `92bead1`.
This is the original design study. A live µ-law prototype has since been implemented on `codex/live-granular-fx`: see [the implementation guide](LIVE-GRANULAR.md) for its actual 72 KiB pool, controls, storage changes and outstanding hardware measurements. The calculations below record the earlier design alternatives.

## Hardware and timing

FM-1 uses the JieLi AC7911B / AC791N (WL82) family: two proprietary 32-bit DSP cores with single-precision floating point. The manufacturer's SDK specifies up to **320 MHz and 578 KB total SRAM**, including resources that are not available as application buffers. Selected family packages support SDRAM; this firmware's FM-1 memory map provides no external SDRAM arena. Do not budget optional family SDRAM as installed memory.

The project's `firmware/hal/fm1_sys.h` documents a **360 MHz bootloader/stock-firmware operating point**, and raises the supplies to stock V15 values before starting both cores. This is repository evidence, not a fresh clock measurement on this particular FM-1. It differs from the vendor's published 320 MHz rating. This proposal requires no clock or voltage changes. Profile using the independent 24 MHz TIMER4 rather than assuming instruction counts equal cycles.

Audio runs nominally at 44,100 Hz, in 32-frame DSP blocks inside a 256-frame DMA half. A half has approximately **5.805 ms** to finish, including nested USB/control work. A 32-frame block spans 0.726 ms. Both cores already render instruments; the second core is not spare. Shared FX execute after the render jobs join on core 0. A per-track insert would execute on whichever core renders that track.

The audio ISR already measures average render load, maximum duration and late DMA halves; two consecutive halves above 85% request voice shedding. An effect that silently increases voice shedding is not a successful performance result even if audio continues.

Primary manufacturer sources:

- [JieLi datasheet catalogue and resource summary](https://doc.zh-jieli.com/SDKWiki/master/Jieli-Tech/fw-AC79_AIoT_SDK/9-developer-resources/9.1-docs-datasheets.html)
- [JieLi SDK hardware specifications](https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK/blob/release/AC79NN_SDK_V1.2.0/README-en.md)
- [FM-1 hardware investigation and board photographs](https://github.com/aroum/fm1-custom-fw/blob/main/REVERSE_ENGINEERING.md)

Local evidence: `firmware/app.ld`, `firmware/hal/fm1_sys.h`, `firmware/hal/fm1_time.h`, `firmware/src/audio.c`, `firmware/src/cpu1.c`, `firmware/src/fx.c`.

## Verified memory budget

Free address space in the existing linked ELF (stacks and guard regions are already separately reserved):

| Region | Free bytes |
| --- | ---: |
| RAM, after `.bss` | 34,148 |
| POOL | 10,164 |
| Low RAM | 6,612 |
| Total across these three regions | 50,924 |

These are separate regions, not one contiguous 50,924-byte allocation. NOINIT slack, stack areas and caches are excluded. Existing instrument arenas are unions actively used by the selected synth engines; they are not available for a simultaneous live effect.

Allocation-only tests used the current compiled firmware objects and the real pi32v2 linker/script, writing separate ELF files under `build/granular-study/`. They did not replace or package the installable firmware:

- Additional **32,768-byte PCM ring in BSS**, **2,048-byte provisional state budget in POOL**, and **256-byte two-core send scratch in POOL**: link passed.
- Additional **34,148 bytes in BSS**: link passed, reaching the region boundary.
- Additional **34,152 bytes in BSS**: link failed, overflowing by four bytes as expected.

The initial POOL-state probe is link-only: it leaves 7,860 bytes in POOL, below the build checker's required 8,192-byte reserve. A corrected probe places the 2 KiB provisional state in low RAM and leaves 1,380 bytes in RAM, 9,908 in POOL and 4,564 in low RAM; it passes linking and those memory headroom conditions. Neither probe implements a processor. The 2 KiB state allocation is a reservation, not a measured size of an implemented processor. The tested 32 KiB layout leaves 1,380 bytes in RAM, 7,860 in POOL and 6,612 in low RAM. Future parameter/UI/persistence additions must fit those remaining budgets too. Flash code space must also be checked after implementation.

## Buffer duration

PCM16 mono storage: `seconds = bytes / (sample_rate * 2)`. Stereo doubles memory. Output remains stereo/44.1 kHz even if capture is mono/22.05 kHz.

| Capture format | One shared 32 KiB ring | Four equal independent rings, 8 KiB each |
| --- | ---: | ---: |
| Mono, 44.1 kHz | 371.5 ms | 92.9 ms per track |
| Mono, 22.05 kHz | **743.0 ms** | **185.8 ms per track** |
| Mono, 11.025 kHz | 1,486.1 ms | 371.5 ms per track |
| Stereo, 44.1 kHz | 185.8 ms | 46.4 ms per track |

These durations describe the complete ring, not a guaranteed maximum grain length at every pitch/direction. A moving write head needs a safe read distance and valid-history checks. Grain size must account for pitch, reverse playback and how much history has actually been captured. FREEZE stops overwrite, permitting longer reads from the captured material.

**Largest contiguous additional PCM allocation in the current layout:** 34,148 bytes, or 387.2 ms at 44.1 kHz / 774.3 ms at 22.05 kHz, with state elsewhere. This consumes all remaining main RAM and is a ceiling, not a sensible shipping allocation. More is possible with segmented buffers or relocation, but is not proved by these tests and consumes implementation headroom.

11.025 kHz is an optional lo-fi quality mode (bandwidth below 5.5 kHz), not a free quality-preserving extension. Downsampling needs an anti-alias filter; simple sample dropping is unsuitable. Grain pitch-up also needs appropriate filtering/limits to control aliasing.

The existing delay alone owns 131,072 bytes. Replacing/reusing it could provide 1.486 s mono at 44.1 kHz or 2.972 s at 22.05 kHz, but independent delay and granular capture cannot both own the same memory. Do not remove delay or additional sample banks for the first version. Reducing sample flash storage does not by itself create PCM RAM.

## Routing choices

### Independent per-track inserts

Add `GRAIN` to the existing insert types. Each of the three synth tracks and the drum track gets its own ring and grain state. It works on rendered audio from any instrument, including FM, physical modelling, sample and drum synthesis. The synth-track insertion point is after distortion and before track filter/slicer/level/sends; the drum path needs an explicit matching integration and a stereo-to-mono capture decision.

Advantages: independent settings and FREEZE per track; existing inserts already have a parameter/UI model; naturally shares DSP work across the cores. No separate master granular stage is necessary to make all four tracks granular.

Trade-offs: with a total 32 KiB PCM budget, each track gets only 186 ms at 22.05 kHz. A mono wet processor loses the drum signal's original stereo detail in the wet component, although the dry path can retain it. Selecting this insert replaces the other insert on that track. Four processed tracks are not equivalent to granulating the summed song: they have separate histories and grain events.

Suggested first prototype: four 4,096-sample PCM16 rings at 22.05 kHz, at most four active grains per track, hard cap of 16 total. Start with existing insert controls for SIZE, DENSITY, PITCH and MIX. Freeze/position/reverse controls need an explicit additional UI and storage design; do not hide them behind undocumented key combinations or overwrite unrelated parameters.

### One shared bus with individual track sends

Add a mono send accumulator to each core, sum it when the cores join, capture into one 16,384-sample PCM16 ring, process once and return stereo wet audio. Keep the existing delay, chorus and reverb.

Advantages: 743 ms at 22.05 kHz; larger grains/clouds; one bounded grain pool (start at eight grains). Each track chooses how much audio it contributes. Sending all four tracks processes their sum.

Trade-offs: sends are individually adjustable, but SIZE/PITCH/FREEZE and captured history are shared. Freezing the melody alone while the drums keep updating the same ring is impossible. Returning wet audio after the bus join consumes core 0's remaining time. All-track sends omit sources such as the global texture bed unless those are explicitly routed; they also do not include downstream master effects. A true post-effects master mode requires a separate tap and protection from unintended feedback.

UI: a GRAN send on each track's FX pages; global granular settings under GLO, using the established compact page indicators. Adding send/global fields requires versioned persistence and editor-protocol work. Existing `P_COUNT`/project records must not simply grow without migration.

### Recommendation

For **independent sound treatment per track**, prototype the per-track insert first. It matches the requested workflow and avoids another master effect. The 186 ms capture history suits short grains, stutter and metallic/time-smearing effects; it is limited for long ambient textures.

For **longer captured phrases/clouds**, choose the shared bus instead. Its per-track sends are useful but must not be described as four independent effects. Do not implement both simultaneously in version one. The routing decision is musical as well as technical.

## Performance: established facts versus outstanding measurement

The existing GRAIN instrument's source reports host instruction counts around 1,050 instructions/output sample per part, approximately 1,170 while indexing. Those are not target CPU cycles or a reusable percentage of FM-1 load. That instrument performs flash ADPCM seeking/decoding; a live PCM ring avoids that work but adds capture/resampling, interpolation, windowing, grain management and mixing.

Useful workload bounds for a candidate live processor, before capture/filter overhead:

| Configuration | Maximum grain updates/second at 44.1 kHz output |
| --- | ---: |
| Shared bus, eight grains | 352,800 |
| Four tracks, four grains each | 705,600 |
| Four tracks, eight grains each | 1,411,200 |

For perspective only: 320 MHz corresponds to approximately 7,256 core clock cycles/output frame; 360 MHz to 8,163. A hypothetical 10% budget for an eight-grain shared processor is only about 91 or 102 cycles per grain update before capture/filter/mix overhead. This is a budgeting constraint, **not an estimate of compiled execution cost**, and the two cores' cycle budgets cannot simply be added for serial bus processing. Running capture at half rate does not halve grain rendering cost when wet output remains 44.1 kHz.

There is no live granular implementation to benchmark yet, and no connected FM-1 timing measurement was taken. A desktop host benchmark would not resolve the target CPU/cache/shared-memory cost. Therefore neither a guaranteed maximum grain count nor a credible CPU percentage can be stated yet. Memory feasibility is established; realtime feasibility remains to be measured.

## Prototype acceptance measurements

Use TIMER4 to instrument only capture/grain processing and record worst-case as well as average time. Compare identical scenes with FX bypassed/enabled. Use existing audio max/late diagnostics, core-1 busy/wait/timeout counters, stack watermark/guards, and add an explicit counter for voice shedding. Average CPU alone can hide deadline overruns and USB nesting.

Exercise all four tracks, worst polyphony of FM6/PHYS/current GRAIN, full drum activity, existing delay/reverb/chorus, USB audio and MIDI, UI navigation, pattern/SONG changes, maximum grain density/pitch/reverse and FREEZE transitions. Include core 1 disabled and runtime fallback. Do not assume full-density FX can coexist with every existing worst-case scene: if necessary, reduce new-grain admission without truncating existing grains abruptly.

A useful initial target is <=10% incremental audio-half time in the shared design, with total worst-case deadline occupancy below 80% in the documented supported stress scenes, no late halves, no new core timeouts, intact guards and no additional voice shedding. These are proposed acceptance targets, not current results. Per-track inserts need measurements per core and on the complete joined pipeline.

Audio tests must additionally cover empty/partially filled buffers, wraparound, read/write collision guards at every pitch/direction, bounded feedback, safe gain with all sends at maximum, transitions without clicks, tail/bypass behavior and correct clearing when a track/project changes. Ring contents are volatile audio history; stored settings can persist, but the recording itself should not be written to flash continuously.


## Longer buffers: follow-up investigation

The earlier 743 ms figure was a conservative **32 KiB / PCM16 / mono / 22.05 kHz design choice**, not a hardware maximum for every possible encoding and layout. The user now prioritises longer ambient capture, so independent 186 ms per-track PCM buffers should not be treated as the preferred design without reconsidering these options.

### Actual large allocations in the target ELF

The target `objdump -t build/felucca.elf` confirms:

| Allocation | Bytes | Current purpose |
| --- | ---: | --- |
| `dly_buf` | 131,072 | Delay, PCM16 |
| `punch_ring` | 65,536 | Whole-mix replay effects: loop/stutter/reverse/stop/half/rewind/etc. |
| `cv_px` | 59,520 | 240 × 124 RGB565 drawing canvas |
| `sl_buf` | 32,768 | Four per-track slicer capture rings |
| `eng_arena` | 25,776 | Three engine state arenas, already shared by union |

These allocations are not unused just because the relevant effect is idle. In particular `punch_process` updates the replay ring even when no punch effect is engaged, so borrowing it requires an explicit ownership/routing change. Its original whole-mix history cannot simultaneously act as four independent per-track histories.

### 8-bit µ-law: double history without lowering sample rate

Use nonlinear 8-bit companding, not simply dropping the low eight PCM bits. Capture encodes each sample, reads decode it (a 256-entry PCM16 lookup table can live in flash), and interpolation/mixing operate on decoded PCM values. Each source sample is independently addressable, so arbitrary positions and reverse reads do not require replaying a decoder from a previous checkpoint.

A 32 KiB ring holds **1.486 s at 22.05 kHz**. This preserves the nominal bandwidth of that sample rate but introduces lossy quantisation. It is not 16-bit quality, and frozen quiet tails/pitched material require listening tests. Output and dry audio can remain stereo at 44.1 kHz. All history figures here refer to mono captured audio.

There is a directly relevant primary implementation: [Mutable Instruments Clouds audio buffer](https://github.com/pichenettes/eurorack/blob/master/clouds/dsp/audio_buffer.h) supports µ-law sample storage and interpolation; its [processor](https://github.com/pichenettes/eurorack/blob/master/clouds/dsp/granular_processor.h) uses that format for its reduced-fidelity buffers. [Its encoder/lookup decoder](https://github.com/pichenettes/eurorack/blob/master/clouds/dsp/mu_law.h) shows how bounded this conversion can be. This is evidence of an appropriate technique, not a benchmark on the FM-1 or a proposal to port all of Clouds.

### Recover RAM from the graphics canvas

The current 59,520-byte canvas is a drawing scratch area, not the panel's retained screen image. A 240 × 32 RGB565 scratch band would use **15,360 bytes**, releasing **44,160 bytes**. Together with the initial 32 KiB pool this gives **76,928 bytes** of history storage:

- PCM16 / 22.05 kHz: **1.744 s**.
- µ-law8 / 22.05 kHz: **3.489 s**.

This does not remove tracks, sample banks, delay, reverb, slicer or punch effects. However, it is an architectural UI change, not a one-line `CV_MAX` reduction: `cv_begin` currently clips an oversized request, and menu/visualiser rendering explicitly uses 124/120-row bands. All large drawing paths, graph coordinate offsets, blits and LCD DMA ownership need a banded rendering design. No audio IRQ may borrow scratch still being used by LCD DMA. Re-rendering geometry per band may increase CPU work or reveal tearing; hardware UI/audio profiling is essential.

Freed canvas memory is in POOL and the initial ring is in main RAM. A unified history would need segmented address mapping or a deliberate relocation of other allocations. Keep the 8 KiB POOL reserve. Small chunk/page mapping can avoid a division per grain read, but its actual target cost remains unmeasured. The 3.489 s is a gross storage calculation before new mapping/guard/filter/state overhead, not a tested final capacity or realtime guarantee. A shipping target nearer **3 seconds** gives a more realistic margin.

### Share memory with existing effects instead

Combining the initial 32 KiB and the 64 KiB punch replay allocation yields **96 KiB**: 2.229 s PCM16 or 4.458 s µ-law at 22.05 kHz, before new overhead. This preserves the independent delay/reverb but makes granular mutually exclusive with punch effects needing that replay history. Non-ring punch effects could remain if explicitly separated. Tail transitions and ownership must be designed; the ring is not free while merely idle.

Reusing the 128 KiB delay allocation plus the initial 32 KiB yields 160 KiB: 3.715 s PCM16 or 7.430 s µ-law at 22.05 kHz, before overhead. Independent delay then cannot retain its original buffer. Alternatively, lowering delay's own capture rate or companding its storage can release part of that memory while keeping its duration, but changes delay quality and requires feedback stability/quantisation tests. These are explicit trade-offs, not free capacity.

Slicer buffers and engine arenas offer smaller conditional gains, but tying granular capacity to instrument/slicer selection makes switching and persistence substantially harder. Reclaiming two entire tracks is unnecessary for the first longer-buffer approach.

### ADPCM and flash are not the preferred first approach

The code already decodes IMA ADPCM sources, and host tools encode them. Four-bit storage would nominally fit 2.972 s in 32 KiB at 22.05 kHz, but a live ring needs predictor/index checkpoints, block identities and decoded caches. For example, 256-sample independent blocks need 128 bytes of codes plus a header; cache and index storage reduce capacity further. Random grain starts, reverse playback, high pitch and many grains increase decoding work. It is worth considering only after µ-law measurements; the existing offline/sample decoder does not establish that live multi-grain ADPCM will meet the deadline.

User sample flash is persistent NOR, not spare audio RAM. Continuous ring writes would introduce erase latency, flash-busy/XIP constraints and wear. Do not solve this with live writes into the sample/pattern banks. Saving a stopped/frozen capture occasionally would be a separate feature.

### Revised recommendation and evidence boundary

Prototype µ-law in the already verified 32 KiB allocation first (about 1.49 s maximum for one owner). In parallel with actual performance/listening results, assess a smaller graphics canvas as the route to an approximately **3-second pool without disabling existing musical features**. Only if that UI change proves too costly should the user choose an explicit exclusive granular/punch or granular/delay mode.

Dynamic per-track division still applies: a three-second total pool gives about 3 / 1.5 / 1 / 0.75 seconds for one / two / three / four owners. It does not provide three seconds to each track simultaneously. Repartitioning must preserve valid data and handle FREEZE loss explicitly.

This follow-up verified symbol sizes, arithmetic and a corrected allocation-only link preserving the existing 8 KiB POOL rule (`build/granular-study/allocation-headroom-only.elf`, `headroom-result.json`). No graphics refactor, codec, granular DSP, listening assessment or target timing test has been implemented. Installable firmware remains unchanged.
