# piterfly: feature and technical notes

This checkout extends **SLOOP 2.8.5** on branch `codex/piterfly-24-patterns`. For the complete user-facing comparison and controls in Polish, see [PITERFLY.md](PITERFLY.md). This is a feature build, not an upstream SLOOP release.

## Changes from base SLOOP

- **24 persistent patterns** instead of four: A1–F4, six banks of four. Each pattern still holds all four tracks, sounds, locks, nudges and fill conditions.
- **128 song-order entries** instead of 16, each 1–64 bars. The quick live chain remains limited to eight entries.
- **Three user sample banks** instead of four. USR4's 80 KiB is repurposed for pattern storage.
- Full step grids, outlined playback positions and a separate editing cursor; compact themed page-navigation dots.
- Per-track MIDI THRU settings, piterfly arcade branding and USB device names.
- Fresh overwrite confirmations, explicit pending-save status, retry of failed settings writes and song-import preflight checks.

The maximum **64 steps per track is inherited from SLOOP**, not a new piterfly feature.

## SAVE controls

Hold SAVE and turn **KNOB 1**, the first parameter knob, to select bank A–F. White keys are numbered 1–16 from F3 on the left.

| White keys with SAVE held | Action |
| --- | --- |
| 1–4 | Select one of the bank's four patterns |
| 5–8 | Store the working loop; green floppy-disk tiles |
| 9–12 | Delete the corresponding pattern; red trash-bin tiles |
| 13 | LOOP / SONG mode, unless SONG REC is active |
| 14 | Arm or finish SONG REC |
| 15 | Unassigned |
| 16, CHAIN tile | Open the SONG order editor |

Selection takes effect on the next bar during loop playback, immediately when stopped. Running SONG playback blocks manual pattern selection. A used slot requires a second store press within three seconds. Releasing SAVE, changing bank or choosing another operation cancels overwrite confirmation.

Delete requires stopped transport and a second press within three seconds while SAVE stays held. It keeps the working loop, removes song references and has **no Undo**. Bank changes and SAVE release cancel delete confirmation.

Only white keys **1, 5, 9 and 13** light as group landmarks. Screen colours distinguish save and delete; the FM-1 driver supports LED brightness, not RGB colour selection.

Short SAVE on HOME or DRUMS opens SONG; other parameter pages may open the SAVE family. SAVE on the SONG page stores its order. SAVE → SECTION remains available with pattern selection, ERASE, LOAD and SAVE.

## SONG and live storage

SONG's four knobs are ENTRY, SECTION, BARS and LENGTH. LENGTH now reaches 128. Repeated patterns are allowed. SONG REC records pattern selections and their bar durations, not audio or new notes. It starts on a bar boundary once playback has a selected stored pattern. STOP or SAVE + white key 14 finishes it.

The quick chain is separate: during loop playback, hold SAVE, tap at least two selection keys, then release SAVE. It loops up to eight entries.

Four editable live snapshots may await flash saving. A fifth distinct pending store says **STOP TO SAVE MORE**, protecting earlier edits. All 24 already-stored patterns remain selectable while playing.

**PENDING is not a durable save.** HOME and the web editor report it until flash is written. Stop transport, release notes and allow idle time: writes wait for quiet audio and inactivity, with retry throttling. **PATTERNS SAVED** confirms completion. Pending RAM changes can be lost on power-off; patterns committed to flash survive it. Failed settings writes retain their dirty state and retry rather than silently discarding the pending order.

## Screens and navigation

HOME shows every track in rows of 16 real steps: 32 steps use two rows and 64 use four. Parameter-page footers show all steps of the selected track. REC READY and free-take summaries use the same real-step grouping.

The playback position has a full white outline, keeping the note colour inside. Parameter-page footers mark the editing cursor with yellow corners, white against a yellow fill; both indicators may coexist. Physical readability of the small 64-step cells remains unverified.

HOME outlines the selected track in its theme track colour. Recording turns the target’s frame red and shows REC; when recording ends, the selected track returns to its own colour. There is no blinking frame or ARM tag. Stopped recording readiness keeps the existing REC READY screen; recording-target routing is unchanged.

SEQ's held step layer and the DRUMS editor intentionally keep 16 editable steps per page, mapped to the 16 white keys. OCT− / OCT+ changes pages; this is paging, not folding multiple steps into one cell.

Parameter-page headers have compact palette-coloured dots, with the theme accent on the current page. One-page sections have no dots. Temporary messages and MIDI LEARN take priority. The track icon and number are separated. HOME's bars on the right are configured track levels, not live audio meters.

