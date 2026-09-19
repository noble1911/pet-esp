# Speech and eating polish

The home screen now has a 56 × 48 microphone button centred beneath the pet. Hold it to talk and release to send, exactly like BOOT. The microphone turns mint while listening and blue while thinking. Its picture uses native LVGL shapes so it does not depend on a font containing a microphone glyph.

Home and celebration captions share a rounded cream bubble with plum outline, soft shadow and a small stepped tail. Short captions centre vertically; longer captions retain a bounded, scrolling text area. The entire bubble disappears together when speech/reactions expire.

## Eating

Meals take 2.8 seconds: hold the snack, take a bite after 650 ms, chew gently until 2.1 seconds, then settle before a 200 ms fade out. The celebration fades in over 200 ms. Selection cannot change during the meal; stats, one star and the completed-food event are still committed only once at completion. Leaving before completion cancels it. A failed save restores full opacity and shows the existing error.

The yellow rectangle on coloured pets came from the coat-tint exclusion: all pixels in a broad rectangle below the face kept their source yellow, including belly and paws. Each food now has separate hold/bite silhouettes to preserve the food itself while recolouring surrounding fur. Artwork is unchanged. These masks are in `data/eating-food-masks.json`; `python3 scripts/pack_food_masks.py` compiles the small row-span table used by the renderer (Pillow required only when regenerating). The generated header is checked in for normal firmware builds.

## Validation and delivery

- Real LVGL touch tests cover the compact microphone, release/BOOT behaviour, the gap outside its hit target, captions and existing game/reward navigation.
- All seven foods and both eating poses checked across six coats. Regression tests require belly recolouring while protecting food centres/highlights, including toast, pancakes, cake and the cupcake's star.
- Meal timing, settled pause, fade, exactly one award, late cancellation, and save-failure recovery pass, together with the full host regression suite.
- ESP-IDF 5.3.5 firmware build passed. The device was disconnected for user testing, so this version has **not been flashed or tested on physical hardware**.
- No pet save, backend, model or voice changes. Rollback checkpoint: `pre-speech-polish-v1` (`dd61b30`).

[Actual UI screenshots and meal animation](previews/speech-polish-v1/README.md).
