#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
"""Make the site (GitHub Pages):

  index.html                  redirect to the installer
  firmware/felucca-VER.fwsc   the package
  webapp/installer/index.html index_pkg.html with fm1pkg.js, fm1ota.js and the metadata inlined
  webapp/changes/index.html   piterfly vs SLOOP comparison (+ logo/screenshots)
  webapp/editor/index.html    editor.html (+ fukiai.ttf, FUKIAI-LICENSE.txt)
  webapp/editor/sloop-editor.html  the editor in one file, for offline use (SLOOP 2.6): its icon font inlined,
                              its links to the installer and the font licence online; opened from the disk in
                              Chrome / Edge (file:// is a secure context: Web MIDI asks for permission)
  webapp/sw.js, webapp/sloop.webmanifest, webapp/icon-*.png  the site without internet (SLOOP 2.6): an app to
                              install; opened once online, the installer, the editor and the firmware are kept
                              by the browser and work offline, still on https (on Android a downloaded file opens
                              as content://, where Chrome refuses Web MIDI)
  src/                        not touched

  web/make_site.py build/felucca-X.Y.fwsc X.Y OUT_DIR [--beta]
  (--beta: web/beta_banner.html at the top of the installer: the beta channel, docs/beta)

The package identity (FM-1_9xx) is read from the package; the device reports it
after the install.
"""
import base64
import hashlib
import html as html_escape
import json
import re
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
ONLINE = "https://piotrwozniak.github.io/fm1-piterfly/"  # Selected destination; generating files does not deploy them.


def offline_editor(html):
    """editor.html as one file for the disk: the icon font inlined, the download link out, the relative links online"""
    font = base64.b64encode((HERE / "fukiai.ttf").read_bytes()).decode()
    for a, b in (('url("fukiai.ttf")', 'url(data:font/ttf;base64,' + font + ')'),
                 ('href="../installer/"', 'href="' + ONLINE + 'webapp/installer/" target="_blank" rel="noopener"'),
                 ('href="FUKIAI-LICENSE.txt"', 'href="#font-licences"'),
                 ('href="../licences/"', 'href="#font-licences"')):
        if html.count(a) != 1:
            raise SystemExit(f"editor.html must contain {a} once; update make_site.py")
        html = html.replace(a, b)
    for pat, what in ((r"<!--DL-->.*?<!--/DL-->", "<!--DL-->...<!--/DL-->"),       # (the file has no site
                      (r"<!--PWA-->.*?<!--/PWA-->", "<!--PWA-->...<!--/PWA-->"),    # around it: no app to
                      (r"/\*PWA\*/.*?/\*/PWA\*/", "/*PWA*/.../*/PWA*/"),           # install, no copy kept)
                      (r'<button id="install"[^>]*></button>\s*', '<button id="install">')):
        html, n = re.subn(pat, "", html, flags=re.S)
        if n != 1:
            raise SystemExit(f"editor.html must contain {what} once; update make_site.py")
    return html


