#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check the generated Pages artifact without uploading it or enabling Pages."""
import hashlib
from html.parser import HTMLParser
import json
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urljoin, urlsplit

BASE = 'https://piotrwozniak.github.io/piterfly/'


def check(site, release):
    site, release = Path(site).resolve(), Path(release).resolve()
    info = json.loads((release / 'BUILD_INFO.json').read_text())
    version = info['version']

    def local(url):
        u = urlsplit(url)
        if u.netloc != urlsplit(BASE).netloc:
            return None
        if not u.path.startswith('/piterfly/'):
            raise ValueError(f'Link escapes /piterfly/: {url}')
        p = (site / unquote(u.path[len('/piterfly/'):])).resolve()
        if not p.is_relative_to(site):
            raise ValueError(f'Path escapes generated site: {url}')
        if u.path.endswith('/'):
            p /= 'index.html'
        if not p.is_file():
            raise ValueError(f'Missing site asset: {url}')
        return p

    class Links(HTMLParser):
        def __init__(self, url):
            super().__init__()
            self.url = url

        def handle_starttag(self, tag, attrs):
            a = dict(attrs)
            if tag == 'base' and a.get('href'):
                self.url = urljoin(self.url, a['href'])
                local(self.url)
            for key in ('href', 'src'):
                if key in a:
                    local(urljoin(self.url, a[key]))
            if tag == 'meta' and a.get('http-equiv', '').lower() == 'refresh':
                local(urljoin(self.url, a['content'].split('url=', 1)[1]))

    for p in site.rglob('*'):
        if p.is_symlink():
            raise ValueError(f'Symlink in Pages artifact: {p}')
        if p.is_file():
            rel = p.relative_to(site)
            if rel.parts[0] not in ('index.html', 'install-24-patterns.html', 'webapp', 'firmware', 'source') or any(x.startswith('.') for x in rel.parts):
                raise ValueError(f'Unexpected file in public artifact: {rel}')
            if p.suffix == '.html':
                Links(urljoin(BASE, rel.as_posix())).feed(p.read_text())

    sw_url = urljoin(BASE, 'webapp/sw.js')
    sw = local(sw_url).read_text()
    files = json.loads(re.search(r'const FILES = (\[.*?\]);', sw).group(1))
    for f in files:
        local(urljoin(sw_url, f))
    if 'encodeURIComponent(new URL(".", self.location).pathname)' not in sw:
        raise ValueError('Service-worker caches must be isolated by project path')
    manifest_url = urljoin(BASE, 'webapp/sloop.webmanifest')
    manifest = json.loads(local(manifest_url).read_text())
    if urljoin(manifest_url, manifest['scope']) != urljoin(BASE, 'webapp/'):
        raise ValueError('Wrong manifest scope')
    local(urljoin(manifest_url, manifest['start_url']))
    for icon in manifest['icons']:
        local(urljoin(manifest_url, icon['src']))

    installer = local(urljoin(BASE, 'webapp/installer/')).read_text()
    meta = json.loads(re.search(r'const meta = (\{.*?\});', installer).group(1))
    firmware = local(urljoin(BASE + 'webapp/installer/', meta['pkg']))
    if hashlib.sha256(firmware.read_bytes()).hexdigest() != meta['sha256'] or meta['version'] != version:
        raise ValueError('Installer firmware metadata mismatch')
    if firmware.read_bytes() != (release / f'piterfly-{version}.fwsc').read_bytes():
        raise ValueError('Installer firmware differs from corresponding source bundle')
    for name in [*info['artifacts'], 'BUILD_INFO.json', 'SHA256SUMS']:
        if Path(name).name != name or name.startswith('.'):
            raise ValueError(f'Unsafe release filename: {name}')
        served = local(urljoin(BASE, 'source/' + name))
        if served.read_bytes() != (release / name).read_bytes():
            raise ValueError(f'Release artifact mismatch: {name}')
        if '../source/' + name not in files:
            raise ValueError(f'Download absent from offline cache: {name}')
    for line in (site / 'source/SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ')
        if name not in {*info['artifacts'], 'BUILD_INFO.json'}:
            raise ValueError(f'Unexpected checksum entry: {name}')
        if hashlib.sha256((site / 'source' / name).read_bytes()).hexdigest() != digest:
            raise ValueError(f'Checksum mismatch: {name}')
    print(f'Pages PASS: /piterfly/ links, downloads, firmware/source pairing, SHA256SUMS, manifest and {len(files)} offline files')


if __name__ == '__main__':
    if len(sys.argv) != 3:
        raise SystemExit('usage: check_pages_site.py SITE_DIR RELEASE_DIR')
    check(*sys.argv[1:])
