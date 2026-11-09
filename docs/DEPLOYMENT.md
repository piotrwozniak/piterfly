# Publishing the exact Piterfly release on Pages

Site: **https://piotrwozniak.github.io/piterfly/**.
Repository: <https://github.com/piotrwozniak/piterfly> (public).
Installer: `webapp/installer/`; editor: `webapp/editor/`; guide: `webapp/changes/`.

The manual `Deploy piterfly Pages (manual)` workflow takes a `release_tag`,
currently `1.0.1`. Publish that stable GitHub Release first, with all artifacts
covered by its `SHA256SUMS`. Then run the workflow from `main`.

It downloads the release, verifies every checksum and generates the site from
those exact firmware/source/licence/WASM files. It **does not recompile firmware**:
compiler/date differences must not change the binary served by the installer.
`tools/check_pages_site.py` checks the firmware/source pairing, hashes, links,
manifest and service-worker assets under `/piterfly/`. Only `build/site/` is uploaded.
The deploy job alone has Pages/OIDC write permissions. No push or tag runs deployment.

In Settings → Pages, keep GitHub Actions as the source. The `github-pages`
environment can require owner approval. Run Actions → Deploy piterfly Pages
(manual) → Run workflow, branch `main`, release_tag `1.0.1`.
After deployment, compare the public installer package SHA-256 with the Release
and the exact device-test package. No custom domain is configured.

Service-worker cache names include their project path. Generated pages, permitted
images and checksum-covered source/firmware/licence downloads are the only public
artifacts. Git metadata, local caches, toolchains and build logs are not uploaded.
Upstream SLOOP/Felucca links and all required notices remain.

Local frozen bundles stay under `build/releases/`. Preparation does not upload
or install them. For 1.0.1, the binary and its archived compiler inputs come from
`90d96dc`; later Hub/Pages publication metadata does not alter those inputs.

References:
- [Pages workflows](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages)
- [1.0.1 publication record](PUBLICATION-1.0.1.md)