def licence_page(out, version, raw, release_dir=None):
    root = HERE.parent
    page = out / "webapp" / "licences"
    page.mkdir(parents=True, exist_ok=True)
    source = "<p>No matching source bundle is attached to this local preview.</p>"
    if release_dir is not None:
        release_dir = Path(release_dir)
        info = json.loads((release_dir / "BUILD_INFO.json").read_text())
        package = f"piterfly-{version}.fwsc"
        if info["version"] != version or info["artifacts"][package]["sha256"] != hashlib.sha256(raw).hexdigest():
            raise ValueError("source bundle does not match this firmware/version")
        dest = out / "source"
        dest.mkdir(parents=True, exist_ok=True)
        links = []
        for name in [f"piterfly-{version}-source.tar.gz", f"piterfly-{version}-licences.zip", "BUILD_INFO.json", "SHA256SUMS"]:
            data = (release_dir / name).read_bytes()
            if name in info["artifacts"] and hashlib.sha256(data).hexdigest() != info["artifacts"][name]["sha256"]:
                raise ValueError(f"release artifact hash mismatch: {name}")
            (dest / name).write_bytes(data)
            links.append(f'<li><a href="../../source/{html_escape.escape(name)}">{html_escape.escape(name)}</a></li>')
        source = (f'<p>Exact source tree SHA-256: <code>{info["source_tree_sha256"]}</code>.</p>'
                  f'<p>Original Git commit: <code>{info["base_commit"]}</code>. Distribution adaptations are identified by the complete source manifest.</p>'
                  '<ul>' + ''.join(links) + '</ul>')
    texts = [root / "LICENSE", root / "LICENSING.md", root / "THIRD_PARTY_NOTICES.md", root / "PROVENANCE.md",
             root / "docs/ICON-RIGHTS.md", root / "assets/icons-LICENSE.txt",
             root / "assets/piterfly/LICENSE.txt", *sorted((root / "LICENSES").glob("*.txt")),
             root / "assets/samples-cc0/ATTRIBUTION.txt", root / "assets/hiphop-pack/CREDITS.txt"]
    notices = ''.join(f'<details><summary>{html_escape.escape(p.relative_to(root).as_posix())}</summary><pre>{html_escape.escape(p.read_text())}</pre></details>' for p in texts)
    template = (HERE / "licences.html").read_text()
    (page / "index.html").write_text(template.replace("<!--VERSION-->", html_escape.escape(version))
                                    .replace("<!--SOURCE-->", source).replace("<!--LICENCES-->", notices))


ICONS = ("icon-192.png", "icon-512.png", "icon-maskable-512.png")
SCREENSHOTS = ('cursors.png', 'glo-64.png', 'home-64.png', 'layout-home-before.png', 'layout-options-before.png', 'piterfly-granular-sound.png', 'piterfly-granular-source.png', 'piterfly-home-after.png', 'piterfly-midi-channels.png', 'piterfly-midi-home.png', 'piterfly-midi-input.png', 'piterfly-midi-thru.png', 'piterfly-options-after.png', 'rec-ready-64.png')


def app_files(out, version, fw_name):
    """webapp/: the manifest, the icons and sw.js with the list of what it keeps (relative to webapp/) and a
    stamp of their contents, so any change makes a new sw.js and the browser takes the new site"""
    app = out / "webapp"
    for f in ICONS:
        shutil.copy(HERE / f, app / f)
    manifest = {
        "name": "piterfly", "short_name": "piterfly", "id": "editor/", "start_url": "editor/", "scope": "./",
        "display": "standalone", "background_color": "#000000", "theme_color": "#000000",
        "description": "piterfly for the M-VAVE FM-1: the editor and the installer, also without internet",
        "icons": [{"src": "icon-192.png", "sizes": "192x192", "type": "image/png", "purpose": "any"},
                  {"src": "icon-512.png", "sizes": "512x512", "type": "image/png", "purpose": "any"},
                  {"src": "icon-maskable-512.png", "sizes": "512x512", "type": "image/png", "purpose": "maskable"}],
    }
    (app / "sloop.webmanifest").write_text(json.dumps(manifest, indent=1) + "\n", encoding="utf-8")
    keep = {"installer/": app / "installer" / "index.html", "editor/": app / "editor" / "index.html",
            "../firmware/" + fw_name: out / "firmware" / fw_name, "sloop.webmanifest": app / "sloop.webmanifest"}
    for f in ("fukiai.ttf", "FUKIAI-LICENSE.txt"):
        if (app / "editor" / f).exists():
            keep["editor/" + f] = app / "editor" / f
    for f in ("preview.js", "preview-worklet.js", "sloop-preview.wasm"):
        if (app / "editor" / f).exists():
            keep["editor/" + f] = app / "editor" / f
    keep["changes/"] = app / "changes" / "index.html"
    for asset in ("changes.css", "midipipe.html"):
        keep["changes/" + asset] = app / "changes" / asset
    for f in (app / "changes").rglob("*.png"):
        keep[f.relative_to(app).as_posix()] = f
    for f in ICONS:
        keep[f] = app / f
    if (app / "licences/index.html").is_file():
        keep["licences/"] = app / "licences/index.html"
    for f in (app / "licences").rglob("*"):
        if f.is_file() and f != app / "licences/index.html":
            keep[f.relative_to(app).as_posix()] = f
    for f in (out / "source").glob("*"):
        if f.is_file():
            keep["../source/" + f.name] = f
    h = hashlib.sha256()
    for k in sorted(keep):
        h.update(k.encode() + b"\0" + keep[k].read_bytes())
    sw = (HERE / "sw.js").read_text(encoding="utf-8")
    for mark in ("/*STAMP*/", "/*FILES*/[]"):
        if sw.count(mark) != 1:
            raise SystemExit(f"sw.js must contain {mark} once; update make_site.py")
    sw = sw.replace("/*STAMP*/", f"{version}-{h.hexdigest()[:12]}").replace("/*FILES*/[]", json.dumps(sorted(keep)))
    (app / "sw.js").write_text(sw, encoding="utf-8")
    return sorted(keep)
