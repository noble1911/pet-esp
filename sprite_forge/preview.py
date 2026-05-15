#!/usr/bin/env python3
"""Interactive pet preview (architecture §8).

Composites one pet from a gene vector (PIL composite + gene-indexed
tinting, architecture §5.2) and shows it in a window. Skeleton: argument
parsing and the gene model are here; PIL composition is TODO alongside the
renderer (build-order step 5).
"""

from __future__ import annotations

import argparse
import random
import sys

import forge_common as fc


def random_genes(seed: int | None = None) -> list:
    rng = random.Random(seed)
    return [rng.randrange(m) for m in fc.GENE_MAX]


def compose_pet(genes: list, stage: str):
    """Return a PIL.Image of the composed pet. TODO(build-order:5)."""
    raise NotImplementedError(
        "PIL layer composition + tinting not implemented yet "
        "(build-order step 5)")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="Preview a composed pet")
    ap.add_argument("--stage", default="adult", choices=fc.STAGES)
    ap.add_argument("--genes", help="8 comma-separated gene bytes")
    ap.add_argument("--seed", type=int, help="seed for random genes")
    args = ap.parse_args(argv)

    if args.genes:
        genes = [int(x) for x in args.genes.split(",")]
        if len(genes) != 8:
            ap.error("--genes needs exactly 8 values")
    else:
        genes = random_genes(args.seed)

    print(f"stage={args.stage} genes={genes}")
    try:
        img = compose_pet(genes, args.stage)
        img.show()
    except NotImplementedError as e:
        print(f"skeleton: {e}")
        return 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
