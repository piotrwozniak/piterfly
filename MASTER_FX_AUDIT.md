# Master FX and sidechain — phase 0 audit

Date: 2026-11-12. Baseline: `feature/seven-synth-tracks`, commit `a7b50e0`.
Scope: feasibility and proposed architecture only. No DSP implementation, firmware
version change, release replacement, publication or hardware flashing in this audit.

## Verdict

A lightweight stereo master compressor, adjustable saturation, sample-peak ceiling,
event-driven track ducking, presets and live overdub integration fit the existing
architecture. This does not require rewriting the sequencer or giving up synth tracks.

The complete specification is **conditional on reclaiming program space and measuring
the device under load**. The current image has only 2,940 bytes of application-slot
headroom. Neither the complete code size nor target CPU cost of the proposed effects
has been measured. Oversampling is outside the initial implementation.

Audio sidechain is possible, but requires an explicit detector tap and timing policy
because tracks render on two cores and share scratch buffers. Prefer event sidechain
first. Both cores already perform useful work; core 1 is not spare DSP capacity.

MIDI carries control events, not the external instrument's audio. The current firmware
does not route an external audio return into its mixer. Master FX can process the
FM-1's own output, and external MIDI notes can trigger ducking of its internal parts;
this cannot compress or duck the Behringer's actual audio outside the FM-1.

## Confirmed architecture

| Area | Evidence and current behavior |
| --- | --- |
| Tracks | `firmware/src/core.h`: seven synth parts and one drum track, 64 steps each. Synths share eight sounding voices; `drums.c` has six separate drum voices. Twelve synth engines in the default build. |
| Audio format | `core.h`, `audio.c`: nominal 44,100 Hz; fixed-point DSP, 32-frame control/render blocks; 256-frame DMA halves; output scaled for the codec's 24-bit stereo interface. |
| Timing | A block spans 0.726 ms and a DMA half 5.805 ms. `fx.c:lim_block` already delays audio by 32 frames for limiting. These are calculated buffer intervals, **not measured key-to-output latency**. |
| Entry point | `audio.c:fm1_alnk0_irq` renders eight control blocks per DMA half through `audio_block` and `fx.c:mix_block`. |
| Track processing | `fx.c:mix_part_run`: render → distortion → INSERT → SLICER → track filter → level/pan and sends. The drums have their own stereo/send path in `mix_drums`. |
| Shared processing | Two accumulator sets are joined before chorus, tempo delay and FDN reverb returns. Texture, DUST, punch effects and the DJ filter also contribute to the master path. |
| Existing protection | `fx.c:lim_block`, `knee`, `master_pre`: linked peak limiter, soft output knee, DC blocking and optional low cut. No claim of true-peak limiting. |
| Existing ducking | `fx.c:duck_block`: drum kick events dip synth parts, recovering over an eighth note. `drums.c:drum_on` supplies the trigger. Source/target and envelope are currently fixed. |
| DSP dependencies | `dsp.c` supplies integer helpers and interpolated tanh tables. Engine-specific code includes integer msfa FM6 and fixed-point DaisySP/Rings ports. No new DSP library is necessary for this proposal. |
| MIDI | `usb.c`, `midi_uart.c`, `seq.c`: USB/TRS MIDI input, USB MIDI output/THRU, clock input and USB clock output. The audio callback consumes queued events. |
| Recording | `seq.c:input_on`, `rec_note`, `rec_hit`, `seq_tick`: recording stores musical events while rendering continues. Live notes are sounded separately from writing the pattern. Undo and duplicate-playback suppression already exist. |
| Track selection | `seq.c:rec_follow`: record arm follows selection; a free take remains attached to its original track. Preserve this behavior. |
| Persistence | `project.c`, `project_codec.c`, `project_legacy.c`, `pattern_bank.c`: FUN9 projects are 6,016 bytes; 24 patterns plus alternating working-project roots in paired-sector journals. Settings and user presets have separate formats. |
| UI | `params.c`, `ui_*.c`: four controls per page, existing page groups and indicators. MASTER FX and SIDECHAIN need several pages rather than six controls squeezed onto one screen. |
| Browser | `web/editor.html`, `web/preview-worklet.js`, `firmware/host/preview.c`: protocol 17 and eight-track preview. New controls must reach all three, plus backup validation. |

The existing signal path is:

```text
MIDI / keys / sequencer events
  → synth render units and drum render unit
  → per-track effects, level/pan, existing ducking, dry/send accumulators
  → join render cores
  → texture + shared effect returns
  → DUST → punch effects → DJ filter
  → master level → DC/optional low cut → lookahead limiter → soft knee
  → codec output / USB recording
```

USB recording has two policies: MASTER follows the output level; FULL has its own
output-stage state and does not follow the volume knob. Preserve both.

## Verified memory budget

Source: the existing linked `build/feature-seven-synth-tracks/felucca.elf`, its
578,624-byte application image, `firmware/app.ld`, and `tools/build.py` checks.
ELF section sizes were inspected again during this audit.

