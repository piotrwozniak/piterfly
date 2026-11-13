#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
"""Render the bitmap fonts to a C header (assets/fonts/):
  S  labels / units / status   Latin-1 32..255 at 1x
  L  large values / titles     32..95 (digits, signs, capitals), the same glyphs at 2x
Terminus Bold 8x16 (BDF, SIL OFL 1.1). TTF fonts also work through render()
(anti-aliased, tabular figures via the OpenType `tnum` feature).

Glyph format: per glyph an advance width, a bitmap width and an offset; the
bitmap starts FONT_PAD pixels left of the pen position (room for side
bearings); rows top to bottom. The 8x16 Terminus font is stored as cropped
8-bit rows: header = top << 4 | (rows - 1), 255 = blank (bits = 2).
Identical visible glyphs and overlapping byte sequences share storage; lower
case and control glyphs already map to capitals / '?' in gfx.c. Uniform advance
and bitmap widths use constants instead of tables. Other on/off fonts keep
packed 1-bit pixels (bits = 1); anti-aliased TTF fonts use two alpha nibbles
per byte (bits = 0). FONT_L shares FONT_S data and draws it at 2x.
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

FONTS = Path(__file__).resolve().parents[1] / "assets" / "fonts"
# (name, file, px, scale, pixel, first, last): pixel fonts are rendered without
# anti-aliasing at their design size and enlarged by an integer factor
# SLOOP 2.6: Terminus Bold (the same 8 x 16 cell and advances as the regular one: no layout moves), drawn in
# capitals only (gfx.c glyph); the regular ter-u16n.bdf stays for the web editor's font (gen_webfont.py)
SIZES = [("S", "ter-u16b.bdf", 16, 1, True, 32, 255),   # Latin-1 (Hügelton needs the umlaut)
         ("L", "ter-u16b.bdf", 16, 2, True, 32, 95)]   # values / titles: digits, signs, capitals
PAD = 2


def upscale(px_, w, h, scale):
    """integer nearest-neighbour enlargement of a w x h glyph"""
    if scale == 1:
        return px_
    big = []
    for y in range(h):
        row = []
        for x in range(w):
            row += [px_[y * w + x]] * scale
        big += row * scale
    return big


def render_bdf(path, scale, first, last):
    """BDF bitmap font (Terminus 8x16): exact pixels, cell = the font bounding box."""
    lines = path.read_text(errors="replace").splitlines()
    fbb = next(l for l in lines if l.startswith("FONTBOUNDINGBOX")).split()
    cw, ch, fx, fy = int(fbb[1]), int(fbb[2]), int(fbb[3]), int(fbb[4])
    base = ch + fy                                     # rows above the baseline
    glyphs, i = {}, 0
    while i < len(lines):
        if lines[i].startswith("ENCODING"):
            code = int(lines[i].split()[1])
            j = i
            while not lines[j].startswith("BBX"):
                j += 1
            bw, bh, bx, by = map(int, lines[j].split()[1:5])
            dw = cw
            k = i
            while not lines[k].startswith("BITMAP"):
                if lines[k].startswith("DWIDTH"):
                    dw = int(lines[k].split()[1])
                k += 1
            rows = [int(r, 16) for r in lines[k + 1:k + 1 + bh]]
            nbits = ((bw + 7) // 8) * 8
            cell = [0] * (cw * ch)
            top = base - (by + bh)
            for r, bits in enumerate(rows):
                for c in range(bw):
                    if bits >> (nbits - 1 - c) & 1:
                        x, y = bx + c, top + r
                        if 0 <= x < cw and 0 <= y < ch:
                            cell[y * cw + x] = 15
            glyphs[code] = (dw, cell)
            i = k + bh
        i += 1
    if 0xDC not in glyphs and ord("U") in glyphs:      # Ü for "HÜGELTON": U with dots on its top row
        dw, cell = glyphs[ord("U")]
        cell = cell[:]
        lit = [x for y in range(ch) for x in range(cw) if cell[y * cw + x]]
        if lit:
            top = min(y for y in range(ch) for x in range(cw) if cell[y * cw + x])
            for x in range(cw):
                cell[top * cw + x] = 15 if x in (min(lit), max(lit)) else 0
            for x in range(cw):
                cell[(top + 1) * cw + x] = 0
            glyphs[0xDC] = (dw, cell)
    missing = [chr(c) for c in range(first, min(last, 126) + 1) if c not in glyphs]
    if missing:
        print(f"font {path.name}: no glyph for {''.join(missing)!r} (drawn as '?')")
    out = []
    for c in range(first, last + 1):
        dw, cell = glyphs.get(c, glyphs[ord("?")])
        bw = cw + 2 * PAD                              # keep the PAD convention of the TTF path
        px_ = []
        for y in range(ch):
            px_ += [0] * PAD + cell[y * cw:(y + 1) * cw] + [0] * PAD
        out.append((dw * scale, bw * scale, upscale(px_, bw, ch, scale)))
    return ch * scale, out


def render(ttf, px, scale, pixel, first, last):
    if ttf.endswith(".bdf"):
        return render_bdf(FONTS / ttf, scale, first, last)
    font = ImageFont.truetype(str(FONTS / ttf), px)
    feats = None if pixel else ["tnum"]
    asc, desc = font.getmetrics()
    top = max(0, font.getbbox("A8|(", features=feats)[1] - 1)
    h = asc + desc - top
    glyphs = []
    for c in range(first, last + 1):
        ch = chr(c)
        adv = int(round(font.getlength(ch, features=feats)))
        bw = adv + 2 * PAD
        img = Image.new("L", (bw, asc + desc), 0)
        ImageDraw.Draw(img).text((PAD, 0), ch, font=font, fill=255, features=feats)
        img = img.crop((0, top, bw, top + h))
        px_ = [(15 if v >= 128 else 0) if pixel else min(15, (v + 8) // 17) for v in img.tobytes()]
        glyphs.append((adv * scale, bw * scale, upscale(px_, bw, h, scale)))
    return h * scale, glyphs


def metric(lines, name, values):
    if len(set(values)) == 1:
        return "0", values[0]
    lines.append(f"static const uint8_t {name}[{len(values)}] = {{" + ", ".join(map(str, values)) + "};")
    return name, 0


def main(out):
    lines = [f"/* generated by tools/gen_font.py from {SIZES[0][1]} */",
             "#pragma once", "#include <stdint.h>", f"#define FONT_PAD {PAD}  /* x scale for L, see below */", ""]
    base = None                                        # (name, first, h, glyphs) of the 1x font
    for name, ttf, px, scale, pixel, first, last in SIZES:
        h, glyphs = render(ttf, px, scale, pixel, first, last)
        if scale > 1 and pixel and base is not None:
            # SLOOP 2.4: a pixel font at 2x is the 1x font's own bitmaps, enlarged while drawing (gfx.c
            # cv_text): no second copy in flash (24 KB). Same glyphs, same pixels: checked here.
            bname, bfirst, bh, bglyphs, bone = base
            assert scale == 2 and first == bfirst and h == bh * 2, "L must be S at 2x, same first glyph"
            for (adv, bw, g), (badv, bbw, bg) in zip(glyphs, bglyphs):
                assert adv == badv * 2 and bw == bbw * 2 and g == upscale(bg, bbw, bh, 2)
            adv, fixed_adv = metric(lines, f"FONT_{name}_ADV", [a for a, _, _ in glyphs])
            bw, fixed_bw = metric(lines, f"FONT_{name}_BW", [b for _, b, _ in glyphs])
            lines.append(f"static const felucca_font_t FONT_{name} = {{ {h}, {PAD * scale}, {first}, {last}, 1, "
                         f"{adv}, {bw}, FONT_{bname}_OFF, FONT_{bname}_DATA, {bone}, {fixed_adv}, {fixed_bw} }};")
            lines.append("")
            print(f"font {name}: {ttf} {px}px h {h}, digit adv {glyphs[ord('0') - first][0]}, "
                  f"the {bname} bitmaps at {scale}x (0 B)")
            continue
        data, offs, packed = [], [], []
        one = all(v in (0, 15) for _, _, g in glyphs for v in g)
        cropped = one and pixel and h <= 16 and all(bw == 8 + 2 * PAD for _, bw, _ in glyphs)
        for adv, bw, g in glyphs:
            raw = []
            if cropped:
                rows = [sum((g[y * bw + PAD + x] != 0) << (7 - x) for x in range(8)) for y in range(h)]
                assert all(not g[y * bw + x] for y in range(h) for x in (*range(PAD), *range(PAD + 8, bw)))
                ink = [i for i, value in enumerate(rows) if value]
                raw = [255] if not ink else [ink[0] << 4 | (ink[-1] - ink[0]), *rows[ink[0]:ink[-1] + 1]]
                restored = [0] * h if not ink else [0] * ink[0] + raw[1:] + [0] * (h - ink[-1] - 1)
                assert restored == rows, "cropping must preserve every pixel"
            elif one:
                acc = 0
                for i, value in enumerate(g + [0] * (-len(g) % 8)):
                    acc = (acc << 1) | (value != 0)
                    if i % 8 == 7:
                        raw.append(acc); acc = 0
                assert [15 if raw[i // 8] >> (7 - i % 8) & 1 else 0 for i in range(len(g))] == g
            else:
                for y in range(h):
                    row = g[y * bw:(y + 1) * bw] + [0]
                    raw.extend((row[x] << 4) | row[x + 1] for x in range(0, bw, 2))
            packed.append(bytes(raw))
        # gfx.c already folds lower case and C1 controls; their unused bitmaps share the visible glyph.
        for c in range(first, last + 1):
            canonical = c - 32 if (97 <= c <= 122 or 224 <= c <= 254 and c != 247) else c
            if 127 <= canonical < 160: canonical = 63
            raw = packed[canonical - first] if first <= canonical <= last else packed[c - first]
            blob = bytes(data)
            off = blob.find(raw)
            if off < 0:
                overlap = next((n for n in range(min(len(blob), len(raw)), 0, -1) if blob[-n:] == raw[:n]), 0)
                off = len(data) - overlap
                data.extend(raw[overlap:])
            assert bytes(data[off:off + len(raw)]) == raw
            offs.append(off)
        bits = 2 if cropped else 1 if one else 0
        assert len(data) < 65536
        lines.append(f"static const uint8_t FONT_{name}_DATA[{len(data)}] = {{")
        for i in range(0, len(data), 24):
            lines.append("    " + ", ".join(f"0x{b:02x}" for b in data[i:i + 24]) + ",")
        lines.append("};")
        lines.append(f"static const uint16_t FONT_{name}_OFF[{len(offs)}] = {{" + ", ".join(map(str, offs)) + "};")
        adv, fixed_adv = metric(lines, f"FONT_{name}_ADV", [a for a, _, _ in glyphs])
        bw, fixed_bw = metric(lines, f"FONT_{name}_BW", [b for _, b, _ in glyphs])
        lines.append(f"static const felucca_font_t FONT_{name} = {{ {h}, {PAD * scale}, {first}, {last}, 0, "
                     f"{adv}, {bw}, FONT_{name}_OFF, FONT_{name}_DATA, {bits}, {fixed_adv}, {fixed_bw} }};")
        lines.append("")
        if scale == 1:
            base = (name, first, h, glyphs, bits)
        print(f"font {name}: {ttf} {px}px h {h}, digit adv {glyphs[ord('0') - first][0]}, {len(data)} B")
    Path(out).write_text("\n".join(lines))


if __name__ == "__main__":
    main(sys.argv[1])
