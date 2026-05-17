#!/usr/bin/env python3
"""One-off prototype: draw a single pet at 128x128 native resolution to
compare against the current 64x64-sprite-scaled-2x convention.

NOT part of the pipeline — just a visual experiment. The two outputs are
the SAME PHYSICAL SIZE on screen (both 128x128 pixels), so this isolates
the effect of pixel density alone:

  - left:  current renderer (64x64 sprite, NEAREST 2x = 128x128 display)
  - right: hi-res prototype (128x128 sprite, 1x = 128x128 display)

The prototype renders the same conceptual creature (chubby body, round
eyes with iris+pupil+sparkle, smile mouth, pointy ears, short tail,
spots pattern) but at 4x the pixel budget per feature.

Run:
  python3 sprite_forge/prototype_hires.py
"""
from __future__ import annotations
import sys
from pathlib import Path

try:
    from PIL import Image, ImageDraw
except ModuleNotFoundError:
    sys.exit("Pillow not installed — run: pip install -r requirements.txt")

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "docs" / "previews" / "hires_comparison.png"

# ---- Hi-res 128x128 pet drawn directly in PIL --------------------------------
# All coordinates are in 128x128 frame space. Compare with the 64x64 design:
# body extends ~88x60 here, vs ~38x28 in the current sprite (4x area budget).

def draw_hires_pet() -> Image.Image:
    W = H = 128
    img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)

    # Body palette — pick a peach tint manually to mimic what the body palette
    # gene-tint would produce for one specific pet. (Real renderer would tint
    # at composition time; this prototype skips that.)
    BODY_DARK   = (0x9c, 0x5a, 0x3e)   # outline
    BODY_MID    = (0xd6, 0x97, 0x70)   # mid border
    BODY_FILL   = (0xf9, 0xc8, 0xa6)   # interior (matches palette entry 1)
    BODY_LIGHT  = (0xff, 0xe8, 0xd0)   # highlight
    BODY_SHADOW = (0x6e, 0x3d, 0x28)   # floor shadow + feet
    CHEEK       = (0xff, 0x9c, 0xa8)   # pink cheek blush — possible because
                                       # hi-res sprite could use RGB565 format
                                       # (format=2) rather than gray+tint.

    # Eye + mouth colours
    SCLERA      = (0xff, 0xff, 0xff)
    IRIS        = (0x4a, 0x8f, 0xc8)   # blue iris (eye palette entry would
                                       # tint a grey iris to this colour)
    PUPIL       = (0x10, 0x14, 0x20)
    SPARKLE     = (0xff, 0xff, 0xff)
    MOUTH       = (0x40, 0x20, 0x18)

    # Body: chubby ellipse, 88x60 visible, centred at (64, 76)
    # 3-tone shaded: outline (1px ring), mid (1px ring), interior
    d.ellipse((20, 46, 108, 106), fill=BODY_DARK)
    d.ellipse((21, 47, 107, 105), fill=BODY_MID)
    d.ellipse((22, 48, 106, 104), fill=BODY_FILL)
    # Floor shadow at base
    d.ellipse((22, 92, 106, 104), fill=BODY_SHADOW)
    # Bring interior color back above the shadow so it's only a thin band
    d.ellipse((24, 92, 104, 96), fill=BODY_FILL)
    # Re-add the shadow ribbon at the very base
    d.ellipse((26, 100, 102, 104), fill=BODY_SHADOW)

    # Two visible feet poking out below the shadow
    d.ellipse((38, 102, 50, 108), fill=BODY_SHADOW)
    d.ellipse((78, 102, 90, 108), fill=BODY_SHADOW)

    # Pointy ears, attached to head — small triangles on top
    # Left ear
    d.polygon([(34, 26), (44, 50), (24, 50)], fill=BODY_DARK)
    d.polygon([(34, 30), (43, 50), (25, 50)], fill=BODY_MID)
    d.polygon([(34, 34), (42, 50), (26, 50)], fill=BODY_FILL)
    # Inner ear blush
    d.polygon([(34, 40), (40, 48), (28, 48)], fill=CHEEK)

    # Right ear
    d.polygon([(94, 26), (104, 50), (84, 50)], fill=BODY_DARK)
    d.polygon([(94, 30), (103, 50), (85, 50)], fill=BODY_MID)
    d.polygon([(94, 34), (102, 50), (86, 50)], fill=BODY_FILL)
    d.polygon([(94, 40), (100, 48), (88, 48)], fill=CHEEK)

    # Dome highlight (on the body's top dome, above the eyes)
    d.ellipse((38, 50, 60, 60), fill=BODY_LIGHT)

    # Cheek blush — soft pink ovals BELOW the eyes
    d.ellipse((28, 80, 38, 86), fill=CHEEK)
    d.ellipse((90, 80, 100, 86), fill=CHEEK)

    # Eyes — left
    d.ellipse((40, 60, 60, 80), fill=SCLERA)               # sclera 20x20
    d.ellipse((44, 64, 56, 76), fill=IRIS)                 # iris 12x12
    d.ellipse((48, 68, 54, 74), fill=PUPIL)                # pupil 6x6
    d.ellipse((49, 64, 53, 68), fill=SPARKLE)              # sparkle highlight
    d.ellipse((50, 72, 52, 74), fill=(180, 200, 230))      # lower iris glint

    # Eyes — right (mirror)
    d.ellipse((68, 60, 88, 80), fill=SCLERA)
    d.ellipse((72, 64, 84, 76), fill=IRIS)
    d.ellipse((76, 68, 82, 74), fill=PUPIL)
    d.ellipse((77, 64, 81, 68), fill=SPARKLE)
    d.ellipse((78, 72, 80, 74), fill=(180, 200, 230))

    # Mouth — curved smile at y=86-90
    d.arc((54, 82, 74, 92), start=0, end=180, fill=MOUTH, width=2)

    # Pattern: scattered 4x4 spots on the body lower half
    spots = [(36, 86), (90, 88), (60, 100), (78, 96), (40, 96)]
    for sx, sy in spots:
        d.ellipse((sx-3, sy-2, sx+3, sy+2), fill=(0xc4, 0x80, 0x52))

    return img


