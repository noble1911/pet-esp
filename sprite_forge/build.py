#!/usr/bin/env python3
"""Build all .bin sprite assets into firmware/assets/ (architecture §8).

Skeleton: walks the source tree, validates layout/anchors, and is wired to
emit .bin files via forge_common.write_sprite_bin. PNG -> grayscale+alpha
packing and anchor validation are TODO — fill in alongside the renderer
(build-order step 5).
"""

from __future__ import annotations

import argparse
import sys

import forge_common as fc


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


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="Build .bin sprite assets")
    ap.add_argument("--dry-run", action="store_true",
                    help="report what would be built, write nothing")
    args = ap.parse_args(argv)

    parts = discover_parts()
    if not parts:
        print(f"no source art under {fc.PARTS_DIR} — nothing to build")
        print("expected: parts/{stage}/{layer}/{shape}/{animation}.png")
        return 0

    print(f"output: {fc.ASSETS_DIR}")
    for stage, layer, shape, pngs in parts:
        print(f"  {stage}/{layer}/{shape}: {len(pngs)} animation(s)")
        # TODO(build-order:5): load PNGs, pack grayscale+alpha frames,
        #   validate anchors.json, fc.write_sprite_bin(...).

    if args.dry_run:
        print("dry-run: no files written")
        return 0

    print("TODO: .bin emission not implemented yet (build-order step 5)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
