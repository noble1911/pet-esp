#!/usr/bin/env python3
"""Compose the home scene + a pet at on-device coordinates for visual
review. Mirrors the layout the device renders without flashing:

  - Loads sprite_forge/scenes/home.png (184x152), scales 2x NEAREST to
    fill the 368x304 scene region (matches ui.c::make_scene).
  - Composes a pet via preview.compose_pet(), scales 2x, pastes at the
    on-device pet position (PET_X=120, PET_Y=210 in screen coords).
  - Includes a thin cream stat-strip + action-dock band so the
    composite looks like the full screen layout.

Output: docs/previews/scene_home_with_pet.png — committable, viewable
on GitHub.
"""
from __future__ import annotations
import sys
from pathlib import Path

try:
    from PIL import Image
except ModuleNotFoundError:
    sys.exit("Pillow not installed — run: pip install -r requirements.txt")

ROOT = Path(__file__).resolve().parent.parent
SCENE_PNG = ROOT / "sprite_forge" / "scenes" / "home.png"
OUT = ROOT / "docs" / "previews" / "scene_home_with_pet.png"

# Mirror constants from firmware/components/ui/ui.c.
SCREEN_W       = 368
SCREEN_H       = 448
STAT_STRIP_H   = 56
ACTION_DOCK_H  = 88
SCENE_TOP_Y    = STAT_STRIP_H
SCENE_BOTTOM_Y = SCREEN_H - ACTION_DOCK_H
PET_X          = 120
PET_Y          = 210
CREAM          = (0xfd, 0xf6, 0xe3)
WOOD_DARK      = (0x7a, 0x4f, 0x30)


def compose() -> Image.Image:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from preview import compose_pet, random_genes   # noqa: E402

    # Build the full screen canvas. Stat strip + action dock get cream
    # placeholder rectangles — real device draws icons + buttons in those
    # bands but we just want to show the scene context.
    screen = Image.new("RGB", (SCREEN_W, SCREEN_H), CREAM)

    # Stat strip (top band) — cream fill, stays empty here.
    # Action dock (bottom band) — cream fill, stays empty.

    # Scene background — load native, NEAREST 2x to fill the scene area.
    scene = Image.open(SCENE_PNG).convert("RGB")
    sw, sh = scene.size
    scene2x = scene.resize((sw * 2, sh * 2), Image.Resampling.NEAREST)
    screen.paste(scene2x, (0, SCENE_TOP_Y))

    # Compose a representative pet at full device scale. Pet sprite is
    # 64x64; the device scales it 2x to 128x128 around its centre pivot.
    # Use a fixed, expressive gene roll so the preview is reproducible.
    genes = [3, 8, 2, 8, 2, 3, 2, 1]   # pear body, sleepy eyes, smile,
                                       # pointy ears, short tail, spots
    pet_native = compose_pet(genes, "baby")
    pw, ph = pet_native.size
    pet2x = pet_native.resize((pw * 2, ph * 2), Image.Resampling.NEAREST)

    # LVGL's centre-pivot scaling: the visible 128x128 extends ±64 around
    # the object's (PET_X, PET_Y). Top-left of the visible pet on screen:
    paste_x = PET_X - (pw // 2)
    paste_y = PET_Y - (ph // 2)
    # Convert pet to RGBA so we can paste it with its alpha mask onto
    # the (otherwise opaque RGB) screen.
    pet_rgba = pet2x.convert("RGBA")
    screen.paste(pet_rgba, (paste_x, paste_y), pet_rgba)

    # Faint horizontal dividers showing the band boundaries — helps you
    # see what's stat-strip / scene / action-dock at a glance. Same
    # wood-dark as the floor edge for thematic consistency.
    pix = screen.load()
    for x in range(SCREEN_W):
        pix[x, STAT_STRIP_H - 1]    = WOOD_DARK
        pix[x, SCENE_BOTTOM_Y]      = WOOD_DARK

    return screen


def main() -> int:
    img = compose()
    OUT.parent.mkdir(parents=True, exist_ok=True)
    img.save(OUT)
    print(f"wrote {OUT} ({img.width}x{img.height})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