def draw_current_chubby_for_comparison(scale: int = 2) -> Image.Image:
    """Use the existing preview.py composer to render the closest equivalent
    pet at the current 64x64 sprite size, then NEAREST upscale to match the
    hi-res output's 128x128. Same physical screen size, different pixel
    densities — apples-to-apples visual comparison."""
    import sys
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from preview import compose_pet  # noqa: E402
    # Chubby body, peach palette, round eyes, smile mouth, pointy ears,
    # short tail, spots pattern, neutral personality.
    img = compose_pet([0, 1, 2, 6, 1, 3, 2, 0])
    return img.resize((img.width * scale, img.height * scale),
                      Image.Resampling.NEAREST)


def main() -> int:
    current = draw_current_chubby_for_comparison(scale=2)
    hires = draw_hires_pet()

    # Compose side-by-side with labels + a divider so the comparison reads
    # cleanly. Background matches the device scene cream so the comparison
    # looks "as it would on the device floor".
    pad = 16
    label_h = 24
    BG = (0xfd, 0xf6, 0xe3, 0xff)
    out_w = pad * 3 + current.width + hires.width
    out_h = pad * 2 + label_h + max(current.height, hires.height)
    out = Image.new("RGBA", (out_w, out_h), BG)
    d = ImageDraw.Draw(out)

    out.paste(current, (pad, pad + label_h), current)
    out.paste(hires,
              (pad * 2 + current.width, pad + label_h),
              hires)

    # Simple captions (no font — just centred above each)
    d.text((pad + current.width // 2 - 60, pad + 4),
           "64x64 sprite x 2 (current)", fill=(0x3a, 0x2f, 0x24))
    d.text((pad * 2 + current.width + hires.width // 2 - 60, pad + 4),
           "128x128 native (hi-res)", fill=(0x3a, 0x2f, 0x24))

    out.save(OUT)
    print(f"wrote {OUT} ({out_w}x{out_h})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
