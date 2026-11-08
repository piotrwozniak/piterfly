#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check the editable GPL source, checked-in atlas and firmware packer agree."""
import contextlib
import io
import json
import sys
import tempfile
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import gen_icons
import gen_pixel_icons

meta = json.loads((ROOT / "assets/icons.json").read_text())
assert len(meta["names"]) == 86 and meta["cell"] == 12 and meta["cols"] == 16
for name, expected in {"sample":43, "midi":63, "save":64, "load":65, "clear":66,
                       "tape":79, "drum":80, "grain":85}.items():
    assert meta["names"][expected] == name, (name, expected)
with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    gen_pixel_icons.main(tmp / "icons.png")
    assert (tmp / "icons.png").read_bytes() == (ROOT / "assets/icons.png").read_bytes(), "Regenerate the committed atlas"
    (tmp / "icons.json").write_text(json.dumps(meta))
    with Image.open(tmp / "icons.png") as image:
        assert image.size == (192, 72) and image.mode == "L"
        assert "GPL-3.0-only" in image.info["License"]
        assert set(image.tobytes()) <= {0, 85, 170, 255}
    cell, names, cells, _ = gen_icons.load(tmp)
    assert cell == 12 and names == meta["names"] and len(cells) == 86
    assert all(len(pixels) == 144 and any(pixels) for pixels in cells)
    for name, pixels in zip(names, cells):
        expected = gen_pixel_icons.icons[name]
        assert pixels == [expected.get((x,y), 0) for y in range(12) for x in range(12)], name
    with contextlib.redirect_stdout(io.StringIO()):
        gen_icons.main(tmp / "packed.h", tmp)
    header = (tmp / "packed.h").read_text()
    assert "ICON_DATA[86][36]" in header and "FELUCCA_ICONS_N 86" in header
print("GPL pixel icons: 86 IDs, reproducible atlas, 2-bit packing, 3096 bytes PASS")
