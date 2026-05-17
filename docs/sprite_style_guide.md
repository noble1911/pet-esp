# Sprite style guide

Canonical conventions for drawing pet part sprites. Following these means
the renderer composes them correctly across body shapes without
per-shape hand-tuning. Companion to [architecture.md §5](architecture.md)
and [sprite_format.md](sprite_format.md), which cover formats and the
layer stack — this doc covers *where pixels go inside each frame*.

---

## 1. The 64×64 frame is the universe

All part sprites live in a 64×64 frame (or 128×64 for a 2-frame strip,
192×64 for 3, etc — `width = frame_size × num_frames`). The renderer
composes by adding each layer's body-anchor to the layer's pixel
coordinates: pixel `(x, y)` in the layer sprite lands at body-coord
`(x + anchor.x, y + anchor.y)` on the composed canvas.

Body sprites are drawn at canvas position `(0, 0)` — they ARE the canvas.
Every other layer is positioned via the body's per-layer anchor.

## 2. Canonical grid (the "design target")

Every layer sprite is drawn assuming a body whose face is at this
position. Per-body anchors then *shift* the layer to match a body shape
whose face actually lands somewhere else.

```
y =  4  ─── ear tips (top of pointy ears)
y = 13  ─── ear attach line (bottom of ears = top of head)
y = 14  ─── head top
y = 20  ─── face center / eye line
y = 28  ─── mouth line
y = 32  ─── body center (also frame center)
y = 36  ─── tail attach line
y = 44  ─── body bottom (above floor shadow)
y = 48  ─── frame bottom-safe (avoid below — floor scaling pivot)

         x =  6  ─ frame-left safe
         x = 22  ─ eye left   (pupil center)
         x = 32  ─ face / body center column
         x = 42  ─ eye right  (pupil center)
         x = 50  ─ tail attach column (body right edge)
         x = 58  ─ frame-right safe
```

**TL;DR:** if you draw to these row/column conventions on a new sprite,
it will compose correctly out of the box with any body shape that
declares anchors in body-coords (see §4).

## 3. Per-layer conventions

### Body
- Visible content centred at `(32, 32)`. The "head bulge" (where the
  face goes) sits at `y = [14, 28]`. The "lower body" (where the floor
  shadow + legs go) sits at `y = [32, 48]`.
- 2-frame idle strip is the norm: frame 0 = neutral pose, frame 1 =
  subtle deformation (1-2 px bottom-shadow shift, slight squash). The
  renderer cycles at ~5 fps.
- Draw silhouette in luma greys: `#a0a0a0` main body, `#707070` shadow.
  The renderer tints via the body palette at draw time.
- Edges of the silhouette MUST be opaque enough to act as a pattern
  mask — pattern decoration is gated by body alpha (see §3.6).

### Eyes
- Left pupil centre at `(22, 20)`, right pupil at `(42, 20)`. 20-px
  inter-pupil distance.
- Each eye sprite MUST include its own white sclera (`#ffffff`)
  around each pupil — this is the single most common art bug
  ("missing sclera"). Sclera convention: 5×5 white oval centred on
  each pupil position, BEFORE the pupil is drawn over it.
- Pupil: black (`#000000`) for `round/dot`; thin horizontal black bar
  through sclera for `sleepy`. Variant character lives in pupil shape;
  sclera stays consistent.
- Optional eye colour shows where the sclera is grey — keep sclera
  pure white and pupil pure black to bypass the eye-colour palette.
  The palette only tints grey pixels.

### Mouth
- Centred at `(32, 28)`. Mouth shapes are tiny (6-8 px wide, 1-2 px
  tall) so they read as expressions, not as separate features.
- Draw in dark grey `#303030`. Untinted at runtime.
- `neutral` = horizontal line. `smile` = curve up. `frown` = curve down.

### Ears
- Drawn as a PAIR, one on each side of the head. Left ear centred
  around column `x=16`, right around `x=48`. Inter-ear spacing 32 px.
