#!/usr/bin/env python3
"""Render an N×N grid of random pets to one PNG for variety review.

Uses preview.compose_pet, which mirrors the device renderer exactly —
so this grid is what you'd see if you rolled the same gene vectors on
the device. Use it after touching art or anchors to scan for outliers
without flashing.

Examples:
  python3 sprite_forge/random_grid.py
  python3 sprite_forge/random_grid.py --grid 6 --seed 7 --scale 4
  python3 sprite_forge/random_grid.py --out /tmp/before.png
"""
from __future__ import annotations

import argparse
import sys

try:
    from PIL import Image, ImageDraw
except ModuleNotFoundError:
    sys.exit("Pillow not installed — run: pip install -r requirements.txt")

import forge_common as fc
from preview import compose_pet, random_genes


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--stage", default="baby", choices=fc.STAGES)
    ap.add_argument("--grid", type=int, default=4,
                    help="grid edge length (4 → 4×4 = 16 pets, default)")
    ap.add_argument("--scale", type=int, default=3,
                    help="integer NEAREST upscale per cell (default 3 → 192-px cells)")
    ap.add_argument("--seed", type=int, default=0,
                    help="base seed; cell N uses seed+N for reproducibility")
    ap.add_argument("--out", default="/tmp/random_grid.png")
    ap.add_argument("--bg", default="#fdf6e3",
                    help="grid background colour (default cream — matches scene)")
    args = ap.parse_args(argv)

    n = args.grid * args.grid
    genes_set = [random_genes(args.seed + i) for i in range(n)]

    # Compose first cell to learn the per-cell size (assumes uniform).
    first = compose_pet(genes_set[0], args.stage)
    cw, ch = first.width * args.scale, first.height * args.scale

    pad = 4   # inter-cell padding
    grid_w = args.grid * cw + (args.grid + 1) * pad
    grid_h = args.grid * ch + (args.grid + 1) * pad
    out = Image.new("RGBA", (grid_w, grid_h), args.bg)
    draw = ImageDraw.Draw(out)

    print(f"stage={args.stage} grid={args.grid}x{args.grid} → {args.out}")
    for i, g in enumerate(genes_set):
        row, col = divmod(i, args.grid)
        x = pad + col * (cw + pad)
        y = pad + row * (ch + pad)
        img = compose_pet(g, args.stage)
        if args.scale > 1:
            img = img.resize((cw, ch), Image.Resampling.NEAREST)
        out.paste(img, (x, y), img)
        # Faint cell border so we can see where each pet's frame is —
        # helps spot composition overflow / ear-floating issues at a glance.
        draw.rectangle([x, y, x + cw - 1, y + ch - 1],
                       outline="#e8d5a0", width=1)
        print(f"  [{i:2d}] {g}")

    out.save(args.out)
    print(f"wrote {args.out} ({grid_w}×{grid_h})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
