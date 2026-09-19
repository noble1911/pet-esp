# Genetic sprites v1

**Historical release:** superseded by [five complete characters](five-characters-v1.md). Appearance parts and trait browsing are no longer used by current firmware. The details below document the older release.

All seven appearance genes are active in the illustrated renderer. The eight
personality values still affect gentle voice context, not physical appearance.
Saved genes, pet identity, rewards, needs and NVS schema are unchanged.

| Property | Looks |
| --- | --- |
| Body | Pear, Round, Tall, Chubby, Bean, Teardrop, Dumpling, Peanut |
| Coat | 16 shades; original six plus Honey, Lavender, Sea mint, Berry pink, Apricot, Cornflower, Buttercream, Orchid, Pistachio, Blossom |
| Eyes | Round beads, Oval, Sleepy, Sparkly, Almond, Tiny dots, Curious, Crescents |
| Iris | 16 colours, visible with open eyes |
| Ears/tuft | Sprout, Teddy, Kitten, Bunny, Floppy, Little horns, Fluffy tuft, Leaf ears |
| Smile | Little smile, Big smile, Kitty smile, Little O, One tooth, Two teeth, Cheeky grin, Heart smile |
| Markings | Plain, Spots, Stripes, Cream bib, Star patch, Heart patch, Freckles, Cloud patches |

The original 26 complete pose frames remain the action foundation. Eight body
profiles reshape their silhouettes; 48 authored feature tiles supply ears,
open eyes, three mouth states and coat markings. Traits follow waving, idle,
blink, happy/reaching, listening/thinking, talking, both eating poses for all
seven foods, sleep/breathing and bath/splash. Closed eyes keep the appropriate
authored expression; food, quilts and foam naturally hide covered markings.
Existing baby scaling and teen/adult/elder keepsakes still apply. No new life
stages or gameplay effects were added.

All 16 coat genes now have distinct colours. Values 0–5 retain their original
palette; 6–15 use nearby shades of their previous modulo-six colour families.
No save migration or reset is required. Out-of-range bytes use the catalogue
modulus, without writing normalized values back to the saved pet.

Options → My pet previews every appearance trait through the production
renderer. The temporary preview changes only the chosen gene and never saves.
The former grey/saved-for-later cards are now active.

## Voice compatibility

Firmware sends `artwork_version: 2` on every voice snapshot. The pet-only Butler
route selects the current catalogue for v2 and the frozen six-colour catalogue
for older firmware, including a rollback. Missing version means v1. This lets
the backend update safely before the disconnected device is flashed. The
existing gateway forwards the snapshot intact. Model, voice, memory isolation
and the rest of Butler are unchanged.

## Validation and previews

- All 72 individual visual choices have distinct idle renders.
- 49,152 mixed-gene, stage and action renders check pixel bounds, alpha and
  unchanged input state; every stored byte value is exercised.
- All seven foods retain their protected pixels across mixed visual traits and
  both poses; all sixteen coats pass the yellow-belly regression.
- All appearance previews are compared to the production renderer; navigation
  and browsing preserve saved state. The existing game/reward/power suite passes.
- Fifteen Butler tests cover both artwork versions, all gene bytes, actual
  prompt context, memory isolation and the request-scoped Haiku override.
- ESP-IDF build passes with 39% app flash free. Flashed over USB on 2026-09-19
  after the device was reconnected; all four writes passed hash verification.
  A 20-second serial capture confirmed app version `genetic-sprites-v1`, Olive's
  existing identity `6266ea62beb19b67`, stage 2 and saved needs (77/82/86/88),
  display/touch/LVGL, speaker/mic, Wi-Fi and authenticated pet gateway readiness.
  No panic or boot loop was observed. On-device visual and interaction testing
  is left to the user; boot verification does not substitute for that test.

[All traits](previews/genetic-sprites-v1/all-traits.png),
[pose matrix](previews/genetic-sprites-v1/pose-matrix.png),
[all food poses](previews/genetic-sprites-v1/all-food-poses.png),
[animated family](previews/genetic-sprites-v1/pet-family.gif).
These are captures of the real C renderer, not concept illustrations.

Reproduce:

```sh
python3 scripts/pack_genetics.py --check
python3 scripts/generate_pet_traits.py --check
cmake --build /tmp/pet-host-build -j8
mkdir -p /tmp/pet-genetics-test
(cd /tmp/pet-genetics-test && PET_CAPTURE_GENETICS=1 /tmp/pet-host-build/pet_test)
(cd /tmp/pet-genetics-test && /tmp/pet-host-build/pet_test)
python3 scripts/make_genetics_previews.py /tmp/pet-genetics-test
./scripts/device.sh build
```

## Rollback

`pre-genetic-sprites-v1` preserves the prior code at `8554dda`, including the
pending bouncing ball, back target, PWR and speech improvements. Build/flash
that tag from a separate worktree to return to the previous art without erasing
NVS. `genetic-sprites-v1` marks the completed new artwork. The version-aware
backend can stay deployed with either firmware.
