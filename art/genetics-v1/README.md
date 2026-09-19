# Genetic feature artwork

Production additions to the complete illustrated `sprite-v3` poses, using the
approved `art/pixel-v1/approved-concept.png` as the style reference. Created with
the built-in imagegen tool. Exact prompts are the four sibling `*-prompt.txt`
files; original generated PNGs are kept here unchanged.

- `ears-source.png`: eight paired ears/head tufts, 4 × 2. Sprout gene 0 retains
  the original pose-specific leaves rather than replacing them with this atlas.
- `eyes-source.png`: eight open-eye pairs, 4 × 2, with teal irises recoloured at
  runtime. Original authored closed eyes remain during blinks, happy munches,
  celebrations and naps.
- `mouths-source.png`: eight inherited styles × relaxed, open and chewing rows.
- `markings-source.png`: eight decals, 4 × 2; gene 0 is plain and has no decal.

`python3 scripts/pack_genetics.py` compiles the supplied art to RGB565 with key 0;
`--check` checks reproducibility. It splits cells, preserves genuine alpha,
removes chroma magenta/extraction speckles, and samples to the device grid.
It does not synthesize replacement drawings. The firmware adds 29,568 bytes of
feature pixels, pose anchors and small palettes, with no extra full-size buffer.

Body shapes are continuous scanline profiles of the authored whole action poses,
not eight independently generated characters. This preserves the action art and
supports freely mixing all saved genes within the ESP flash budget. Features
use each pose's face/crown anchors. Food silhouettes, quilt and tub occlude them.
See `docs/genetic-sprites-v1.md` and the production-renderer preview gallery.
