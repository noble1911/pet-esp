# Pixel graphics, second pass

These PNGs are actual production LVGL renders from `tests/host/test.c`, at the
ESP display's 368 x 448 resolution. `home-idle.png` shows the compact voice
control; `home-speaking.png` shows the expanded caption and talking face.
The approved reference remains `art/pixel-v1/approved-concept.png`.

Changes: authored stepped body contour, waving paw, tucked feet, ground shadow,
coat highlights, sleeping pillow and embroidered quilt, apple-holding paws,
separate talking mouth without a snack, heart happiness icon, three-part need
meters, inset care-button highlights, and a smaller idle speech panel. The six
coat colors and five growth stages are preserved. This is an incremental native
sprite pass; the room background remains the existing pixel artwork.

Validation: host gameplay/voice tests, including panel expansion and dedicated
talking pose assertions; ESP-IDF firmware build and USB flash. No NVS erase.

Rollback: tag `pre-graphics-v2` preserves the prior voice-enabled source. To test
it without disturbing this checkout, make a separate worktree at that tag and
build/flash using `scripts/device.sh`. Flashing the app preserves saved pet data.

Sprite Studio (https://github.com/JohnKinyanjui/sprite-maker) supports sprite
and animation authoring, but was not installed for this pass: the changes fit
our existing native C renderer and retain dynamic coat/growth customization.