- ALL ear shapes' bottoms at `y=13` (the ear-attach line). Ears extend
  UPWARD from there.
  - Pointy: tip at `y=4`, base at `y=13`.
  - Round: half-disc above `y=13`, top at `y=5`.
  - Floppy: this is the exception — extends DOWNWARD past the head
    (rabbit-style). Tops at `y=13`, hanging to `y=26`. Per-body anchor
    offset cannot perfectly fit both pointy AND floppy on the same
    body; floppy on a low-headed body will droop into the body.
    Considered acceptable for now; redesign as "ear stops just above
    eyes" if it becomes a problem.

### Tail
- Attach line at column `x=50` (right side of body), centred around
  `y=32`. Tails extend RIGHTWARD/UPWARD from there.
- Short: stub at `x=[44, 50]`. Long: curling line from attach. Puff:
  small ball at `x=[46, 54]`.
- All shapes attach at the same column so per-body anchors can shift
  the tail laterally as a unit.

### Pattern
- Pattern is body-tinted (uses body_color palette) AND **automatically
  masked by body alpha** in the renderer's compositor. You can draw a
  pattern as a freeform field of pixels — it will only render where
  the body actually exists.
- Vertical stripes are preferred over horizontal stripes — vertical
  doesn't accidentally form a "row" that competes with the mouth.
- Centred at `(32, 32)` like the body. Decoration extent should sit
  within the body's typical bbox so anchor offsets don't push it off.

### Accessory
- Drawn at the visual position the accessory should appear (a hat
  goes above the head, a scarf around the neck). Centred at `(32,
  24)` works for most.

## 4. Anchors (body-side calibration)

Each body shape declares `anchors.json` listing per-layer offsets:

```json
{
  "eyes":      { "x": 0, "y": 0 },
  "mouth":     { "x": 0, "y": 0 },
  "ears":      { "x": 0, "y": 0 },
  "tail":      { "x": 0, "y": 0 },
  "pattern":   { "x": 0, "y": 0 },
  "accessory": { "x": 0, "y": 0 }
}
```

The legacy array form `{"eyes": [x, y], ...}` is still accepted (no
`pattern` key needed; it defaults to `{x: 0, y: 0}`).

**Interpretation:** the anchor is the pixel offset added to every
layer-sprite pixel. So an anchor of `(0, 0)` means "layer sprite draws
in canonical position" (matches §2/§3 defaults). An anchor of
`(-13, 0)` shifts the layer 13 px left to compensate for a body whose
face centre is at `x=19` rather than the canonical `x=32`.

### Worked example — round body (face at x=19)

Round body's visible head silhouette is centred at `x=19, y=18`, off-
centre from the canonical `(32, 20)` face position. To compensate:

```json
{
  "eyes":   { "x": -13, "y": -2 },
  "mouth":  { "x": -13, "y": -2 },
  "ears":   { "x": -13, "y": -2 },
  "tail":   { "x": -16, "y":  0 },
  "pattern":{ "x":   0, "y":  0 }
}
```

The `-13` for eyes/mouth/ears shifts those layers left so they land on
the round body's actual face. Tail's `-16` shifts the right-attached
tail leftward because the round body's right edge sits at `x=30`, not
the canonical `x=50`. Pattern stays at `(0, 0)` because masking by
body alpha handles positioning automatically.

## 5. What lives in the design grid vs. anchor offsets

- **Design grid (§2/§3)** is what the artist draws to. It assumes a
  hypothetical "ideal body" centred at the frame.
- **Anchors (§4)** translate from that ideal to a particular body
  shape's actual geometry.

A new pet body shape only needs an `anchors.json` to be visually
correct — no new art needed for the eyes/mouth/ears/tail/pattern that
already follow the grid.

A new layer shape (e.g., a new eye variant) only needs to follow the
grid — no per-body work needed.

## 6. Source of truth for offsets

When in doubt about "where should X go on body Y", load the body PNG
in Piskel, eyedrop the visible head/face centre coordinates, and
compute the anchor as `(canonical - measured)`. Don't guess; the
asymmetry has bitten us once already.
