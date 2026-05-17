#!/usr/bin/env python3
"""Author the body + eye palette PNGs the build pipeline expects.

build.py looks for sprite_forge/palettes/{name}.png (16 wide × 8 tall,
each column = one gene index 0..15, each row = one shade 0=dark → 7=light).
If the file is missing, build.py falls back to a generated rainbow which
is fine for testing but ugly — two random pets look like screaming 1980s
parrots.

This script picks 16 *coherent* base hues per palette, then derives an
8-step ramp per hue by linear interpolation through HSL toward black at
the dark end and toward (the hue's pale tint) at the light end. The
goal: every pet looks like it belongs in the same warm-pastel scene
the rest of the UI uses, regardless of which gene value it rolled.

Re-run after editing PALETTES. Output goes to
sprite_forge/palettes/{body,eye}.png — both committed.
"""
from __future__ import annotations

import colorsys
import sys
from pathlib import Path

try:
    from PIL import Image
except ModuleNotFoundError:
    sys.exit("Pillow not installed — run: pip install -r requirements.txt")

ROOT = Path(__file__).resolve().parent.parent
PAL_DIR = ROOT / "sprite_forge" / "palettes"

ENTRIES = 16   # one per gene index
RAMP    = 8    # shades dark→light per entry

# 16 body hues — pastel pet colours that read against the cream wall +
# wood floor. Each entry is the MID shade; the 8-step ramp interpolates
# from a near-black version up to a pale tinted highlight.
# Picked deliberately to avoid muddy adjacent hues — every gene roll
# should look distinct from its neighbours.
BODY_HUES = [
    (0xf4, 0xa7, 0xb9),   # 0  blush pink
    (0xf9, 0xc8, 0xa6),   # 1  peach
    (0xf6, 0xe2, 0x8f),   # 2  butter yellow
    (0xc4, 0xe1, 0x9c),   # 3  spring green
    (0x9c, 0xd9, 0xb8),   # 4  mint
    (0x90, 0xcc, 0xd6),   # 5  sky aqua
    (0xa0, 0xb8, 0xe6),   # 6  cornflower blue
    (0xc1, 0xa8, 0xe2),   # 7  lavender
    (0xe8, 0xa8, 0xc8),   # 8  rose
    (0xf3, 0xb5, 0x86),   # 9  apricot
    (0xea, 0xd1, 0x7a),   # 10 honey
    (0x96, 0xc7, 0x82),   # 11 sage
    (0x6f, 0xc4, 0xc4),   # 12 sea-foam
    (0xb8, 0xc0, 0xe8),   # 13 periwinkle
    (0xd4, 0x9c, 0xc8),   # 14 mauve
    (0xe4, 0xc3, 0xa0),   # 15 sand
]

# 16 eye hues — richer, saturated for tiny areas. Eyes are small in the
# composed sprite (a few pixels of pupil) so they need stronger colour
# to register; pastel eyes wash out at distance.
EYE_HUES = [
    (0x4b, 0x2e, 0x14),   # 0  dark brown
    (0x8b, 0x4f, 0x1f),   # 1  hazel
    (0xc8, 0x84, 0x2a),   # 2  amber
    (0x9b, 0xa5, 0x2a),   # 3  olive
    (0x4a, 0x8f, 0x3a),   # 4  jade
    (0x2a, 0x6f, 0x6f),   # 5  teal
    (0x2c, 0x5a, 0xa8),   # 6  sapphire
    (0x6c, 0x4a, 0xa8),   # 7  violet
    (0xa8, 0x3a, 0x70),   # 8  magenta
    (0xb0, 0x2a, 0x2a),   # 9  ruby
    (0xe2, 0xb8, 0x4a),   # 10 gold
    (0x3a, 0x6a, 0x4a),   # 11 forest
    (0x5a, 0x82, 0xb8),   # 12 ocean
    (0x88, 0x6a, 0x4a),   # 13 chestnut
    (0x4a, 0x4a, 0x5a),   # 14 slate
    (0xc8, 0xa8, 0x9c),   # 15 rose-quartz
]


def lerp(a: int, b: int, t: float) -> int:
    return int(round(a + (b - a) * t))


def ramp_for(rgb: tuple[int, int, int]) -> list[tuple[int, int, int]]:
    """Build an 8-step dark→light ramp through the given hue.

    Step 0 = ~10% luma version (almost black, preserves the hue),
    step 7 = pale highlight (the hue lightened ~70% toward white).
    Middle steps interpolate linearly in RGB space — simple, gives
    a perceptually OK ramp for pixel-art shading without needing a
    full HSL/LAB transform.
    """
    r, g, b = rgb
    # Dark end: 15% of the base colour (almost the hue's shadow).
    dark = (lerp(0, r, 0.15), lerp(0, g, 0.15), lerp(0, b, 0.15))
    # Light end: 70% toward white (preserves identity, brightens).
    light = (lerp(r, 255, 0.55), lerp(g, 255, 0.55), lerp(b, 255, 0.55))
    out = []
    for s in range(RAMP):
        t = s / (RAMP - 1)
        out.append((
            lerp(dark[0], light[0], t),
            lerp(dark[1], light[1], t),
            lerp(dark[2], light[2], t),
        ))
    return out


def write_palette(path: Path, hues: list[tuple[int, int, int]]) -> None:
    assert len(hues) == ENTRIES, f"need {ENTRIES} hues, got {len(hues)}"
    img = Image.new("RGB", (ENTRIES, RAMP))
    for x, hue in enumerate(hues):
        for y, shade in enumerate(ramp_for(hue)):
            img.putpixel((x, y), shade)
    img.save(path)
    print(f"wrote {path}")


def main() -> int:
    PAL_DIR.mkdir(parents=True, exist_ok=True)
    write_palette(PAL_DIR / "body.png", BODY_HUES)
    write_palette(PAL_DIR / "eye.png", EYE_HUES)
    return 0


if __name__ == "__main__":
    sys.exit(main())
