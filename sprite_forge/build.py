#!/usr/bin/env python3
"""Build all .bin sprite assets into firmware/assets/ (architecture §8, build-order step 5).

Walks the source art tree, slices each animation strip into frames, packs
them to the on-device `.bin` format (docs/sprite_format.md, magic "PSPR"),
validates body anchors and emits the binary anchor sidecar ("PANC"), and
round-trip verifies every file it writes so the output stays byte-compatible
with firmware/components/renderer. The built tree is shipped to the device in
a flash LittleFS partition (architecture §2/§5; SD deferred).

Source PNG convention (sprite_forge owns this schema — the architecture left
it undefined): one animation = a **horizontal strip of square frames**.
`frame_size = image_height`, `num_frames = image_width // image_height`.
Width must be an exact multiple of height. This is exactly what Piskel's
sprite-sheet export (and piskel-mcp) produces.

Art is authored grayscale + alpha; colour is applied on-device from
gene-indexed palettes, so RGB is collapsed to luma here and any colour the
artist left in is intentionally discarded (format 0 = [gray, alpha]).
"""

from __future__ import annotations

import argparse
import colorsys
import json
import struct
import sys
from pathlib import Path

import forge_common as fc

try:
    from PIL import Image
except ModuleNotFoundError:
    sys.exit("Pillow not installed — run: pip install -r requirements.txt")

# Anchor keys a body shape must define, in the canonical on-device order
# (docs/sprite_format.md "Anchors"). This order IS the binary layout.
# Order matters — this IS the binary layout of anchors.bin. `pattern`
# was added in schema/binary version 2; older anchors.json files that
# omit it (and read in the legacy [x, y] array form) default to (0, 0)
# for pattern, which is harmless because the renderer masks pattern
# writes to body alpha automatically.
ANCHOR_KEYS = ("eyes", "mouth", "ears", "tail", "accessory", "pattern")
ANCHOR_MAGIC = b"PANC"
ANCHOR_VERSION = 2


class BuildError(Exception):
    """A source-art problem that should fail the build with a clear message."""


def _luma(r: int, g: int, b: int) -> int:
    """Rec.601 luma. Robust even if the artist left slight colour in."""
    return (r * 299 + g * 587 + b * 114) // 1000


def pack_png(path: Path) -> tuple[int, int, int, bytes]:
    """Slice a horizontal frame strip and pack it to format-0 bytes.

    Returns (frame_w, frame_h, num_frames, pixels) where pixels is
    [gray, alpha] per pixel, frame 0 first, row-major within a frame.
    """
    img = Image.open(path).convert("RGBA")
    iw, ih = img.size
    if ih == 0 or iw == 0:
        raise BuildError(f"{path}: empty image")
    if iw % ih != 0:
        raise BuildError(
            f"{path}: width {iw} is not a multiple of height {ih} — "
            f"expected a horizontal strip of {ih}x{ih} frames")
    fw = fh = ih
    nframes = iw // ih
    for name, val in (("frame size", fw), ("num_frames", nframes)):
        if not 0 < val < 256:
            raise BuildError(f"{path}: {name}={val} out of 1..255 "
                             f"(header field is one byte)")

    rgba = img.tobytes()  # iw*ih*4, row-major
    out = bytearray(fw * fh * nframes * 2)
    o = 0
    for f in range(nframes):
        for y in range(fh):
            row = (y * iw + f * fw) * 4
            for x in range(fw):
                i = row + x * 4
                out[o] = _luma(rgba[i], rgba[i + 1], rgba[i + 2])
                out[o + 1] = rgba[i + 3]
                o += 2
    return fw, fh, nframes, bytes(out)


def _parse_anchor_point(path: Path, key: str, value) -> tuple[int, int]:
    """Accept either the legacy `[x, y]` array form OR the named-object
    `{"x": int, "y": int}` form (docs/sprite_style_guide.md §4). Both
    return a plain (int, int) tuple.
    """
    if isinstance(value, list):
        if len(value) != 2 or not all(isinstance(c, int) for c in value):
            raise BuildError(
                f"{path}: anchor '{key}' array form must be [int x, int y]")
        return value[0], value[1]
    if isinstance(value, dict):
        if "x" not in value or "y" not in value:
            raise BuildError(
                f"{path}: anchor '{key}' object form needs 'x' and 'y' keys")
        if (not isinstance(value["x"], int)
                or not isinstance(value["y"], int)):
            raise BuildError(
                f"{path}: anchor '{key}' x/y must be ints")
        return value["x"], value["y"]
    raise BuildError(
        f"{path}: anchor '{key}' must be [x, y] or {{x, y}}")


