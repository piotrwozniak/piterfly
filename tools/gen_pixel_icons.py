#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""piterfly's original 12x12 pixel icons; no inherited atlas pixels are used.

Regenerate: python tools/gen_pixel_icons.py assets/icons.png
The first 20 shapes preserve the accepted arcade proposal. Existing icon IDs
come from assets/icons.json; every remaining shape is drawn here from geometry.
"""
import argparse
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CELL = 12
icons={}
def icon(name):
 assert name not in icons, name
 p={};icons[name]=p
 def dot(x,y,v=3):
  assert 0<=x<12 and 0<=y<12,(name,x,y)
  p[x,y]=v
 def line(x,y,a,b,v=3):
  dx=abs(a-x);sx=1 if x<a else -1;dy=-abs(b-y);sy=1 if y<b else -1;e=dx+dy
  while True:
   dot(x,y,v)
   if (x,y)==(a,b):break
   q=2*e
   if q>=dy:e+=dy;x+=sx
   if q<=dx:e+=dx;y+=sy
 def box(x,y,a,b,v=3):
  line(x,y,a,y,v);line(a,y,a,b,v);line(a,b,x,b,v);line(x,b,x,y,v)
 def fill(x,y,a,b,v=3):
  for xx in range(x,a+1):
   for yy in range(y,b+1):dot(xx,yy,v)
 return dot,line,box,fill
D,L,B,F=icon('save');L(1,1,9,1);L(1,1,1,10);L(1,10,10,10);L(10,2,10,10);B(3,1,7,4);L(7,2,7,3);B(3,7,8,10)
D,L,B,F=icon('load');L(5,1,5,7);L(3,5,5,7);L(5,7,7,5);L(1,7,1,10);L(1,10,10,10);L(10,7,10,10)
D,L,B,F=icon('clear');L(2,3,9,3);L(4,1,7,1);L(4,2,7,2);L(3,4,3,9);L(8,4,8,9);L(4,10,7,10);L(5,5,5,8);L(6,5,6,8)
D,L,B,F=icon('tape');B(1,2,10,9);B(2,4,4,6);B(7,4,9,6);L(5,5,6,5);L(4,8,7,8)
D,L,B,F=icon('drum');L(3,2,8,2);L(1,3,2,3);L(9,3,10,3);L(1,4,10,4);L(1,5,1,8);L(10,5,10,8);L(2,9,9,9);L(4,5,4,8);L(7,5,7,8);L(3,0,7,3)
D,L,B,F=icon('grain');F(1,2,2,3);F(6,1,7,2);F(9,4,10,5);F(4,5,5,6);F(1,8,2,9);F(7,8,8,9);D(9,1);D(4,9)
D,L,B,F=icon('sample');L(1,6,1,6);L(2,4,2,8);L(4,1,4,10);L(6,3,6,9);L(8,4,8,8);L(10,5,10,7)
D,L,B,F=icon('env');L(1,10,3,2);L(3,2,5,5);L(5,5,8,5);L(8,5,10,10)
D,L,B,F=icon('cutoff');L(1,1,1,10);L(1,10,10,10);L(2,3,6,3);L(6,3,8,7);L(8,7,10,8)
D,L,B,F=icon('reso');L(1,8,3,8);L(3,8,5,2);L(5,2,6,2);L(6,2,8,8);L(8,8,10,8)
D,L,B,F=icon('delay');B(1,1,3,5);B(5,3,7,7,2);B(9,5,10,9,1)
D,L,B,F=icon('reverb');F(1,5,2,6);L(4,3,4,8);L(7,2,7,9,2);L(10,1,10,10,1)
D,L,B,F=icon('lfo_wave');L(1,6,2,3);L(2,3,4,3);L(4,3,7,8);L(7,8,9,8);L(9,8,10,5)
D,L,B,F=icon('mix');L(2,1,2,10);L(5,1,5,10);L(9,1,9,10);F(1,3,3,4);F(4,7,6,8);F(8,5,10,6)
D,L,B,F=icon('gate');L(1,9,3,9);L(3,9,3,2);L(3,2,7,2);L(7,2,7,9);L(7,9,10,9)
D,L,B,F=icon('tempo');L(5,1,1,10);L(1,10,10,10);L(10,10,6,1);L(6,6,9,2);D(5,6)
D,L,B,F=icon('steps')
for x in (0,3,6,9):
 for y in (2,5,8):F(x,y,x+1,y+1,3 if (x+y)%3 else 1)
D,L,B,F=icon('midi');L(4,1,7,1);L(2,2,3,2);L(8,2,9,2);L(1,3,1,7);L(10,3,10,7);L(2,8,3,8);L(8,8,9,8);L(4,9,7,9);L(5,10,6,10)
for x,y in [(4,3),(7,3),(3,5),(8,5),(5,7),(6,7)]:D(x,y)
D,L,B,F=icon('arp');L(1,9,3,9);L(3,9,3,7);L(3,7,5,7);L(5,7,5,5);L(5,5,7,5);L(7,5,7,3);L(7,3,10,3)
D,L,B,F=icon('generic');F(1,2,2,3);L(4,2,10,2);L(4,3,8,3);F(1,5,2,6);L(4,5,10,5);L(4,6,7,6);F(1,8,2,9);L(4,8,10,8);L(4,9,9,9)


def circle(dot, cx=5, cy=5, radius=4, value=3):
    x, y, error = radius, 0, 1 - radius
    while x >= y:
        for dx, dy in [(x,y),(y,x),(-y,x),(-x,y),(-x,-y),(-y,-x),(y,-x),(x,-y)]:
            dot(cx + dx, cy + dy, value)
        y += 1
        if error < 0:
            error += 2*y + 1
        else:
            x -= 1
            error += 2*(y-x) + 1


def path(line, points, value=3):
    for (x,y),(a,b) in zip(points, points[1:]):
        line(x,y,a,b,value)


# Envelope stages and pitch controls share axes, but keep different silhouettes.
D,L,B,F=icon('attack');L(1,10,10,10);L(2,9,9,1);L(7,1,9,1)
D,L,B,F=icon('decay');L(1,10,10,10);path(L,[(2,1),(3,4),(5,7),(9,9)])
D,L,B,F=icon('sustain');L(1,7,10,7);L(2,6,2,8);L(9,6,9,8);L(4,3,7,3)
D,L,B,F=icon('release');L(1,10,10,10);path(L,[(1,3),(4,3),(6,8),(10,9)])
D,L,B,F=icon('rate');circle(D);L(5,5,5,2);L(5,5,8,5)
D,L,B,F=icon('wave');path(L,[(1,8),(3,2),(5,8),(7,2),(10,8)])
D,L,B,F=icon('pulse');path(L,[(1,9),(2,9),(2,2),(4,2),(4,9),(8,9),(8,2),(9,2),(9,9),(10,9)])
D,L,B,F=icon('shape');path(L,[(2,2),(7,1),(10,5),(8,9),(3,10),(1,6),(2,2)])
D,L,B,F=icon('pitch');L(3,9,3,2);L(1,4,3,2);L(3,2,5,4);L(8,2,8,9);L(6,7,8,9);L(8,9,10,7)
D,L,B,F=icon('tune');path(L,[(2,1),(2,5),(4,7),(6,7),(8,5),(8,1)]);L(5,7,5,10);F(4,9,6,10)
D,L,B,F=icon('detune');path(L,[(1,8),(3,3),(5,8)]);path(L,[(6,7),(8,1),(10,7)])
D,L,B,F=icon('transpose');L(1,9,7,9);L(4,2,10,2);L(6,8,6,3);L(4,5,6,3);L(6,3,8,5)
D,L,B,F=icon('octave');B(1,6,4,9);B(7,1,10,4);L(4,5,6,3);D(5,5)
D,L,B,F=icon('level');path(L,[(1,4),(3,4),(6,1),(6,10),(3,7),(1,7),(1,4)]);L(8,4,9,5);L(9,5,8,7)
D,L,B,F=icon('pan');L(1,5,10,5);L(1,5,3,3);L(1,5,3,7);L(10,5,8,3);L(10,5,8,7);F(5,4,6,6)
D,L,B,F=icon('mute');path(L,[(1,4),(3,4),(5,2),(5,9),(3,7),(1,7),(1,4)]);L(7,4,10,7);L(7,7,10,4)
D,L,B,F=icon('dist');path(L,[(1,8),(3,2),(5,2),(7,9),(9,9),(10,6)])
D,L,B,F=icon('drive');path(L,[(2,1),(2,10),(10,5),(2,1)]);L(0,5,2,5);L(10,5,11,5)
D,L,B,F=icon('fold');path(L,[(1,8),(4,2),(6,5),(8,2),(10,8)])
D,L,B,F=icon('bits');path(L,[(1,9),(1,7),(4,7),(4,5),(7,5),(7,2),(10,2)])
D,L,B,F=icon('chorus');path(L,[(1,3),(3,1),(5,3),(7,1),(10,3)],1);path(L,[(1,6),(3,4),(5,6),(7,4),(10,6)],2);path(L,[(1,9),(3,7),(5,9),(7,7),(10,9)])
D,L,B,F=icon('feedback');path(L,[(3,2),(9,2),(10,3),(10,8),(9,9),(2,9),(1,8),(1,5),(5,5)]);L(3,3,5,5);L(5,5,3,7)
D,L,B,F=icon('swing');F(1,2,2,4);F(7,2,8,4);F(3,7,4,9);F(9,7,10,9);L(2,6,4,6);L(9,5,9,6)
D,L,B,F=icon('scale');B(1,2,10,10);L(4,2,4,10);L(7,2,7,10);F(3,2,4,6);F(6,2,7,6)
D,L,B,F=icon('quantize');B(6,3,10,8);L(1,5,6,5);L(4,3,6,5);L(6,5,4,7);D(8,5)
D,L,B,F=icon('glide');L(1,9,10,2);L(7,2,10,2);L(10,2,10,5)
D,L,B,F=icon('slide');F(1,8,3,10);F(8,1,10,3);L(3,7,8,4)
D,L,B,F=icon('voice');path(L,[(2,9),(2,3),(4,1),(6,1),(7,3),(7,7),(5,9),(2,9)]);L(9,3,10,4);L(10,4,10,6);L(10,6,9,7)
D,L,B,F=icon('mod');circle(D);L(3,5,7,5);L(5,3,5,7);D(10,10)
D,L,B,F=icon('noise')
for x,y in [(1,1),(4,1),(8,1),(10,2),(2,3),(6,3),(9,4),(1,5),(4,5),(7,5),(3,7),(6,7),(10,7),(1,9),(5,9),(8,10)]:D(x,y)
D,L,B,F=icon('sub');path(L,[(1,5),(2,2),(4,2),(7,7),(9,7),(10,4)]);L(2,10,9,10)
D,L,B,F=icon('ratio');path(L,[(1,3),(3,3),(3,5),(1,7),(3,7)]);D(5,4);D(5,7);L(8,3,8,8);L(7,4,8,3);L(7,8,9,8)
D,L,B,F=icon('algorithm');B(1,1,3,3);B(7,1,9,3);L(2,4,2,5);L(8,4,8,5);L(2,5,8,5);L(5,5,5,7);B(4,8,6,10)
D,L,B,F=icon('accent');L(2,5,5,2);L(5,2,8,5);F(4,7,6,9)
D,L,B,F=icon('time');L(2,1,9,1);L(2,10,9,10);path(L,[(3,2),(3,3),(7,7),(8,9)]);path(L,[(8,2),(8,3),(4,7),(3,9)])
D,L,B,F=icon('phase');path(L,[(1,7),(3,2),(5,7),(7,2),(10,7)],1);path(L,[(1,4),(3,9),(5,4),(7,9),(10,4)])
D,L,B,F=icon('fade');F(1,1,2,10);F(4,4,5,10);F(7,7,8,10);L(10,9,10,10)
D,L,B,F=icon('keytrack');B(1,5,7,10);L(3,5,3,10);L(5,5,5,10);L(9,7,9,1);L(7,3,9,1);L(9,1,11,3)
D,L,B,F=icon('size');L(1,3,1,9);L(10,3,10,9);L(2,6,9,6);L(2,6,4,4);L(2,6,4,8);L(9,6,7,4);L(9,6,7,8)
D,L,B,F=icon('damp');path(L,[(1,5),(2,1),(4,9),(6,3),(8,7),(9,5)]);L(10,1,10,10)
D,L,B,F=icon('tone');L(1,10,10,10);F(2,6,3,9);F(5,3,6,9);F(8,1,9,9)
D,L,B,F=icon('sweep');path(L,[(1,8),(3,8),(5,7),(7,5),(9,2)]);L(6,2,9,2);L(9,2,9,5)
D,L,B,F=icon('vibrato');path(L,[(1,3),(3,1),(5,3),(7,1),(10,3)]);L(1,8,10,8);L(1,8,3,6);L(10,8,8,10)
D,L,B,F=icon('loop');path(L,[(1,5),(1,2),(9,2),(9,5)]);L(7,3,9,5);L(9,5,11,3);path(L,[(10,7),(10,9),(2,9),(2,6)]);L(0,8,2,6);L(2,6,4,8)
D,L,B,F=icon('hold');F(2,2,4,9);F(7,2,9,9)
D,L,B,F=icon('order');L(1,2,6,2);L(1,5,5,5);L(1,8,4,8);L(9,1,9,9);L(7,7,9,9);L(9,9,11,7)
D,L,B,F=icon('prob');B(1,1,10,10)
for x,y in [(3,3),(8,3),(5,5),(3,8),(8,8)]:D(x,y)
D,L,B,F=icon('length');L(1,2,1,10);L(10,2,10,10);L(1,6,10,6);D(5,4);D(6,8)
D,L,B,F=icon('division');L(3,8,8,2);F(2,2,3,3);F(8,8,9,9)
D,L,B,F=icon('chip');B(3,3,8,8);B(5,5,6,6)
for n in (3,5,7):L(n,1,n,2);L(n,9,n,10);L(1,n,2,n);L(9,n,10,n)

# Waveform symbols are mathematical sketches, not copies of a font or atlas.
D,L,B,F=icon('w_sin');path(L,[(1,6),(2,3),(3,2),(4,3),(6,8),(7,9),(8,8),(10,3)])
D,L,B,F=icon('w_tri');path(L,[(1,8),(5,1),(10,8)])
D,L,B,F=icon('w_saw');path(L,[(1,9),(8,2),(8,9),(10,7)])
D,L,B,F=icon('w_sqr');path(L,[(1,9),(1,2),(5,2),(5,9),(10,9),(10,2)])
D,L,B,F=icon('w_pls');path(L,[(1,9),(1,2),(3,2),(3,9),(10,9)])
D,L,B,F=icon('w_pwm');path(L,[(1,9),(1,2),(3,2),(3,9),(6,9),(6,2),(9,2),(9,9),(10,9)])
D,L,B,F=icon('w_sh');path(L,[(1,8),(3,8),(3,2),(5,2),(5,6),(7,6),(7,4),(10,4)])
D,L,B,F=icon('w_dsin');path(L,[(1,7),(2,2),(3,7),(4,9),(5,7),(6,2),(7,7),(8,9),(9,7),(10,2)])
D,L,B,F=icon('w_spls');path(L,[(1,9),(1,6),(3,6),(3,2),(5,2),(5,6),(7,6),(7,9),(10,9)])
D,L,B,F=icon('w_rsaw');path(L,[(1,2),(1,9),(4,3),(6,9),(8,4),(10,9)])
D,L,B,F=icon('w_rtri');path(L,[(1,8),(3,2),(5,8),(7,2),(9,8),(10,5)])
D,L,B,F=icon('w_rtrp');path(L,[(1,8),(2,2),(4,2),(5,8),(7,8),(8,2),(9,2),(10,8)])
D,L,B,F=icon('mouth');path(L,[(1,5),(3,3),(5,4),(7,3),(10,5),(8,8),(3,8),(1,5)]);L(3,5,8,5)
D,L,B,F=icon('trio');B(1,7,3,9);B(4,2,6,4);B(8,7,10,9);L(3,6,4,5);L(7,5,8,6)
D,L,B,F=icon('drawbar');L(1,1,1,9);L(5,1,5,6);L(9,1,9,3);F(0,8,2,10);F(4,5,6,7);F(8,2,10,4)
D,L,B,F=icon('slice');path(L,[(1,8),(2,3),(4,8),(6,2),(8,8),(10,4)]);L(5,1,5,10,1);L(9,1,9,10,1)


def main(out):
    from PIL import Image, PngImagePlugin
    meta = json.loads((ROOT / "assets/icons.json").read_text())
    names, cols = meta["names"], meta["cols"]
    assert meta["cell"] == CELL
    assert set(icons) == set(names), (set(names)-set(icons), set(icons)-set(names))
    atlas = Image.new("L", (cols*CELL, math.ceil(len(names)/cols)*CELL))
    for i, name in enumerate(names):
        for (x,y), value in icons[name].items():
            atlas.putpixel(((i % cols)*CELL+x, (i // cols)*CELL+y), value*85)
    notice = PngImagePlugin.PngInfo()
    notice.add_text("License", "GPL-3.0-only; full text in LICENSE")
    notice.add_text("Source", "tools/gen_pixel_icons.py; original piterfly pixel geometry")
    atlas.save(out, pnginfo=notice, compress_level=9)
    print(f"piterfly GPL icons: {len(names)} x {CELL}x{CELL}, {atlas.width}x{atlas.height}; no inherited pixels")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("out", type=Path)
    main(parser.parse_args().out)
