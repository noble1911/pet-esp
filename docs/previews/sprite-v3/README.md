# Sprite v3 review — actual firmware renders

All PNGs and GIFs in this directory come from production `ui.c` and the real
LVGL renderer at the ESP's 368x448 resolution. They are not prospective mockups.

Open `overview.png` for the screen comparison. The `*-animation.gif` files show
idle, each food, bath, sleep, play, party and talking. Food animations include
the transition to the care reward screen. `eat-*-hold.png` and `eat-*-bite.png`
show the three selected foods directly. Colour and growth previews cover six
coat colours and all five growth stages.

Changes:
- Finished illustrated character poses and matching room/icon artwork.
- Selected snack survives subsequent taps; both held and bitten art match it.
- Distinct wave/blink, cuddle/dance, listening/thinking/talking, reach, bath/splash
  and breathing-under-quilt states. Beds and tubs stay grounded.
- Idle captions no longer cover the room. Hold the fixed button beneath the pet
  or BOOT to talk; captions appear in the room when needed.
- Large care buttons, readable labels, and the existing options/volume/Wi-Fi
  screens remain available. Brighter Sprout and pet-only Haiku remain unchanged.

Validation: host care, growth, persistence, cancel, repeated navigation, volume,
voice/button and proactive speech tests pass. Added checks cover all three
selected foods, different held/bite pixels, extra taps and exactly one reward.
ESP-IDF 5.3.5 build succeeds, with 46% app-partition space remaining. USB flash
preserves saved pet state; no erase was performed. Boot verification confirmed
Sprout loaded at stage 3 with its existing pet ID, display/touch up, audio tasks
ready with the PSRAM speech buffer, Wi-Fi connected and pet gateway ready.
No boot errors or display-transfer failures were logged.

Reproduce:

```sh
cmake -S tests/host -B /tmp/pet-host-build
cmake --build /tmp/pet-host-build -j8
mkdir -p docs/previews/sprite-v3/frames
(cd docs/previews/sprite-v3 && PET_CAPTURE_ANIMATIONS=1 /tmp/pet-host-build/pet_test)
python3 scripts/render_sprite_review.py
./scripts/device.sh build
```

The large intermediate PPM animation captures are ignored; GIFs are retained.
Rollback source is tagged `pre-sprite-art-v3` (e4a3f05), including working voice.
For an independent rollback checkout: `git worktree add ../pet-before-v3
pre-sprite-art-v3`, then build/flash there using its `scripts/device.sh`.
Flashing source-built firmware leaves the saved pet progress intact.
