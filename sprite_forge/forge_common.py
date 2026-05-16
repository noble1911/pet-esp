"""Shared helpers for the sprite forge.

Single source of truth (laptop side) for the on-device sprite layout.
Must stay byte-compatible with firmware/components/renderer and
docs/sprite_format.md. Skeleton — composition/tinting is TODO.
"""

from __future__ import annotations

import struct
from pathlib import Path

# Repo paths -------------------------------------------------------------
ROOT = Path(__file__).resolve().parent.parent
PARTS_DIR = ROOT / "sprite_forge" / "parts"
PALETTES_DIR = ROOT / "sprite_forge" / "palettes"
ASSETS_DIR = ROOT / "firmware" / "assets"  # gitignored output

# .bin sprite sheet (docs/sprite_format.md / architecture §5.6) ----------
SPRITE_MAGIC = b"PSPR"

FMT_GRAY_ALPHA = 0   # [gray, alpha], tinted on device
FMT_PALETTIZED = 1
FMT_RGB565 = 2

BPP = {FMT_GRAY_ALPHA: 2, FMT_PALETTIZED: 1, FMT_RGB565: 2}

# Mirrors docs/gene_spec.md and firmware pet_state.h.
STAGES = ["egg", "baby", "child", "teen", "adult", "elder"]
LAYERS = ["body", "tail", "ears", "mouth", "eyes", "pattern", "accessory"]
GENE_MAX = [8, 16, 8, 16, 8, 8, 8, 8]


def pack_sprite_header(width: int, height: int, num_frames: int,
                       fmt: int) -> bytes:
    """Pack the 8-byte sprite sheet header."""
    for n, v in (("width", width), ("height", height),
                 ("num_frames", num_frames)):
        if not 0 < v < 256:
            raise ValueError(f"{n}={v} out of 1..255")
    if fmt not in BPP:
        raise ValueError(f"unknown format {fmt}")
    return SPRITE_MAGIC + struct.pack("<BBBB", width, height, num_frames, fmt)


def write_sprite_bin(path: Path, width: int, height: int,
                     num_frames: int, fmt: int, pixels: bytes) -> None:
    """Write a complete .bin sprite sheet, validating the payload size."""
    expected = width * height * num_frames * BPP[fmt]
    if len(pixels) != expected:
        raise ValueError(
            f"pixel payload {len(pixels)} != expected {expected}")
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as fh:
        fh.write(pack_sprite_header(width, height, num_frames, fmt))
        fh.write(pixels)


# Palette tint table (architecture §5.2, docs/sprite_format.md). A palette
# is PALETTE_ENTRIES gene-indexed columns × PALETTE_RAMP shade rows
# (dark→light). The renderer maps a pixel's gray to a shade row
# (shade = gray * ramp // 256) and looks up RGB565 — no per-pixel math.
PALETTE_MAGIC = b"PPAL"
PALETTE_VERSION = 1
PALETTE_ENTRIES = 16   # body_color / eye_color genes are 0..15
PALETTE_RAMP = 8       # shades per entry


def rgb565(r: int, g: int, b: int) -> int:
    """8-8-8 → 5-6-5, matching the on-device renderer."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def pack_palette_header(entries: int, ramp: int) -> bytes:
    if not 0 < entries < 256 or not 0 < ramp < 256:
        raise ValueError("palette entries/ramp out of 1..255")
    return PALETTE_MAGIC + struct.pack("<BBBB", PALETTE_VERSION,
                                       entries, ramp, 0)


def write_palette_bin(path: Path, ramp_rgb) -> None:
    """Write a "PPAL" table. `ramp_rgb` is PALETTE_ENTRIES rows, each
    PALETTE_RAMP (r,g,b) tuples ordered darkest→lightest. Body is
    entry-major uint16 RGB565, little-endian."""
    if len(ramp_rgb) != PALETTE_ENTRIES:
        raise ValueError(
            f"need {PALETTE_ENTRIES} entries, got {len(ramp_rgb)}")
    body = bytearray()
    for entry in ramp_rgb:
        if len(entry) != PALETTE_RAMP:
            raise ValueError(f"each entry needs {PALETTE_RAMP} shades")
        for (r, g, b) in entry:
            body += struct.pack("<H", rgb565(r, g, b))
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as fh:
        fh.write(pack_palette_header(PALETTE_ENTRIES, PALETTE_RAMP))
        fh.write(bytes(body))