| Region | Used bytes | Limit bytes | Remaining bytes |
| --- | ---: | ---: | ---: |
| Application image | 578,624 | 581,564 | **2,940** |
| Main RAM: data + BSS | 96,148 | 98,304 | **2,156** |
| Low RAM | 28,800 | 29,384 | **584** |
| POOL | 335,180 | 344,064 | **8,884** |
| NOINIT | 15,660 | 15,696 | **36** |

The builder requires 8,192 bytes of POOL headroom: only **692 bytes** are available
there without violating that reserve. These are separate regions, not a single
contiguous free allocation. Stacks are reserved separately; their runtime margins
must still be measured. The package file size is not the executable-image limit.

USR2/3 provided flash for persistent patterns, not free executable space or audio RAM.
Do not reuse the live granular/engine pages for simultaneous master-state storage.
The current build already uses the compact text boot-logo fallback to fit flash.

Provisional implementation budgets, **targets rather than measured processor sizes**:

- At most 1 KiB additional BSS for master state, parameters, smoothing and sidechain.
- No additional NOINIT or low-RAM allocation; avoid growing `track_t`.
- At most 512 bytes additional POOL use, including all four cached project copies.
- No full-song, per-track stereo history or new lookahead buffers.
- First establish a program-space recovery plan. A working target is 8–12 KiB of
  recovered space for DSP, UI and compatibility code, subject to actual compiled size.
  Achieve it through measured code/data reductions with unchanged behavior, not by
  silently deleting instruments, presets, legacy import or other existing functions.

CPU headroom is unknown on a physical FM-1. The current ISR tracks duration, late
halves and load; two consecutive halves above 85% request voice shedding. Existing
host instruction counts and static target-loop estimates do not establish a device
CPU percentage. Increased shedding is a regression, even if playback continues.

## Proposed smallest architecture

Keep the current audio path and insert one shared master processor after the DJ
filter and before the output-level split:

```text
current track FX + optional track ducking
  → current mixer and shared effects
  → optional lightweight master EQ
  → stereo-linked compressor without additional lookahead
  → compensated saturation
  → current master-level / USB FULL split
  → current DC/low-cut stages
  → existing limiter with optional ceiling control
  → existing output knee and output conversion
```

Process the common master module once, so USB FULL does not double compressor/EQ
work or get a second gain-reduction envelope. Retain separate existing DAC/USB
limiter state. Do not add a second limiter. Default bypass must take the original
path bit-for-bit; it bypasses new processing while retaining the old safety stages.

The brief names Master EQ but supplies no EQ controls or band requirements. For the
initial design, propose two fixed-frequency shelves with neutral defaults; defer a
parametric or multiband EQ. This is a proposed scope decision, not an existing feature.

The compressor should use linked L/R peak detection, bounded fixed-point arithmetic,
smoothed attack/release and gain, and a small table or approximation for its static
curve. Calculate parameter-dependent coefficients at control rate. Measure accuracy
and distortion rather than labelling an untested algorithm artifact-free.

Use one implementation for CLEAN/PUNCH/WARM/LOUD. Amount maps parameters toward
neutral values; 0 must reproduce the old path. Smooth preset and bypass changes
without requiring an additional stereo audio block. A sample-peak ceiling is feasible;
true-peak control and oversampling are not initial promises.

### Event sidechain first

Use one source selector, a target bitmask and one shared attack/release envelope.
This supports one source ducking any chosen subset of eight tracks, including an
individual target, without an 8×8 matrix of processors.

Hook accepted musical attacks at the common instrument/drum event path, after
existing routing and duplicate suppression. Do not also trigger from `rec_note`,
the MIDI input queue and MIDI output: recording one played note must not create
multiple sidechain triggers. Include arpeggios, ratchets, live playing and drum lanes
in tests. A kick-lane source is needed because all drum lanes share track 8.

Events are consumed before both render jobs are dispatched. Publish one envelope
snapshot for that block, and multiply the selected targets' existing gain ramps.
No render order dependency or audio buffer is necessary. Use sample-time envelope
advancement; optional tempo synchronization must use the existing transport clock.

Preserve legacy G_DUCK behavior when the new sidechain mode is disabled. When the
new mode is explicitly enabled, select its gain policy in place of legacy ducking
rather than accidentally multiplying two independent duck envelopes.

### Audio sidechain second

There is a mono scratch buffer per render core, not eight retained stereo track
buffers. This is sufficient for a source detector tap in `mix_part_run`; the drum
source needs a matching tap in `mix_drums`. Peak detection of one selected source
does not require preserving all track audio.

The minimum predictable design uses the previous completed block's source envelope
for the next block's target gain. Additional control delay is one block, approximately
0.726 ms; no added audio lookahead buffer. Each source renderer writes its own
detector slot, consumed only after the render-core join. Tap before sidechain gain
to prevent detector feedback, with explicit level/mute semantics.

Same-block audio response to a source rendered later would instead require retaining
target buffers or changing render scheduling. Do not promise it at the same resource
cost. Arbitrary reciprocal audio routing is outside the first implementation.

