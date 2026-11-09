# Piterfly 1.0.1 publication record

Canonical firmware SHA-256:
`d5e3309ee41145d2eae6b4dde07c93ea59f5c69953f850505f39ca74f16c3dd5`.
Product: `FM-1_1120101`; loader: `ota-FM-1_1120101`.

The release tag `1.0.1` is pinned to `90d96dc7c123135dc3b49b59a62c2a617e753d7e`.
All 381 preferred-source inputs in the matching source archive were compared with
that commit (accounting for Git's declared Windows line endings). Checksums of all
frozen artifacts were verified. No firmware was rebuilt for publication.

The original build report describes the state at compilation. On 2026-11-09,
the maintainer confirmed that Piterfly 1.0.1 was tested on M-VAVE FM-1.
The release notes state: **Tested on M-VAVE FM-1.**

Completed software checks include the real 1.0.0 OTA implementation staging the
new loader, simulated NOR writes in both directions, app/loader USB replies,
browser/CLI upgrade and resume, refusal of foreign loaders, pattern migration,
audio parity, package/source matching and checksums. The maintainer has also confirmed the FM-1 test.

`fmhub.json` and Pages deployment configuration are publication metadata added
on top of the frozen source commit. The Hub reads card metadata from default
branch `main`. The Pages workflow downloads published release assets without
recompiling, so both download paths keep the canonical firmware hash.

The 1.0.0 tag and all existing release assets are preserved. There is no claim
of Hub acceptance until the maintainer has reviewed the honest submission and
a public listing can be verified.
