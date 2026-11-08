# piterfly 1.0.0 — distribution-tree provenance

Distribution candidate prepared locally on 2026-11-08. No firmware Release has been published or installed on hardware.

This tree derives from `piotrwozniak/fm1-piterfly`, private source commit
`8d9ac74c4a8b7cac356a850755714dffe0149a87`. That project modifies SLOOP 2.8.5,
upstream commit `bac9c05ea7627449405aa10e48da21d75b6c8c56` from
<https://github.com/isod89/sloop-fm1>, based on Felucca by Leo Kuroshita
(@kurogedelic), Hügelton Instruments. The initial export was a plain source directory, without a Git repository or copied .git.
No inherited Git objects, branches, remotes, tags or reflogs are included. This does
not change authorship or erase the origin of the code.

`PUBLIC_EXPORT.json` records every excluded path and hash, the original hashes
and executable bits of retained files, and the exact private origin commit.
It contains metadata, not the excluded artwork. The finished release archive's
`SOURCE_MANIFEST.json` identifies the final build inputs after the changes below.
SOURCE_INPUTS.json records the reviewed source inventory independently of Git.

## Distribution-only changes

- Exclude 46 historical artwork/proposal files: inherited SLOOP logos, upstream
  screenshots, pre-replacement granular proposals and two historical comparisons.
- Exclude 15 generated upstream site/firmware files. Rebuild the site from `web/`
  and firmware from this tree. Do not reuse the old precompiled browser preview.
- Keep the dated audit and its raw captures private. Current licence conclusions
  and the primary evidence links below replace references to those captures.
- Replace the two documentation comparisons with schematic legacy layouts rendered
  by `tools/render_ui_docs.c` using only the current GPL icon atlas. They illustrate
  step visibility, not an authentic historical device screenshot. Captions in
  English, Polish and Spanish identify this distinction.
- Remove links to excluded artwork without removing feature explanations or credits.
- Seal preferred source paths, hashes and executable bits in SOURCE_INPUTS.json,
  independently of Git. Reject changed/unlisted sources, nested Git metadata, excluded paths and
  known historical image hashes. Copy only listed documentation images.
- Update the distribution notices to distinguish this clean tree from the private
  historical repository. No licence for excluded material is inferred or granted.

The firmware, HAL, loader, DSP algorithms, sounds and their build generators are
unchanged from the origin commit. The GPL SLOOP logo-generation code remains in
`tools/gen_logo.py`; the default build uses only its piterfly splash compiler and
`assets/piterfly/arcade-source.png`. Old artwork outputs are not shipped.

## Evidence and rights retained

- New atlas: commit `27cc86fe8f8bb43a35a6a987d70029515748e743` in the private
  project; `tools/gen_pixel_icons.py`, `assets/icons-LICENSE.txt`, and
  `tests/pixel_icons_test.py` provide editable source and reproducibility checks.
- piterfly artwork: owner declaration and GPL-3.0-only choice of 2026-11-08 in
  `assets/piterfly/LICENSE.txt`; `arcade-source.png` is retained.
- Historical reservation: `LICENSES/Felucca-legacy-notice.txt`, unchanged. It is
  preserved as an original notice, not applied to the replacement icon atlas.
- Old atlas removal: <https://github.com/hugelton/Felucca/commit/727f272015da26eb2d0291bd652eba28ff57cb37>.
- Explicit CC0 dedication for the wave generator AND generated sounds:
  <https://github.com/hugelton/Felucca/commit/5738617968a22b5d4c0d3470cfcc95559052f446>.
- Sample sources: `assets/samples-cc0/ATTRIBUTION.txt`, both sample-pack CREDITS
  files, `THIRD_PARTY_NOTICES.md` and the full licence texts in `LICENSES/`.
- The optional EPIANO files remain CC-BY-3.0, with the Greg Sullivan / kinwie
  attribution and adaptation information. They are not relabelled CC0 or GPL.
- Three Apache-2.0 JieLi SDK package inputs are included in release sources with
  the pinned tag, hashes and licence. The proprietary-toolchain distribution
  status remains unestablished; the toolchain itself is not bundled.

## Verification boundary

A local clean build, archive inventory and host tests establish technical checks,
not hardware timing, power-cut recovery or a new grant from any third-party author.
The selected public site base URL is `https://piotrwozniak.github.io/piterfly/`.
The offline editor points to that destination. Selecting this URL does not enable
GitHub Pages, upload files or change repository visibility. Deployment still
requires a separate instruction; see docs/DEPLOYMENT.md. The original private
repository is not modified.

## New private source repository

On 2026-11-09 the owner requested a new private repository at
<https://github.com/piotrwozniak/piterfly>, initialized on `main` with one fresh
root commit. No inherited Git objects or history are imported. `PUBLIC_EXPORT.json`
records the earlier export operation, before this new repository was initialized.
The browser-preview correction is included (also recorded as `dec83e9` in the
original private project); all original authorship and licence notices remain.

The inventory tool excludes the new root `.git` directory from source archives,
and still rejects nested Git metadata and symlinks. `build/`, caches, virtual
environments and local dependency directories are ignored by Git.
Existing validated firmware/source/licence bundles and checksums stay unchanged
under `build/releases/piterfly-1.0.0-validated/` for a later Release; they predate
the repository housekeeping changes and are not committed. Re-seal the current
sources and rebuild browser audio before preparing any future release bundle.
No Release, GitHub Pages deployment or public visibility is enabled by this step.
