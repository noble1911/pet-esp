#!/usr/bin/env python3
"""Compose a pet from .bin assets without a device — fast feedback loop.

This is the Python mirror of `firmware/components/renderer/renderer.c`'s
`compose_pet()`. Both read the SAME `.bin` files produced by build.py
and the SAME palettes from `.pal`, so the output here should be
pixel-equivalent to what the device draws (modulo float rounding in
alpha blends — negligible at our colour depth).

Use this for fast iteration on art and per-body anchors — render the
PNG, eyeball it, adjust, re-render. No flash required.

Examples:
  python3 sprite_forge/preview.py                   # random pet
  python3 sprite_forge/preview.py --seed 42         # reproducible roll
  python3 sprite_forge/preview.py --genes 1,0,2,5,1,2,0,0 --stage baby
  python3 sprite_forge/preview.py --scale 6 --out /tmp/pet.png

architecture §5 / §8.
"""

from __future__ import annotations

import argparse
import random
import struct
import sys
from pathlib import Path

try:
    from PIL import Image
except ModuleNotFoundError:
    sys.exit("Pillow not installed — run: pip install -r requirements.txt")

import forge_common as fc

ROOT = Path(__file__).resolve().parent.parent
ASSETS_DIR = ROOT / "firmware" / "assets"

# Mirror firmware/components/renderer/include/renderer.h enums + layer dirs.
LAYER_BODY      = 0
LAYER_TAIL      = 1
LAYER_EARS      = 2
LAYER_MOUTH     = 3
LAYER_EYES      = 4
LAYER_PATTERN   = 5
LAYER_ACCESSORY = 6
LAYER_COUNT     = 7

LAYER_DIRS  = ["body", "tail", "ears", "mouth", "eyes", "pattern", "accessory"]
LAYER_ANIMS = ["idle", "idle", "idle", "neutral", "normal", "idle", "idle"]
LAYER_NAMES = LAYER_DIRS   # anchors.bin uses these keys for indexing

# Gene byte indices (docs/gene_spec.md).
GENE_BODY_SHAPE  = 0
GENE_BODY_COLOR  = 1
GENE_EYE_SHAPE   = 2
GENE_EYE_COLOR   = 3
GENE_EAR_SHAPE   = 4
GENE_MOUTH_SHAPE = 5
GENE_PATTERN     = 6
GENE_PERSONALITY = 7


def random_genes(seed: int | None = None) -> list:
    rng = random.Random(seed)
    return [rng.randrange(m) for m in fc.GENE_MAX]


# ---- File loaders (match the C parsers in renderer.c byte-for-byte) -------

def _load_sprite(path: Path) -> dict:
    """Parse a PSPR .bin sprite sheet (header + frame 0 only). The device
    cycles frames at ~5 fps for animation; we just want a static snapshot."""
    data = path.read_bytes()
    if data[:4] != b"PSPR":
        raise RuntimeError(f"{path}: not a PSPR sheet")
    width, height, num_frames, fmt = struct.unpack("<BBBB", data[4:8])
    bpp = {0: 2, 1: 1, 2: 2}.get(fmt)
    if bpp is None:
        raise RuntimeError(f"{path}: unknown format {fmt}")
    frame_size = width * height * bpp
    pixels = data[8:8 + frame_size]
    return {"w": width, "h": height, "fmt": fmt, "bpp": bpp, "pixels": pixels}


def _load_anchors(path: Path) -> dict:
    """Decode anchors.bin (PANC v1 with 5 entries or v2 with 6).
    Returns dict keyed by layer name -> (x, y)."""
    data = path.read_bytes()
    if data[:4] != b"PANC":
        raise RuntimeError(f"{path}: not a PANC file")
    ver, cnt = data[4], data[5]
    keys = ("eyes", "mouth", "ears", "tail", "accessory")
    if ver == 2 and cnt == 6:
        keys = keys + ("pattern",)
    elif not (ver == 1 and cnt == 5):
        raise RuntimeError(f"{path}: bad PANC header v{ver} cnt{cnt}")
    pts = struct.unpack_from(f"<{cnt * 2}h", data, 6)
    out = {k: (pts[i * 2], pts[i * 2 + 1]) for i, k in enumerate(keys)}
    out.setdefault("pattern", (0, 0))   # v1 default, matches renderer.c
    return out


