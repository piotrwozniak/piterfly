# piterfly licensing and provenance

piterfly **1.0.0** is modified SLOOP 2.8.5, based on Felucca by Leo Kuroshita
(@kurogedelic), Hügelton Instruments. Its code is **GPL-3.0-only**, except for the
components below. Original notices and authorship are retained.

**This is a sanitized distribution tree.** Active device icons are GPL-3.0-only,
generated from original piterfly geometry in `tools/gen_pixel_icons.py`. The
inherited atlas, historical/proposal screenshots and SLOOP artwork whose terms
were not established are excluded, together with the inherited Git history.
Replacement does not relicense those omitted materials. See [icon rights](docs/ICON-RIGHTS.md),
[release preparation](docs/RELEASE-1.0.0.md), [notices](THIRD_PARTY_NOTICES.md)
and [export provenance](PROVENANCE.md).

## Code and complete source

Firmware, HAL, loader, own engine wrappers, tools, web code and tests retain
GPL-3.0-only, except as listed below. Full text: [LICENSE](LICENSE). Binary
recipients must have the complete corresponding, modifiable source under its
applicable licences. A patch or link to unmodified Felucca/SLOOP is not the
source of the modified piterfly binary. Preserve notices and identify changes.

The local preparation tool builds from a frozen source snapshot and records
file hashes, original Git commit, export adaptations, build inputs and output
hashes. Its source archive accompanies the local firmware. Release preparation verifies the sealed filesystem inventory without creating a Git repository.
The source manifest identifies the original main commit and the exact exported source hashes.

## Components and assets

| Component | Licence/status | Evidence and notices |
| --- | --- | --- |
| Application, HAL, loader and own DSP/wrappers | GPL-3.0-only | Original file headers; LICENSE |
| FM6 core, firmware/src/fm6_core.c | Apache-2.0 | Google 2012, Pascal Gauthier 2016–2025; Leo's C port 2026; LICENSES/Apache-2.0-msfa.txt |
| PHYS DaisySP port, phys_dsp.c | MIT | Electrosmith / Emilie Gillet; Leo's fixed-point port; LICENSES/MIT-DaisySP.txt and file notice |
| PHYS sympathetic strings, phys_symp.c | MIT | Rings, Emilie Gillet 2015; Leo's port; LICENSES/MIT-Rings.txt |
| assets/samples-cc0/ | CC0-1.0 | VSCO-2 CE, VCSL, specified Sonic Pi samples; per-file ATTRIBUTION.txt; LICENSES/CC0-1.0.txt |
| Optional hip-hop pack, except EPIANO | CC0-1.0 | Versilian/Sonic Pi; pack CREDITS identifies sources |
| Optional hip-hop EPIANO, four WAVs | CC-BY-3.0 | Greg Sullivan Wurlitzer EP200 v1.1, mapped by kinwie; retuned, cut and processed by SLOOP; preserve credit, licence URI and adaptation information; LICENSES/CC-BY-3.0.txt |
| tools/gen_waves.py AND every sound it generates | CC0-1.0, explicit author dedication | Felucca commit 5738617968a22b5d4c0d3470cfcc95559052f446 covers both script and outputs; audited algorithm matches |
| Terminus BDF and converted bitmap/TTF font data | SIL OFL 1.1 | Dimitar Toshkov Zhekov 2020, Terminus 4.48; LICENSES/OFL-1.1-Terminus.txt; conversions stay OFL |
| Web Fukiai icon font | MIT | Hügelton 2026 notice and upstream Leo Kuroshita 2024 notice retained in LICENSES/MIT-Fukiai.txt |
| Current assets/icons.png device atlas | GPL-3.0-only | Original 86-icon piterfly set; editable generator tools/gen_pixel_icons.py; assets/icons-LICENSE.txt |
| Older Felucca atlas and historical screenshots | Excluded from this tree and its history; permission not established | Original reservation retained; docs/ICON-RIGHTS.md |
| Inherited SLOOP graphics and pre-replacement proposals | Excluded from this export | PUBLIC_EXPORT.json records exact paths and hashes; authorship remains credited |
| piterfly artwork and derived branding | GPL-3.0-only | User owns the artwork and selected GPL on 2026-11-08; assets/piterfly/LICENSE.txt |
| Three embedded JieLi SDK files | Apache-2.0 | Official tag AC79NN_SDK_V1.2.1_2023-12-13, verified hashes; LICENSES/Apache-2.0-JieLi.txt |

The historically mentioned docs/panel.jpg is absent from this checkout. All 16
sampled drum roles in the audited default build use the supplied CC0 recordings.
gen_waves.py also provides fallbacks and the optional SLICE demo. No external
Roland 808/909 WAV pack was added relative to the SLOOP base. Procedural drum
parameter banks do not imply permission to redistribute commercial recordings.

Factory FM6 patches are the project's own according to inherited notices.
User-imported samples/SysEx are not automatically part of this distribution.
CrispyZebra retains its historical GPL credit for PHASE. klattsch (MIT) is credited
as a VOICE design reference, with no copied library according to the source notice.

## Original GPL section 7 permission

The original notice and conditional additional permission are preserved verbatim
in LICENSES/Felucca-legacy-notice.txt. Permission to combine is conditional on
following separate asset terms; it does not itself supply the missing old-atlas
licence. The later CC0 dedication resolves the generator/sounds case only.
This fork does not extend or rewrite the original authors' grant.

## Tools and names

The JieLi toolchain, Docker, Python/Pillow and optional regeneration libraries
are build tools, not embedded runtime libraries. The linker uses this project's
own objects without vendor libc or SDK .a files. The vendor toolchain bundle's
redistribution licence was not established and that bundle is not repackaged.
The three SDK files actually embedded in .fwsc have verified Apache notices.
Bundling Python/Node or a native installer would require their runtime notices.

Felucca, Hügelton and SLOOP identify their authors/projects. M-VAVE/FM-1 identify
compatible hardware and belong to their owners. piterfly is independent, without
endorsement. Copyright licences do not automatically grant trademark rights.
Code contributions are GPL-3.0-only unless expressly documented otherwise.
Retain earlier notices and describe modifications and dates. No statement here
changes rights in privately imported samples, presets or the user's music.
