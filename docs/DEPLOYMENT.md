# piterfly Pages — prepared, not deployed

Target: **https://piotrwozniak.github.io/piterfly/**, from the private
[piotrwozniak/piterfly](https://github.com/piotrwozniak/piterfly) repository.
The installer is at `webapp/installer/`, the editor at `webapp/editor/`,
and the comparison/documentation at `webapp/changes/`, under `/piterfly/`.
Upstream SLOOP and Felucca links remain references to their original projects.

## Manual deployment workflow

`.github/workflows/pages.yml` has **only `workflow_dispatch`**. Pushing `main`,
creating a tag or opening a pull request does not run it. It accepts only `main`.
It does not create a GitHub Release, change visibility or automatically enable Pages.

When explicitly started, the workflow:

1. Checks out the cleaned sources with read-only contents access and no persisted credentials.
2. Downloads the vendor JieLi toolchain and verifies its four executable hashes
   against `tools/jieli-toolchain.sha256`. Uses the three pinned SDK files and
   licences already in `vendor/ac79`. A changed vendor download fails verification.
3. Verifies the sealed inventory, export policy and site generator; builds browser
   audio and tests it against native audio. Builds firmware and its corresponding
   source/licence archives from the same sealed snapshot.
4. Generates `build/site/` and checks `/piterfly/` links, firmware/source hashes,
   downloads, manifest and service-worker files with `tools/check_pages_site.py`.
5. Uploads **only `build/site/`** as the Pages artifact. The deploy job alone has
   `pages: write` and `id-token: write`, and deploys through the `github-pages` environment.

The generated site includes the installer, editor, permitted documentation images,
firmware, corresponding source, full licences and checksums. `source/` contains the
complete checksum-covered release artifacts, including browser audio and build metadata.
There are no loose Git objects, checkout files, local dependencies, logs, toolchain
files or caches in the uploaded directory. Preferred sources appear only in the
intentional, matching source archive. Downloads remain accessible without private
repository access when the site is eventually published.

Service-worker URLs and manifest scope are relative to `/piterfly/webapp/`;
firmware/source downloads stay under `/piterfly/`. Cache names include the worker's
path so updates do not delete caches of other projects on the same github.io origin.

## Next step — requires a separate publishing decision

Nothing is deployed by committing this configuration. When publication is authorized:

1. Confirm that the account plan permits Pages for this private repository; do not
   make it public as a workaround without a separate instruction. A Pages site may
   expose the generated files publicly even while its repository stays private.
2. In this repository's **Settings → Pages**, choose **GitHub Actions** as the source.
   Optionally require approval for the `github-pages` environment.
3. Go to **Actions → Deploy piterfly Pages (manual) → Run workflow**, select `main`
   and run it. The configured Pages base must remain `/piterfly/` (no custom domain).
4. After successful deployment, check installer/editor and firmware/source/licence
   downloads at the target URL. No GitHub Release is needed for the Pages site.

The older local bundles under `build/releases/piterfly-1.0.0-validated/` remain
unchanged for a later Release. The workflow builds a fresh matching bundle; it does
not upload those ignored local files or use the old project's checkout/history.

Official references:
- [Custom Pages workflows](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages)
- [Pages availability and visibility](https://docs.github.com/en/pages/getting-started-with-github-pages/what-is-github-pages)