def _load_palette(path: Path) -> list:
    """Decode a PPAL palette into a 2D table [entry][shade] -> (r, g, b).
    RGB565 -> RGB888 expansion matches renderer.c's bit-replicate."""
    data = path.read_bytes()
    if data[:4] != b"PPAL":
        raise RuntimeError(f"{path}: not a PPAL file")
    entries, ramp = data[5], data[6]
    table = []
    off = 8
    for _ in range(entries):
        row = []
        for _s in range(ramp):
            (rgb565,) = struct.unpack_from("<H", data, off)
            off += 2
            r = (rgb565 >> 8) & 0xF8
            r |= r >> 5
            g = (rgb565 >> 3) & 0xFC
            g |= g >> 6
            b = (rgb565 << 3) & 0xF8
            b |= b >> 5
            row.append((r, g, b))
        table.append(row)
    return table


# ---- Shape resolution (mirrors renderer.c::resolve_shape) -----------------

def _resolve_shape(layer_base: Path, gene: int) -> str | None:
    if not layer_base.exists():
        return None
    shapes = sorted(p.name for p in layer_base.iterdir() if p.is_dir())
    if not shapes:
        return None
    return shapes[gene % len(shapes)]


def _resolve_sprite_file(shape_dir: Path, anim: str) -> Path | None:
    """`{anim}.bin` if present, else alphabetically first *.bin
    (excluding anchors.bin) — matches renderer.c's fallback."""
    candidate = shape_dir / f"{anim}.bin"
    if candidate.exists():
        return candidate
    bins = sorted(p for p in shape_dir.glob("*.bin")
                  if p.name != "anchors.bin")
    return bins[0] if bins else None


# ---- Tinting (mirrors renderer.c::tint_for + renderer_tint_rgb565) --------

def _gray_to_rgb(gray: int) -> tuple[int, int, int]:
    """Untinted layer pixel: gray -> RGB565 -> RGB888 round-trip so the
    output matches the panel's quantised colour exactly."""
    rgb565 = ((gray & 0xF8) << 8) | ((gray & 0xFC) << 3) | (gray >> 3)
    r = (rgb565 >> 8) & 0xF8; r |= r >> 5
    g = (rgb565 >> 3) & 0xFC; g |= g >> 6
    b = (rgb565 << 3) & 0xF8; b |= b >> 5
    return (r, g, b)


def _tint(palette: list, entry: int, gray: int) -> tuple[int, int, int]:
    ramp = len(palette[0])
    entries = len(palette)
    shade = min(ramp - 1, (gray * ramp) >> 8)
    return palette[entry % entries][shade]


def _gene_value(layer: int, genes: list) -> int:
    return {
        LAYER_BODY:      genes[GENE_BODY_SHAPE],
        LAYER_TAIL:      genes[GENE_BODY_SHAPE],    # tail follows body
        LAYER_EARS:      genes[GENE_EAR_SHAPE],
        LAYER_MOUTH:     genes[GENE_MOUTH_SHAPE],
        LAYER_EYES:      genes[GENE_EYE_SHAPE],
        LAYER_PATTERN:   genes[GENE_PATTERN],
        LAYER_ACCESSORY: 0,
    }[layer]


def _tint_for(layer: int, genes: list, pal_body, pal_eye):
    """(palette_or_None, entry) per renderer.c::tint_for."""
    if layer == LAYER_EYES:
        return pal_eye, genes[GENE_EYE_COLOR]
    if layer in (LAYER_BODY, LAYER_TAIL, LAYER_EARS, LAYER_PATTERN):
        return pal_body, genes[GENE_BODY_COLOR]
    return None, 0   # mouth, accessory — untinted


# ---- Composition ----------------------------------------------------------