def validate_anchors(path: Path, frame_w: int, frame_h: int) -> dict:
    """Load + validate a body shape's anchors.json (the human-authored source).

    `pattern` is optional — if a legacy file omits it, default to (0, 0).
    The renderer masks pattern writes to body alpha so a zero anchor is
    a sensible no-op (style guide §4).
    """
    if not path.exists():
        raise BuildError(f"missing {path} — body shapes require anchors.json "
                         f"(docs/sprite_format.md, docs/sprite_style_guide.md)")
    try:
        data = json.loads(path.read_text())
    except json.JSONDecodeError as e:
        raise BuildError(f"{path}: invalid JSON ({e})")
    out: dict[str, tuple[int, int]] = {}
    for key in ANCHOR_KEYS:
        if key in data:
            x, y = _parse_anchor_point(path, key, data[key])
        elif key == "pattern":
            # Legacy files (pre-v2) omit pattern — default zero.
            x, y = 0, 0
        else:
            raise BuildError(f"{path}: missing anchor '{key}'")
        # Anchors can legitimately be negative (shift LEFT/UP) to push a
        # layer drawn at canonical position onto a body whose face sits
        # off-centre. Range bounds are int16, matching the binary format.
        if not (-32768 <= x <= 32767 and -32768 <= y <= 32767):
            raise BuildError(f"{path}: anchor '{key}' ({x},{y}) exceeds int16")
        # Soft sanity: an anchor more than a frame away is almost certainly
        # a typo. Warn but don't fail — leave room for unusual layouts.
        if abs(x) > frame_w or abs(y) > frame_h:
            print(f"  warn: {path} anchor '{key}' ({x},{y}) > frame "
                  f"{frame_w}x{frame_h} — typo?")
        out[key] = (x, y)
    return out


def write_anchors_bin(path: Path, anchors: dict) -> None:
    """Emit the on-device binary anchor sidecar (docs/sprite_format.md).

    Layout: "PANC", u8 version, u8 count, then count*(int16 x, int16 y)
    little-endian in ANCHOR_KEYS order. No on-device JSON parser needed.
    """
    buf = bytearray(ANCHOR_MAGIC)
    buf += struct.pack("<BB", ANCHOR_VERSION, len(ANCHOR_KEYS))
    for key in ANCHOR_KEYS:
        x, y = anchors[key]
        buf += struct.pack("<hh", int(x), int(y))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(bytes(buf))
    rb = path.read_bytes()
    expected = 6 + 4 * len(ANCHOR_KEYS)
    if (rb[:4] != ANCHOR_MAGIC or rb[4] != ANCHOR_VERSION
            or rb[5] != len(ANCHOR_KEYS) or len(rb) != expected):
        raise BuildError(f"{path}: anchors.bin round-trip mismatch")


def verify_bin(path: Path, fw: int, fh: int, nf: int) -> None:
    """Re-read a written .bin and assert it matches what we intended."""
    b = path.read_bytes()
    if b[:4] != fc.SPRITE_MAGIC:
        raise BuildError(f"{path}: bad magic {b[:4]!r}")
    w, h, n, fmt = b[4], b[5], b[6], b[7]
    expected = 8 + fw * fh * nf * fc.BPP[fc.FMT_GRAY_ALPHA]
    if (w, h, n, fmt) != (fw, fh, nf, fc.FMT_GRAY_ALPHA) or len(b) != expected:
        raise BuildError(
            f"{path}: round-trip mismatch: header={(w, h, n, fmt)} "
            f"len={len(b)} expected={(fw, fh, nf, fc.FMT_GRAY_ALPHA)}/{expected}")


def discover_parts() -> list:
    """Return [(stage, layer, shape, [animation.png ...])] from parts/."""
    found = []
    if not fc.PARTS_DIR.exists():
        return found
    for stage_dir in sorted(p for p in fc.PARTS_DIR.iterdir() if p.is_dir()):
        for layer_dir in sorted(p for p in stage_dir.iterdir() if p.is_dir()):
            for shape_dir in sorted(p for p in layer_dir.iterdir()
                                    if p.is_dir()):
                pngs = sorted(shape_dir.glob("*.png"))
                found.append((stage_dir.name, layer_dir.name,
                              shape_dir.name, pngs))
    return found


PALETTE_NAMES = ("body", "eye")


