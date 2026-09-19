# Special milestone foods

Open **Food → Special treats**. Earn care stars by feeding, playing, bathing or finishing a nap. The menu shows each treat, its goal and stars remaining; unlocked treats say **Yours! Tap to enjoy**. A newly reached goal gets a celebration with a direct button to the treats menu, and any simultaneous sticker/room-gift reward is also listed.

| Treat | Care stars |
|---|---:|
| Star cupcake | 10 |
| Berry pancakes | 25 |
| Rainbow jelly | 50 |
| Party cake | 100 |

These are permanently unlocked recipes, not limited stock. Existing saved care stars count automatically. Nothing is purchased, deducted or claimed separately. Apple, toast and cookie remain freely available. **Start fresh** resets the milestones with the rest of the pet.

A special meal restores up to 40 fullness and 10 happiness, capped at 100. It awards the same one care star as any other completed care action; ordinary food restores up to 30 fullness. Leaving before the eating animation finishes gives no stats, stars or completed-food event. A failed save shows an error without awarding the meal. Rapid taps cannot switch the active food or award it twice. No NVS schema or inventory changes are needed.

## Artwork and sound

The built-in imagegen tool created one atlas matching the existing complete food sprites. It supplies eight full character poses (whole and bitten versions of each food) and four menu icons. Original [atlas](../art/special-foods-v1/treats-source.png) and exact [prompt](../art/special-foods-v1/PROMPT.md) are retained. `scripts/pack_special_foods.py` extracts the authored rows, applies the magenta transparency key, resizes using nearest-neighbour sampling and encodes the assets into flash. It preserves the first row's feet by using the inspected gutters instead of equal thirds. No runtime PNG decoder or new sprite buffer is needed. Fur still uses the pet's genetic coat colour; the food keeps its authored colour.

Cupcake gets a bright bell phrase, pancakes a warm plucked phrase, jelly a wobbling bounce and cake a celebration phrase. Existing mute, volume and speech interruption rules apply. Auditions: [cupcake](previews/special-foods-v1/cupcake.wav), [pancakes](previews/special-foods-v1/pancakes.wav), [jelly](previews/special-foods-v1/jelly.wav), [cake](previews/special-foods-v1/cake.wav).

The pet voice backend receives the unlocked/locked foods and remaining star goals in its derived reward snapshot. Exact food events distinguish each completed meal, including automatic reactions. Voice cannot feed the pet, unlock foods, spend stars or infer permanent preferences from one meal. Haiku and household Butler settings remain unchanged.

## Validation

- Production LVGL touch suite plus all four boundaries, locked-food rejection, existing-save unlocks, direct celebration navigation, cancel, repeated taps, correct completed-food event/sound, save failure, stat caps, star-counter saturation, reload persistence and reset relocking.
- Render tests compare whole/bitten poses and distinct foods, and verify that coat changes leave the food region unchanged. Reviewed [device overview](previews/special-foods-v1/overview.png), individual screens and four short animation previews in the same directory.
- `scripts/test_audio.sh` passes for the actual renderer/speaker owner, including distinct new effects, bounded peaks/duration and existing interruption/stop behaviour.
- Fourteen Butler tests pass, including all food thresholds, availability snapshots and proactive reactions for each food event.
- ESP-IDF build and verified USB flash; 40% of the application partition remains free. Boot confirms the existing Sprout identity (`dffaa588e4bc0d90`), saved teen stage, display/touch/audio, Wi-Fi and pet gateway ready. The real pet is not fed or reset by the tests; its existing save is retained.

## Deployment and rollback

Backend commit: `c00e0e3` in HomeServer; gateway unchanged. The pre-change server route is backed up on the Mac mini at `~/pet-voice-backup/pet-pre-special-foods.py`.

Firmware checkpoint **pre-special-foods-v1** (`187a39d`). Build/flash that tag in a separate worktree to remove special foods; saved progress and ordinary snacks remain valid. Keeping the newer backend is compatible with older firmware. No save migration or rollback is required.