def compose_pet(genes: list, stage: str = "baby") -> Image.Image:
    """Render a pet from a gene vector. Returns a PIL RGBA image at the
    body sprite's native size (typically 64×64).

    Mirrors firmware/components/renderer/renderer.c::compose_pet():
      - Body first (defines canvas size + the body-alpha mask for pattern).
      - Iterate layers back-to-front, src-over alpha blend onto canvas.
      - Pattern layer is gated by body alpha at the destination pixel.
      - Per-body anchors shift each layer by (anchor.x, anchor.y).
    """
    stage_dir = ASSETS_DIR / stage
    if not stage_dir.exists():
        raise RuntimeError(f"missing stage dir {stage_dir} — run build.py first")

    body_layer_base = stage_dir / LAYER_DIRS[LAYER_BODY]
    body_shape = _resolve_shape(body_layer_base, _gene_value(LAYER_BODY, genes))
    if body_shape is None:
        raise RuntimeError(f"no body shape for stage {stage}")
    body_shape_dir = body_layer_base / body_shape
    body_path = _resolve_sprite_file(body_shape_dir, LAYER_ANIMS[LAYER_BODY])
    body = _load_sprite(body_path)

    anchors_path = body_shape_dir / "anchors.bin"
    anchors = _load_anchors(anchors_path) if anchors_path.exists() else {
        k: (0, 0) for k in ("eyes", "mouth", "ears", "tail", "accessory", "pattern")
    }

    pal_body = _load_palette(ASSETS_DIR / "palettes" / "body.pal")
    pal_eye  = _load_palette(ASSETS_DIR / "palettes" / "eye.pal")

    W, H = body["w"], body["h"]
    canvas = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    body_alpha = body["pixels"]   # bytes; alpha at index*2 + 1

    def body_alpha_at(cx: int, cy: int) -> int:
        if cx < 0 or cx >= W or cy < 0 or cy >= H:
            return 0
        return body_alpha[(cy * W + cx) * 2 + 1]

    for L in range(LAYER_COUNT):
        layer_base = stage_dir / LAYER_DIRS[L]
        gene = _gene_value(L, genes)
        shape = _resolve_shape(layer_base, gene)
        if shape is None:
            continue
        sprite_path = _resolve_sprite_file(layer_base / shape, LAYER_ANIMS[L])
        if sprite_path is None:
            continue
        sp = _load_sprite(sprite_path)
        ox, oy = (0, 0) if L == LAYER_BODY else anchors.get(LAYER_DIRS[L], (0, 0))
        pal, entry = _tint_for(L, genes, pal_body, pal_eye)

        # Build a per-layer RGBA buffer at canvas size, then alpha-composite
        # onto the running canvas. Slower than poking the canvas directly
        # per pixel, but PIL's alpha_composite handles src-over correctly
        # without us hand-rolling the math the renderer does.
        layer_img = Image.new("RGBA", (W, H), (0, 0, 0, 0))
        layer_px = layer_img.load()
        sw, sh = sp["w"], sp["h"]
        src = sp["pixels"]
        for yy in range(sh):
            cy = oy + yy
            if cy < 0 or cy >= H:
                continue
            for xx in range(sw):
                cx = ox + xx
                if cx < 0 or cx >= W:
                    continue
                idx = (yy * sw + xx) * 2
                gray, a = src[idx], src[idx + 1]
                if a == 0:
                    continue
                if L == LAYER_PATTERN and body_alpha_at(cx, cy) == 0:
                    continue
                if pal:
                    r, g, b = _tint(pal, entry, gray)
                else:
                    r, g, b = _gray_to_rgb(gray)
                layer_px[cx, cy] = (r, g, b, a)
        canvas = Image.alpha_composite(canvas, layer_img)

    return canvas


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--stage", default="baby", choices=fc.STAGES,
                    help="life stage (default: baby — only stage with art today)")
    ap.add_argument("--genes",
                    help='8 comma-separated gene bytes ("1,5,2,0,1,2,0,3"). '
                         'Defaults to a random roll.')
    ap.add_argument("--seed", type=int,
                    help="random seed (ignored if --genes is given)")
    ap.add_argument("--scale", type=int, default=4,
                    help="integer NEAREST upscale of the output (default 4 → ~256 px)")
    ap.add_argument("--out", default="/tmp/pet_preview.png",
                    help="output PNG path (default /tmp/pet_preview.png)")
    args = ap.parse_args(argv)

    if args.genes:
        genes = [int(b) for b in args.genes.split(",")]
        if len(genes) != 8:
            ap.error("--genes needs exactly 8 values")
    else:
        genes = random_genes(args.seed)

    print(f"stage={args.stage} genes={genes}")
    img = compose_pet(genes, args.stage)
    if args.scale > 1:
        img = img.resize((img.width * args.scale, img.height * args.scale),
                         Image.Resampling.NEAREST)
    img.save(args.out)
    print(f"wrote {args.out} ({img.width}×{img.height})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
