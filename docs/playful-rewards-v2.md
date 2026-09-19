# Playful rewards v2

Home → star → **My treasures** offers two large picture buttons: **Stickers** (heart) and **Gifts** (present). Both collections return to this menu with the back arrow.

## Room gifts

All six gifts have separate fixed places. In Gifts, tap **Add gift** to place one or **Remove ✓** to put it away; the screen stays open so several can be chosen. **See my room** returns home. **Put all away** removes room gifts, keeping the wall sticker and every unlock. Stars are never spent.

Tap placed gifts for a four-second pretend interaction: flowers sway, bunting makes a rainbow dance, teddy releases hearts, the moon lamp brings a sleepy daydream, the cushion bounces, and the trophy celebrates. Gifts do not restore needs or earn stars.

## Stickers

Tap an earned sticker to put it in the little wall frame and immediately play its animation and sound. Tap the frame to replay. Choosing another replaces the favourite; the collection retains everything. **Take sticker off wall** clears only the frame. The selection survives reboot.

All eighteen stickers have themed artwork, phrases and sound/pose combinations. Motions include rising bubbles/hearts, bouncing balls/bunnies, fluttering butterflies/kites, orbiting rainbows/sunshine, showers and gentle sways. The music sticker triggers a short musical sound and dance. The apple is pretend nibbling, the moon a daydream: these are toys, not care activities.

Only one effect runs at a time. Repeated taps restart it; navigating away or starting voice clears it. Sound follows existing mute/volume and microphone/speech priority. No network call or new AI charge is needed for these effects.

## Save compatibility and rollback

No NVS schema or struct layout changes. Unlocks still derive from lifetime stars. Existing single-gift saves load without a write or migration.

- `inventory[13]`: 200..217 is the wall sticker; zero or invalid/locked values mean none.
- `inventory[14]`: 128..191 encodes a six-bit gift mask. Locked bits are filtered. Absent/invalid markers use the old single-gift slot.
- `inventory[15]`: remains 100..105 for the first placed gift, or zero. This lets the previous release display one gift if rolled back. Selecting gifts in the older release does not edit the newer multi-gift mask; returning to this release restores that saved arrangement.

Selections save a copy before changing live state. Save failure leaves the room unchanged and shows an error. Starting fresh clears both room arrangement and wall sticker with the other pet state.

The source checkpoint before this update is `pre-playful-rewards-v2` (`2602231`). Build it in a separate worktree using the same ESP-IDF setup and private configuration as described in [the original rollback instructions](playtime-v1.md#save-compatibility-and-rollback). Flash app/assets without erasing NVS.

The pet-only backend derives all placed gifts and the favourite wall sticker from each snapshot, understands both save formats, and explains the new menu/actions. Existing Butler users and the pet's Haiku model are unchanged.

## Validation

- Real LVGL pointer tests: star menu, both branches/back links, multi-gift toggling/removal, each placed gift's tap target, and all 18 sticker actions.
- Reload persistence, schema-1 single-gift compatibility, locked/invalid selections, failed saves, all 256 encoded byte values, and no care-star or need changes from toy interactions.
- Replay, navigation cancellation, voice interruption, and existing food/game/traits/reset/volume regression tests.
- Fifteen pet backend tests, including both old/new gift encodings and all byte values for gift/sticker decoding.
- ESP-IDF 5.3.5 build; 40% of the 3 MB app partition remains free. Effects reuse five small LVGL image objects and existing flash-backed art; no additional full-screen buffers or runtime image decoding.

[Production UI screenshots and animation](previews/playful-rewards-v2/README.md). Captures use the real UI with mocked hardware and an unlocked test pet; they do not grant rewards to the physical pet.