## Parameters, synchronization and persistence

- Keep new master/sidechain state separate from `track_t` and its NOINIT footprint.
- Prefer appended global descriptors; do not grow `P_COUNT` merely to expose master
  controls. That would also affect sound presets, locks and every track snapshot.
- Apply a validated parameter snapshot at a block boundary before dispatching core 1.
  Brief interrupt protection can publish a complete command/snapshot; `volatile`
  alone is not a multi-field atomic update. Do not copy or recalculate large data
  while audio interrupts are disabled.
- Existing FUN9 saves only 32 project globals. Appending enum values alone will not
  persist these controls. Define a versioned project extension/envelope and defaults
  for old projects; leave old FUN9 offsets and checksums interpretable by its importer.
- Account for the extension in four RAM snapshots, staging, autosave, journal,
  browser song files and backup preflight. The 7,936-byte journal payload has room
  for compact settings; RAM/cache expansion still needs a fresh link check.
- Keep firmware identity/version and release artifacts unchanged during this work,
  as requested. A future file-format or protocol capability change is a separate,
  explicit compatibility decision; do not make old readers silently accept new data.

## File plan and staged acceptance

| Stage | Concrete files | Acceptance |
| --- | --- | --- |
| Resource gate | `tools/build.py`, linked ELF/disassembly, identified code/data hotspots | Demonstrate enough flash without removing existing functions; recheck every memory region and stack margins. |
| Compressor | New `firmware/src/master_fx.c`, called from `fx.c`; new host DSP test | Below/above threshold, curve/knee, attack/release, stereo linking, impulses, silence, extreme valid input and exact disabled output. Compile on target before adding UI. |
| Event ducking | Small sidechain module, `fx.c`, `voice.c`, `drums.c`, narrowly scoped `seq.c` integration | One attack per accepted event; chosen targets only; drums/kick lane, live input, recording, arp/ratchets, clock and pattern changes. Both-core output parity. |
| Audio detector | Source taps in `fx.c:mix_part_run` / `mix_drums`, join-time publication | Detector independence from render split, documented one-block delay, source mute policy and no feedback. |
| Saturation/ceiling | `master_fx.c`, existing `fx.c:lim_block` / `knee` | Drive/compensation, parameter ramps, bounded sample peaks, no overflow, no additional limiter latency. No true-peak assertion. |
| Presets/UI | `params.c`, `core.h`, `ui_*.c`, `editor.c`, `editor_live.c`, `web/editor.html` | Readable pages, neutral Amount 0, smooth changes during playback/recording, optional gain-reduction display only if budget permits. |
| Persistence/preview | `project.c`, `project_legacy.c`, `pattern_bank.c` as needed, `firmware/host/preview.c`, `web/preview-worklet.js`, backup/song parsers | Power-cycle and torn-write tests, legacy defaults, old-reader rejection, native/WASM parity. |
| Integration | `tests/hold_rec_test.c`, `seven_tracks_test.c`, `mix_split_test.c`, `stress_test.c`, new combined scenarios, `run_tests.sh` | Seven synths + drums; MIDI live input and overdub with effects; unchanged undo/selection/transport rules. |

The resource gate should precede the brief's P1; postponing Live Play/Overdub until
the end is risky. Add a small combined playback/live/recording scenario with every
DSP stage, then run the full matrix after UI and persistence integration.

## Validation and remaining decisions

Existing logs from the previous feature build report the full host suite passing,
182 unchanged golden renders, seven-part ASan/UBSan and TSan checks, browser tests
and native/WASM parity. These establish the baseline only: **none tests the proposed
new effects**, and none is a physical-device performance result. No tests or builds
were rerun for this documentation-only audit.

New verification must include curve/envelope tolerances, bypass goldens, ceiling
bounds, denormal-free silence behavior where applicable, parameter abuse, recording
deduplication, both render splits, persistence failures and MIDI bursts. An external
MIDI note can be a trigger even when its internal track level is zero; testing an
external device's audible output is a separate physical routing scenario.

On hardware, record baseline/enabled average and worst DMA-half times, nested USB
cost, late halves, voice shedding, both-core waits and stack margins. Stress the
heaviest engines, all active inserts, live notes, overdub, pattern changes, MIDI
clock/I/O and USB recording, including its 48 kHz resampling mode. Measure actual
key/MIDI-to-output latency with loopback. The deadline is 5.805 ms per half including
nested work; a target of at most 80% worst-case occupancy leaves margin below the
existing 85% shedding threshold. This target is not a measured capability.

Decisions before implementation: accept the one-source/multiple-target model,
one-block delay for audio sidechain, initial two-shelf EQ scope, persistence scope
(pattern versus device), and the measured flash-recovery approach. If resources do
not allow the complete design, the minimum useful edition is the compressor,
event ducking, reused limiter/soft knee and shared presets, with audio detection/EQ
deferred explicitly. Phase 1 remains pending architecture approval as the brief requests.
