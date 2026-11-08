# Selected site destination — not deployed

The owner selected **https://piotrwozniak.github.io/fm1-piterfly/** as the public
site destination. The installer is under `webapp/installer/`, the comparison
under `webapp/changes/`, and the editor under `webapp/editor/` relative to that
base. `web/make_site.py` uses this base for links in the downloaded offline editor.
All generated site assets, service-worker scope and firmware/source bundle links
remain relative so the `/fm1-piterfly/` prefix is retained.

No GitHub repository, visibility, Pages setting, branch or remote was changed.
No files were uploaded and the destination is not claimed to be live.

## Hosting while preserving the private source repository

The existing `piotrwozniak/fm1-piterfly` repository stays private and unchanged.
A second repository with the identical owner/name cannot be created alongside it.
There are two deployment routes to assess before any publishing action:

1. Publish the sanitized site under `fm1-piterfly/` inside an account site hosted
   from a separate `piotrwozniak.github.io` repository. This produces the selected
   URL without changing the original private repository or exposing its history.
   Inspect any existing account site first; do not overwrite unrelated content.
2. Use GitHub Pages on the existing private project only if the account plan
   supports it and the owner separately authorizes changes there. GitHub Pro,
   Team and Enterprise support Pages from private repositories. That is not
   authorization to upload inherited history or unresolved assets to a public site.

Official references:
- https://docs.github.com/en/pages/getting-started-with-github-pages/what-is-github-pages
- https://docs.github.com/en/pages/getting-started-with-github-pages/creating-a-github-pages-site

When authorized, deploy only the generated `build/site/` with the matching clean
source archive, notices and checksums. Use the cleaned source tree, not the old
checked-in `docs/webapp` or `docs/firmware` from the private project. The browser-audio WASM must be rebuilt from the sealed sources and included
with its build metadata before hosting the audio preview. This local preparation
performs that build rather than copying the inherited binary. Source and licence downloads must remain public alongside
the binary, even if the development repository remains private.
