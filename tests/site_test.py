#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""SLOOP 2.6: the offline editor (web/make_site.py offline_editor): one file that needs nothing next to it; and the
site without internet (webapp/sw.js, the manifest): what sw.js keeps is all there and all it needs."""
import json
import hashlib
from html.parser import HTMLParser
import html
import re
import shutil
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "web"))
import make_site  # noqa: E402
sys.path.insert(0, str(ROOT / "tools"))
import check_pages_site  # noqa: E402

src = (ROOT / "web" / "editor.html").read_text(encoding="utf-8")
off = make_site.offline_editor(src)
bad = 0


def check(what, ok):
    global bad
    print(f"site: {what:<70} {'ok' if ok else 'FAIL'}")
    bad += not ok


# Every prose fragment, image description and navigation label needs both translations.
class ChangesText(HTMLParser):
    def __init__(self):
        super().__init__()
        self.skip = False
        self.words = set()

    def handle_starttag(self, tag, attrs):
        self.words.update(v for k, v in attrs if k in ("alt", "aria-label") and v)
        if tag in ("script", "style", "select"):
            self.skip = True

    def handle_endtag(self, tag):
        if tag in ("script", "style", "select"):
            self.skip = False

    def handle_data(self, text):
        if text.strip() and not self.skip:
            self.words.add(text.strip())


changes_html = (ROOT / "web/changes.html").read_text()
copy = ChangesText()
copy.feed(changes_html)
translations = json.loads(re.search(r"const TRANSLATIONS = (.*?);\nconst LANGUAGES", changes_html, re.S).group(1))
for language in ("pl", "es"):
    missing = [word for word in copy.words if not translations[language].get(word)]
    check(f"changes: {language} covers all text, captions, alt text and labels ({len(copy.words)})", not missing)
    if missing:
        print("missing translations:", missing)


check("the editor links the offline file (download)", src.count('download="sloop-editor.html"') == 1)
check("the installer page links it too",
      (ROOT / "web" / "index_pkg.html").read_text(encoding="utf-8").count('href="../editor/sloop-editor.html"') == 1)
check("offline: the icon font inlined", 'url("fukiai.ttf")' not in off and "font/ttf;base64," in off)
check("offline: no download link to itself", "<!--DL-->" not in off and 'id="offline"' not in off)
check("offline: the installer and the font licence online",
      'href="../installer/"' not in off and make_site.ONLINE + "webapp/installer/" in off
      and 'href="FUKIAI-LICENSE.txt"' not in off)
rel = [m for m in re.findall(r'(?:src|href)="([^"#]+)"', off) if not re.match(r"(https?:|data:|mailto:)", m)]
check("offline: no other file next to it (no relative src / href)", not rel)
check("offline editor targets the selected piterfly Pages installer",
      'href="https://piotrwozniak.github.io/piterfly/webapp/installer/"' in off)
check("offline: no url() to a file", not re.search(r'url\((?!data:)["\']?[A-Za-z]', off))
check("offline: no app, no service worker (a file has no site)",
      "serviceWorker" not in off and "webmanifest" not in off and 'id="install"' not in off)
for name in ("LICENSE", "LICENSES/OFL-1.1-Terminus.txt", "LICENSES/MIT-Fukiai.txt"):
    check(f"offline: full {name} accompanies embedded software/fonts",
          html.escape((ROOT / name).read_text()) in off)

# the site without internet: a site made from any package (a fake one with the identity make_site checks)
tmp = Path(tempfile.mkdtemp())
try:
    pkg = bytearray(4096)
    for i, ch in enumerate("FM-1_900"):
        pkg[i * make_site.BLK + make_site.KEEP] = (ord(ch) + i + 1) & 0xFF
    for i in range(8, make_site.BLOCKS):
        pkg[i * make_site.BLK + make_site.KEEP] = 0x7D
    pkg[2000:2016] = b"FELUCCA-LOADER-1"
    (tmp / "p.fwsc").write_bytes(bytes(pkg))
    make_site.main(tmp / "p.fwsc", "9.9", tmp / "site")
    app = tmp / "site" / "webapp"
    sw = (app / "sw.js").read_text(encoding="utf-8")
    files = json.loads(re.search(r"const FILES = (\[.*?\]);", sw).group(1))
    path = lambda f: (app / f / "index.html") if f.endswith("/") else (app / f)   # noqa: E731
    check("sw.js: stamped with the version", 'const STAMP = "9.9-' in sw and "/*" + "FILES*/" not in sw)
    check("sw.js keeps the installer, the editor, its font, the firmware",
          {"installer/", "editor/", "editor/fukiai.ttf", "../firmware/sloop-9.9.fwsc"} <= set(files))
    check("sw.js keeps the MidiPipe tutorial and shared documentation styles",
          {"changes/midipipe.html", "changes/changes.css"} <= set(files))
    check("sw.js keeps the licences page at its directory URL", "licences/" in files)
    actual_images = {p.name for p in (app / "changes/screenshots").glob("*.png")}
    check("site copies only listed GPL documentation images", actual_images == set(make_site.SCREENSHOTS))
    check("historical screenshots never enter the generated site",
          not {"sloop-home-before.png", "sloop-options-before.png"} & actual_images)
    legal = (app / "licences/index.html").read_text()
    check("licences page: no unrelated source advertised without a matching bundle",
          "No matching source bundle" in legal and "Historical artwork with unresolved terms is excluded" in legal)
    # A bundle claiming a different firmware hash must never be linked as corresponding source.
    release = tmp / "release"
    release.mkdir()
    info = {"version": "9.9", "artifacts": {"piterfly-9.9.fwsc": {"sha256": "0" * 64}}}
    (release / "BUILD_INFO.json").write_text(json.dumps(info))
    try:
        make_site.licence_page(tmp / "site", "9.9", bytes(pkg), release)
        rejected = False
    except ValueError:
        rejected = True
    check("wrong firmware/source pairing is rejected", rejected)
    # Complete download set: every SHA256SUMS entry must actually be downloadable.
    artifacts = {"piterfly-9.9.fwsc": bytes(pkg), "piterfly-9.9-source.tar.gz": b"source fixture",
                 "piterfly-9.9-licences.zip": b"licence fixture"}
    info.update(source_tree_sha256="fixture", base_commit="fixture", artifacts={
        n: {"sha256": hashlib.sha256(data).hexdigest()} for n, data in artifacts.items()})
    for name, data in artifacts.items():
        (release / name).write_bytes(data)
    (release / "BUILD_INFO.json").write_text(json.dumps(info))
    sums = {**artifacts, "BUILD_INFO.json": (release / "BUILD_INFO.json").read_bytes()}
    (release / "SHA256SUMS").write_text("".join(f"{hashlib.sha256(data).hexdigest()}  {name}\n" for name, data in sums.items()))
    make_site.main(tmp / "p.fwsc", "9.9", tmp / "pages", release_dir=release)
    check_pages_site.check(tmp / "pages", release)
    check("Pages: checksum-covered artifacts, relative links and /piterfly/ manifest are valid", True)
    index = tmp / "pages/index.html"
    index.write_text(index.read_text() + '<a href="/webapp/editor/">bad root link</a>')
    try:
        check_pages_site.check(tmp / "pages", release)
        rejected = False
    except ValueError:
        rejected = True
    check("Pages: link escaping the /piterfly/ prefix is rejected", rejected)
    check("sw.js: every file it keeps is in the site", all(path(f).is_file() for f in files))
    m = json.loads((app / "sloop.webmanifest").read_text(encoding="utf-8"))
    check("manifest: standalone, starts at the editor, icons 192 and 512 (+ maskable)",
          m["display"] == "standalone" and m["start_url"] == "editor/" and
          {i["sizes"] for i in m["icons"]} == {"192x192", "512x512"} and
          any(i.get("purpose") == "maskable" for i in m["icons"]) and
          all((app / i["src"]).is_file() for i in m["icons"]))
    for page in ("editor", "installer"):
        h = (app / page / "index.html").read_text(encoding="utf-8")
        check(f"{page}: links the manifest, registers ../sw.js, an install button",
              'href="../sloop.webmanifest"' in h and 'register("../sw.js")' in h and "beforeinstallprompt" in h)
    stamp = sw.split('const STAMP = "')[1].split('"')[0]
    (tmp / "site2").mkdir()
    make_site.main(tmp / "p.fwsc", "9.9", tmp / "site2")
    same = (tmp / "site2" / "webapp" / "sw.js").read_text(encoding="utf-8").split('const STAMP = "')[1].split('"')[0]
    pkg[3000] ^= 1
    (tmp / "p.fwsc").write_bytes(bytes(pkg))
    make_site.main(tmp / "p.fwsc", "9.9", tmp / "site2")
    other = (tmp / "site2" / "webapp" / "sw.js").read_text(encoding="utf-8").split('const STAMP = "')[1].split('"')[0]
    check("sw.js: the same site, the same stamp; a file changed, a new one", stamp == same and stamp != other)
finally:
    shutil.rmtree(tmp, ignore_errors=True)
print("site:", "FAILED" if bad else "offline editor and offline site test passed")
sys.exit(1 if bad else 0)