BLOCKS, BLK, KEEP = 20, 0x30, 0x2F


def strip_module(src):
    src = re.sub(r"^export\s+", "", src, flags=re.M)
    return re.sub(r"^import .*?;\n", "", src, flags=re.M)


def product_of(raw):
    """the package identity: one marker byte after each of the first 20 blocks (fm1pkg.js productOf)"""
    return "".join(chr((m - i - 1) & 0xFF) for i in range(BLOCKS) if (m := raw[i * BLK + KEEP]) != 0x7D)


def main(pkg, version, out, release_dir=None):
    pkg, out = Path(pkg), Path(out)
    raw = pkg.read_bytes()
    product = product_of(raw)
    if not re.fullmatch(r"FM-1_9\d\d", product):
        raise SystemExit(f"{pkg}: identity {product!r} is not a Felucca package (FM-1_9xx)")
    if b"FELUCCA-LOADER-1" not in raw:              # marker of firmware/loader
        raise SystemExit(f"{pkg}: no Felucca update loader in it")
    html = (HERE / "index_pkg.html").read_text(encoding="utf-8")
    lib = strip_module((HERE / "fm1pkg.js").read_text(encoding="utf-8")) + "\n" + \
        strip_module((HERE / "fm1ota.js").read_text(encoding="utf-8"))
    name = f"sloop-{re.sub(r'[^A-Za-z0-9.-]', '-', version)}.fwsc"
    meta = json.dumps({"version": version, "product": product, "pkg": "../../firmware/" + name,
                       "sha256": hashlib.sha256(raw).hexdigest()})   # the page checks the download against it
    for mark in ("/*LIB*/", "/*META*/"):
        if html.count(mark) != 1:
            raise SystemExit(f"index_pkg.html must contain {mark} once; update make_site.py")
    html = html.replace("/*LIB*/", lib).replace("/*META*/", meta)
    logo = HERE.parent / "assets" / "piterfly" / "arcade-web.png"
    logo_uri = "data:image/png;base64," + base64.b64encode(logo.read_bytes()).decode()
    html = html.replace("<!--LOGO-->", f'<img src="{logo_uri}" width="446" height="304" alt="piterfly">')
    build_info = '<section id="build-info" style="border:2px solid #ffc618;padding:16px;margin:20px 0;color:#ffc618"><strong style="font-size:24px">piterfly · 24 PATTERNS</strong><br>6 banks A–F · 64 steps per track · 3 sample banks · 128 song entries<br><small>USR4 becomes pattern storage. Back up before installing.</small></section>' if "24-patterns" in version else ""
    html = html.replace("<!--BUILD_INFO-->", build_info)
    inst, ed, fw = out / "webapp" / "installer", out / "webapp" / "editor", out / "firmware"
    for d in (inst, ed, fw):
        d.mkdir(parents=True, exist_ok=True)
    guide = HERE.parent / "output/pdf/Studio-0.3-guide-rapide-FR.pdf"
    guide_link = ""
    if guide.exists() and "--studio-guide" in sys.argv:   # LIVE: that guide describes the removed RYTHME tab
        shutil.copy(guide, out / guide.name)
        guide_link = '<p lang="fr"><a href="../../' + guide.name + '">Guide rapide illustré (PDF, 4 pages)</a></p>'
    html = html.replace("<!--STUDIO_GUIDE-->", guide_link)
    banner = ""
    if "--beta" in sys.argv:                       # the beta channel (docs/beta): what it is, what it implies
        notes = HERE / "beta_banner.html"
        banner = notes.read_text(encoding="utf-8") if notes.exists() else "<p><b>BETA</b></p>"
    html = html.replace("<!--BANNER-->", banner)
    for old in list(fw.glob("felucca-*.fwsc")) + list(fw.glob("sloop-*.fwsc")):   # one package: the current one
        old.unlink()
    shutil.copy(pkg, fw / name)
    ed_html = (HERE / "editor.html").read_text(encoding="utf-8")
    dl = f'download="sloop-editor-{re.sub(r"[^A-Za-z0-9.-]", "-", version)}.html"'
    if ed_html.count('download="sloop-editor.html"') != 1 or html.count('download="sloop-editor.html"') != 1:
        raise SystemExit('editor.html and index_pkg.html must link download="sloop-editor.html" once; update make_site.py')
    (ed / "index.html").write_text(ed_html.replace('download="sloop-editor.html"', dl), encoding="utf-8")
    (ed / "sloop-editor.html").write_text(offline_editor(ed_html), encoding="utf-8")
    (inst / "index.html").write_text(html.replace('download="sloop-editor.html"', dl), encoding="utf-8")
    if "24-patterns" in version:
        direct = html.replace("<head>", '<head><base href="/webapp/installer/">', 1)
        (out / "install-24-patterns.html").write_text(direct.replace('download="sloop-editor.html"', dl), encoding="utf-8")
    for f in ("fukiai.ttf", "FUKIAI-LICENSE.txt"):
        if (HERE / f).exists():
            shutil.copy(HERE / f, ed / f)
    # Optional browser DSP, built separately without the hardware toolchain.
    for f in ("preview.js", "preview-worklet.js"):
        shutil.copy(HERE / f, ed / f)
    dsp = HERE.parent / "build" / "preview" / "sloop-preview.wasm"
    if release_dir is not None:
        release = Path(release_dir)
        info = json.loads((release / "BUILD_INFO.json").read_text())
        dsp = release / f"piterfly-{version}-preview.wasm"
        if dsp.exists() and hashlib.sha256(dsp.read_bytes()).hexdigest() != info["artifacts"][dsp.name]["sha256"]:
            raise ValueError("browser preview hash does not match this release")
    if dsp.exists():
        shutil.copy(dsp, ed / "sloop-preview.wasm")
    elif (ed / "sloop-preview.wasm").exists():
        (ed / "sloop-preview.wasm").unlink()  # never serve an old audio build as the current one
    changes = out / "webapp" / "changes"
    changes.mkdir(parents=True, exist_ok=True)
    shutil.copy(HERE / "changes.html", changes / "index.html")
    for asset in ("changes.css", "midipipe.html"):
        shutil.copy(HERE / asset, changes / asset)
    shutil.copy(logo, changes / "logo.png")
    screenshots = changes / "screenshots"
    screenshots.mkdir(exist_ok=True)
    for old in screenshots.glob("*.png"):
        if old.name not in SCREENSHOTS:
            old.unlink()  # an older local site must not retain excluded comparison images
    for shot in SCREENSHOTS:
        shutil.copy(HERE.parent / "assets/piterfly/screenshots" / shot, screenshots / shot)
    licence_page(out, version, raw, release_dir)
    app_files(out, version, name)
    (out / "index.html").write_text(
        '<!doctype html><meta charset="utf-8"><title>piterfly</title>'
        '<meta http-equiv="refresh" content="0; url=webapp/installer/">'
        '<a href="webapp/installer/">piterfly installer</a>\n', encoding="utf-8")
    print(f"site: {out}: webapp/installer ({len(html)} B), webapp/editor, firmware/{name} ({len(raw)} B, {product})")


if __name__ == "__main__":
    args = [a for a in sys.argv[1:] if a not in ("--studio-guide", "--beta")]
    release_dir = None
    if "--release-dir" in args:
        i = args.index("--release-dir")
        if i + 1 >= len(args):
            sys.exit("--release-dir needs the local prepared release directory")
        release_dir = args[i + 1]
        del args[i:i + 2]
    if len(args) != 3:
        sys.exit(__doc__)
    main(*args, release_dir=release_dir)