def _default_palette() -> list:
    """16 evenly-spaced hues, each an 8-step dark→light ramp. Placeholder
    until palettes/{name}.png are hand-authored."""
    rows = []
    for i in range(fc.PALETTE_ENTRIES):
        hue = i / fc.PALETTE_ENTRIES
        ramp = []
        for s in range(fc.PALETTE_RAMP):
            t = s / (fc.PALETTE_RAMP - 1)
            val = 0.22 + 0.74 * t           # darkest → lightest
            sat = 0.85 - 0.30 * t           # ease saturation in highlights
            r, g, b = colorsys.hsv_to_rgb(hue, sat, val)
            ramp.append((round(r * 255), round(g * 255), round(b * 255)))
        rows.append(ramp)
    return rows


def _palette_from_png(path: Path) -> list:
    """PNG schema: PALETTE_ENTRIES wide (gene index) × PALETTE_RAMP tall
    (row 0 darkest → last lightest)."""
    img = Image.open(path).convert("RGB")
    w, h = img.size
    if (w, h) != (fc.PALETTE_ENTRIES, fc.PALETTE_RAMP):
        raise BuildError(
            f"{path}: palette must be {fc.PALETTE_ENTRIES}x{fc.PALETTE_RAMP} "
            f"(entries × shades dark→light), got {w}x{h}")
    px = img.load()
    return [[px[x, y] for y in range(h)] for x in range(w)]


def build_palettes(dry_run: bool) -> tuple:
    """Pack palettes/{name}.png (or a generated default) → assets/palettes/
    {name}.pal. Returns (built, errors)."""
    built = errors = 0
    for name in PALETTE_NAMES:
        src = fc.PALETTES_DIR / f"{name}.png"
        out = fc.ASSETS_DIR / "palettes" / f"{name}.pal"
        try:
            if src.exists():
                rows = _palette_from_png(src)
                origin = src.name
            else:
                rows = _default_palette()
                origin = "generated default (no PNG)"
            size = 8 + fc.PALETTE_ENTRIES * fc.PALETTE_RAMP * 2
            if dry_run:
                print(f"  palette {name}: {origin} -> {size} B [dry-run]")
            else:
                fc.write_palette_bin(out, rows)
                print(f"  palette {name}: {origin} -> {out.name} "
                      f"({out.stat().st_size} B) ok")
                built += 1
        except (BuildError, ValueError, OSError) as e:
            print(f"  ! palette {name}: {e}")
            errors += 1
    return built, errors


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="Build .bin sprite assets")
    ap.add_argument("--dry-run", action="store_true",
                    help="report what would be built, write nothing")
    args = ap.parse_args(argv)

    parts = discover_parts()
    print(f"output: {fc.ASSETS_DIR}")
    built = errors = 0

    pbuilt, perrors = build_palettes(args.dry_run)
    built += pbuilt
    errors += perrors

    if not parts:
        print(f"no part art under {fc.PARTS_DIR} "
              f"(expected parts/{{stage}}/{{layer}}/{{shape}}/{{anim}}.png)")
    for stage, layer, shape, pngs in parts:
        if stage not in fc.STAGES:
            print(f"  ! unknown stage '{stage}' (see forge_common.STAGES)")
            errors += 1
            continue
        if layer not in fc.LAYERS:
            print(f"  ! unknown layer '{layer}' (see forge_common.LAYERS)")
            errors += 1
            continue
        shape_dir = fc.PARTS_DIR / stage / layer / shape
        out_dir = fc.ASSETS_DIR / stage / layer / shape
        for png in pngs:
            rel = f"{stage}/{layer}/{shape}/{png.stem}"
            try:
                fw, fh, nf, pixels = pack_png(png)
                anchors = None
                if layer == "body":
                    anchors = validate_anchors(
                        shape_dir / "anchors.json", fw, fh)
                if args.dry_run:
                    extra = " +anchors.bin" if anchors is not None else ""
                    print(f"  {rel}: {fw}x{fh} x{nf}f -> "
                          f"{len(pixels) + 8} bytes{extra} [dry-run]")
                    continue
                out_bin = out_dir / f"{png.stem}.bin"
                fc.write_sprite_bin(out_bin, fw, fh, nf,
                                    fc.FMT_GRAY_ALPHA, pixels)
                verify_bin(out_bin, fw, fh, nf)
                extra = ""
                if anchors is not None:
                    write_anchors_bin(out_dir / "anchors.bin", anchors)
                    extra = " +anchors.bin"
                print(f"  {rel}: {fw}x{fh} x{nf}f -> {out_bin.name} "
                      f"({out_bin.stat().st_size} B){extra} ok")
                built += 1
            except (BuildError, ValueError) as e:
                print(f"  ! {rel}: {e}")
                errors += 1

    print(f"\n{'dry-run: ' if args.dry_run else ''}"
          f"{built} built, {errors} error(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