## Storage and migration

Back up before installing: **USR4 is overwritten**. USR1–USR3, factory samples, synthesised kits and SYN1–SYN4 remain. USR4 and the combined USR3+4 kit are unavailable as complete kits; old kit IDs remain REMOVED so SYN references do not shift. An old combined kit can still play its USR3 half.

The original 32 KiB of pattern storage plus USR4 provides 28 sectors of 4 KiB. Up to 24 sectors hold current patterns; four are spare. A save writes a free sector, verifies the payload and commits a checksummed header last. The previous record is protected during replacement. Deletion writes a tombstone; boot selects the newest valid record. This does not provide two permanent lifetime copies of every pattern or guarantee protection against flash bit rot.

Old A–D migrate to A1–A4. Legacy sectors stay protected until committed journal replacements exist; migration resumes after interruption. Stored patterns are read through flash XIP, with four editable snapshots in RAM.

PER3 settings migrate to **PER4**, retaining the panel/calibration, order, lights, SYN kits and MIDI routing. There is no automatic reverse migration to older firmware: retain an upstream-format backup for downgrade.

## Web editor, protocol and file import

Use this build's matching editor. Protocol **14** retains protocol 13's pattern-count INFO extension and direct selection/storage operations. PROJECT addresses slots 0–23. LIVE ops 13/14 select/store a pattern; its state includes the 24-bit ready mask and pending-write flag. The 7-bit song-length field encodes length 128 as zero.

Backup objects 2–5 remain A1–A4; 64–83 are B1–F4. USR4 object 35 is no longer advertised. Settings PER4 use a 260-byte order at offset 48, lights at offset 308, and a 312-byte header before SYN/MIDI data. Project payloads retain FUN6. See [EDITOR_PROTOCOL.md](../web/EDITOR_PROTOCOL.md).

Song import replaces **every target pattern slot and the working loop**, including empty slots. An old four-pattern song fills A1–A4 and clears the remaining slots. It is not a merge into unused slots.

Before writing a pattern, import checks order fields, duplicate object IDs, lengths, file CRCs, project envelopes/internal FNV checksums, device capacity and readable target settings. Invalid input is refused before modification. Multi-object import is still not one atomic transaction against unplugging or power loss: keep a backup.

The editor refuses extended files on older firmware, nonempty USR4 restoration into this build, and PER4 restoration onto older firmware. Help and import confirmations reflect the connected device's actual pattern/sample capacity.

## Software validation and limits

| Test | Coverage |
| --- | --- |
| [pattern_bank_test.c](../tests/pattern_bank_test.c) | Full capacity, legacy-sector protection, interrupted programming, journal rotation, tombstones, unchanged USR1–USR3 |
| [pattern24_test.c](../tests/pattern24_test.c) | Full-length patterns, save/load/play, SysEx F4 access, backup mapping, deletion, cold cache, bounded live snapshots, disabled USR4 |
| [pattern24_migration_test.c](../tests/pattern24_migration_test.c) | Old A–D, occupied USR4, warm/cold restarts, PER3 → PER4, saved 128-entry order |
| [arranger_test.c](../tests/arranger_test.c) | 128-entry playback, stop/loop, repeated patterns and fractional timing |
| [save_safety_test.c](../tests/save_safety_test.c) | Injected flash failure/retry, fresh overwrite confirmation, pending/durable status and LIVE flag |
| [ui_pages_test.c](../tests/ui_pages_test.c) | Rendering bounds, step 64, distinct edit/playback positions, pending badge, page dots, 20,000-frame random-input fuzz |
| [test_web.mjs](../web/test_web.mjs) | Protocol/editor tests, invalid imports with zero writes, legacy compatibility, package/updater simulation |
| [midi_thru_test.c](../tests/midi_thru_test.c), [midi_thru_settings_test.c](../tests/midi_thru_settings_test.c) | Routing, channel/track changes, note release, USB/TRS and settings retention |

Builds and `.fwsc` package validation pass within the firmware's memory limits. Full web tests and focused tests of final changes have passed; storage safety tests also ran with AddressSanitizer/UndefinedBehaviorSanitizer.

**Not yet validated on a physical FM-1:** this version's installation, screen readability, hardware timing and real power-cut behaviour. Simulated interruption tests are not physical power-cut tests.

The original SLOOP/Felucca authorship and GPL-3.0 licence remain in effect. The old [GUIDE.md](../GUIDE.md) and [SLOOP.md](../SLOOP.md) are retained as upstream references; these feature notes override their changed limits and controls.
