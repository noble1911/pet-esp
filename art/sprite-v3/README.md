# Illustrated Sprout — sprite art v3

Generated with the built-in imagegen tool from `art/pixel-v1/approved-concept.png`.
The design choice is finished illustrated sprites, not a body assembled from
ellipses and rectangles. Sprite Studio was tested earlier but is not required by
this asset pipeline. The prompt set is recorded in `PROMPTS.md`.

Sources are preserved. `poses-source.png` is the initial identity reference;
it contained a simulated checkerboard, so `poses-key.png` is the corrected,
solid-magenta production source. The compiler removes that supplied chroma key,
splits the regular atlas cells, and packs their pixels for the device. No runtime
PNG decoder, network art generation, or new Python dependency on the ESP.

18 whole-character frames, each on a 72x72 canvas:
- wave, relaxed idle, delighted cuddle, attentive listening;
- apple/toast/cookie held and bitten (six independent food poses);
- sleep and breathing quilt; bath and splash;
- blink, talking mouth, reaching left and right.

Pixels are stored as RGB565 in flash with zero reserved for transparency. A
single ARGB working image is rendered by LVGL at exact 3x nearest-neighbour
scale. The original coat gene still tints yellow fur; foods and care props keep
their authored colours. Babies are smaller; later stages retain their small
growth keepsakes. Pet NVS schema and gameplay rewards are unchanged.

Icons are authored 24x24 assets compiled to ARGB arrays. Food buttons now use
the same snack designs as the character's eating frames. Day and night rooms
are compiled as 184x152 RGB565 images displayed at exact 2x scale.

Regenerate firmware assets:

```sh
python3 scripts/pack_pet_sprites.py
python3 scripts/pack_pixel_rooms.py
```

The scripts require Pillow on the development Mac only. Generated headers are
checked in so ordinary firmware builds do not need Python image dependencies.
