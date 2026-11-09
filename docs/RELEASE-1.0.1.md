# Piterfly 1.0.1 — identity change, local device-test candidate

The running application and `.fwsc` now identify as **`FM-1_1120101`**.
The packaged Felucca loader identifies as **`ota-FM-1_1120101`**.
`firmware/src/version.h` defines both the software version and the product number;
`tools/build.py` reads the number instead of deriving `910` from major/minor.
Default and release builds use the same identity. The number remains independent
of the version label; rebuilding an older release requires its original tag.

## Hub availability

On 2026-11-09 the identity checker at
<https://thefirmwarehub.com/submit/#check> returned for `FM-1_1120101`:
**“Free ✓ No firmware on the hub uses it.”**
This is an availability check, not a reservation or submission. Recheck before
publishing. The old `FM-1_910` is shared by other firmware; 1.0.0 stays unchanged.

## Format and updater checks

- The `.fwsc` identity has up to 20 marker bytes. `FM-1_1120101` uses 12 and
  none encodes as the unused `0x7D` marker.
- The USB handshake provides 27 identity bytes. The application uses 12 and the
  `ota-` loader 16, so neither is truncated.
- Python and browser updaters parse a numeric suffix, not a three-digit version.
  They compare the model (`FM-1`) before updating and the full new identity after
  reboot. They impose no ascending numeric-version requirement.
- The browser generator accepts the numeric `.fwsc` identity and still requires
  the Felucca loader marker. Browser and CLI interrupted-update paths explicitly
  accept this Piterfly loader as well as legacy `ota-FM-1_9xx` loaders; other
  long-ID loaders and the stock loader remain rejected by the custom resume path.
- The Hub's documentation only promises interrupted-update resume for `9xx`
  Felucca IDs. Do not assume its resume path recognizes this new long ID: use the
  updated Piterfly installer/CLI to resume it.

The `ota-FM-1_015` value in the legacy boot parameter structure is unchanged.
It is part of the existing stage/boot protocol; it is not the identity answered
by the running application or custom loader.

## Updating from 1.0.0

The old app (`FM-1_910`) stages the loader from the **new package**, which writes
1.0.1 and reboots into `FM-1_1120101`. The new host installer supports both phases.
There is no change to flash layout, the 24-pattern bank, sample allocation,
FUN7 projects, PER4 settings or protocol 15. No identity-based data migration is
performed. As always, make a backup before the hardware test.

Host tests cover the old-to-new identity transition, resumed new loader, reverse
transition, app/loader USB replies and writing the real new package over a 1.0.0
flash image. These checks do not replace installation and power-loss tests on FM-1.
Verify on hardware: installation from 1.0.0, the USB handshake, saved patterns and
settings after power-off, and installing another firmware again.

The published tag `1.0.0` and all its release assets are retained. No 1.0.1 tag,
GitHub Release, Pages deployment or Hub submission is created for this local test.

## Local build and test installer

Use Python with `tools/requirements-build.txt`, the pinned toolchain and
`AC79_SDK=$PWD/vendor/ac79`. After source review:

```sh
python tools/prepare_release.py --seal-sources
python tools/build_preview.py  # emcc or clang/wasm-ld on PATH
python tools/prepare_release.py --out build/releases/piterfly-1.0.1
python web/make_site.py build/releases/piterfly-1.0.1/piterfly-1.0.1.fwsc 1.0.1 build/site-1.0.1 --release-dir build/releases/piterfly-1.0.1
python -m http.server 8767 --directory build/site-1.0.1
```

Open <http://localhost:8767/webapp/installer/> in Chrome/Edge when ready to test.
This local installer serves the new package; the existing public website is not
changed by preparing it. Keep the matching source archive, licences, BUILD_INFO
and SHA256SUMS with the firmware file.
