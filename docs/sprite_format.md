# Sprite format

Expands [architecture.md §5](architecture.md). The architecture doc is
authoritative; this file is the working reference shared by the on-device
`renderer` component and the laptop-side `sprite_forge` tool — both must
agree byte-for-byte.

## On-device `.bin` sprite sheet

One file per part animation. No PNG decoder on device.

```
offset  size  field
0       4     magic   = "PSPR" (0x50 0x53 0x50 0x52)
4       1     width        (px, per frame)
5       1     height       (px, per frame)
6       1     num_frames
7       1     format       0 = grayscale+alpha
                           1 = palettized
                           2 = RGB565
8       N     pixel data   width * height * num_frames * bpp
```

Bytes-per-pixel by `format`:

| format | meaning          | bpp | notes                                  |
|--------|------------------|-----|----------------------------------------|
| 0      | grayscale+alpha  | 2   | `[gray, alpha]`; tinted at draw time   |
| 1      | palettized       | 1   | index into sheet/part palette          |
| 2      | RGB565           | 2   | little-endian; used as-is, no tint     |

Frames are stored consecutively, frame 0 first, row-major within a frame.

## Tinting (format 0)

Part art is authored grayscale + alpha. At draw time the grayscale value
maps to a shade of a tint colour chosen from a 16-entry palette indexed by
genes (`body_color`, `eye_color`). One sprite × 16 palette entries → 16
variants from a single asset.

## Layer stack (back to front)

```
6 Accessory   (optional)
5 Pattern     (tinted)
4 Eyes        (shape + tint)
3 Mouth       (shape)
2 Ears/Horns  (shape + body tint)
1 Tail        (shape + body tint)
0 Body        (shape + primary tint)
```

## Anchors

Each body shape defines anchor offsets (eyes, ears, mouth, tail, accessory)
so upper layers compose correctly regardless of body proportions. The
human-authored source (sprite_forge owns the schema):

```
parts/{stage}/body/{shape}/anchors.json
{
  "eyes":      [x, y],
  "mouth":     [x, y],
  "ears":      [x, y],
  "tail":      [x, y],
  "accessory": [x, y]
}
```

`build.py` does **not** ship this JSON. It emits a fixed binary sidecar
`anchors.bin` next to the shape's `.bin` so the device needs no JSON parser:

```
offset  size  field
0       4     magic   = "PANC"
4       1     version = 1
5       1     count   = 5
6       N     count × { int16 x, int16 y }  little-endian
              order: eyes, mouth, ears, tail, accessory
```

Coordinates are pixels relative to the body sprite's top-left, at the
authored part size (~64×64); int16 is signed for future off-frame offsets.

## Animation states

- Body: `idle`, `walk`, `sleep`, `eat`, `play`
- Eyes: `normal`, `blink`, `happy`, `sad`, `surprised`
- Mouth: `neutral`, `smile`, `open`, `frown`

Sprite animation advances at ~5 fps; the UI overlay runs at 30 fps.

## Source layout (sprite_forge)

```
sprite_forge/parts/{stage}/{layer}/{shape}/{animation}.png
sprite_forge/palettes/*.png
```

`build.py` validates/packs and emits `.bin` (plus body `anchors.bin`) into
`firmware/assets/` (gitignored). That tree is flashed to the device in a
LittleFS `assets` partition (architecture §2/§5; SD deferred).
