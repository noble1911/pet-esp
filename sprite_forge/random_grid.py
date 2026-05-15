#!/usr/bin/env python3
"""Save a 4x4 PNG of random pets for quick visual review (architecture §8).

Skeleton: builds the random gene set and grid plan; rendering each cell
reuses preview.compose_pet, which is TODO (build-order step 5).
"""

from __future__ import annotations

import argparse
import sys

import forge_common as fc
from preview import compose_pet, random_genes


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="4x4 grid of random pets")
    ap.add_argument("--stage", default="adult", choices=fc.STAGES)
    ap.add_argument("--out", default="random_grid.png")
    ap.add_argument("--seed", type=int, default=0)
    args = ap.parse_args(argv)

    genes = [random_genes(args.seed + i) for i in range(16)]
    print(f"stage={args.stage} -> 4x4 grid -> {args.out}")
    for i, g in enumerate(genes):
        print(f"  [{i:2d}] {g}")

    try:
        # TODO(build-order:5): paste each compose_pet() onto a 4x4 canvas
        #   and save to args.out.
        compose_pet(genes[0], args.stage)
    except NotImplementedError as e:
        print(f"skeleton: {e}")
        return 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
