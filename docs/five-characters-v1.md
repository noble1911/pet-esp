# Five complete characters v1

Replaces the hybrid genetic renderer that moved facial parts independently.
The original Sprout returns unchanged, joined by Cloud bunny, Pebble penguin,
Peach kitten and Tiny dragon. Every character has 26 complete illustrated frames:
idle, wave, blink, happy, listening, talking, left/right play poses, two nap and
two bath poses, plus hold/bite poses for apple, toast, cookie and four treats.
There are no runtime eyes, mouth, ears, markings, recolouring or body warps.
Baby size scales the complete frame; other stages share native-size artwork.

## Choosing and preserving a pet

Open **Options → My pet → Choose character**. Arrows preview all five friends.
**Choose this character** saves only the appearance. The current option is
labelled **My character**. Browsing, cancelling and failed saves leave the pet
unchanged. Name, ID, needs, care stars, gifts, stickers, personality and voice
memory identity all remain. Existing pets default to the original Sprout.

The saved struct and NVS schema 1 are unchanged. `genes[6]` values 240–244 encode
an explicit character selection; all other values map to Sprout. Other old
appearance bytes are retained for compatibility but ignored by rendering.
Personality remains `genes[7] % 8`. A successful choice changes only byte 6.
There are no rarity, unlock or care differences between the five characters.

## Art and voice

See [source art and prompts](../art/characters-v1/README.md). The checked-in
RLE atlas holds 130 complete frames in 635,520 bytes, compared with 1,347,840
uncompressed RGB565 bytes. Asset compilation asserts no clipped opaque pixels
and exact RLE round trips. Normal poses share one scale and horizontal origin;
whole-image baseline alignment avoids independent feature motion.

`data/pet_characters.json` generates firmware and Butler catalogues with
`scripts/generate_pet_characters.py`. Firmware voice snapshots use artwork
version 3; Butler derives the actual character and personality without exposing
obsolete body-part genes. Character type is separate from the pet's personal
name. Version 1 and 2 clients remain supported. Pet Haiku, voice, memory isolation
and the rest of Butler's model settings are unchanged. Backend commit `87f227f`.

## Verification

- Host checks decode all 130 frames, distinguish all five characters, exercise
  every action, food and stage, check output bounds, and prove legacy appearance
  bytes cannot alter the selected character.
- Real LVGL pointer tests cover all chooser previews, wraparound, cancellation,
  failed save, successful save/reload, and preserving every other saved field.
- Full gameplay, reward, food timing, speech UI, power and persistence suite passes.
- Sixteen Butler tests pass, including all character markers and all byte values
  for old appearance genes, version compatibility and existing voice isolation.
- Actual C-renderer [animation preview](previews/five-characters-v1/five-friends.gif),
  [all actions](previews/five-characters-v1/all-actions.png),
  [all food poses](previews/five-characters-v1/all-foods.png), and
  [chooser](previews/five-characters-v1/choose-character-1.png) reviewed.

## Rollback

`pre-five-characters-v1` preserves the immediately preceding firmware, including
its genetic renderer. `pre-genetic-sprites-v1` preserves the older original
Sprout renderer with the recent gameplay/control changes. Build either in a
separate worktree and flash without erasing NVS. The updated backend supports
both older artwork versions and can stay deployed.

After selecting a new character, older firmware interprets byte 6 as an old
marking gene, so the old marking may differ on rollback. All other genes and
progress remain intact. Merely upgrading does not write any marker. Restoring
this release recognises the selected character again if byte 6 was unchanged.

## Device flash record — 2026-09-19

Firmware tag `five-characters-v1` (`8d8d125`) built successfully with ESP-IDF
5.3.5: application size `0x2290d0`, 28% of the 3 MiB app partition free.
Flashed `/dev/cu.usbmodem101`; esptool verified the written hashes and reported
success. NVS was not erased. The backend is deployed and healthy, with all
16 tests passing against the deployed route.

The gateway recorded this device reconnecting at 19:45:04 UTC, and its known
Wi-Fi address 192.168.1.58 responds to ping. The first USB boot capture reached
the loaded-app message, then stopped; subsequent serial sync attempts returned
no data although the USB device remains enumerated. Thus the flash and network
reachability are verified, but post-flash screen behaviour and the saved pet
have not been independently rechecked from application logs. A user screen
check / USB reconnect has been requested. This is not a verified on-device
animation review.

Local logs: `/tmp/pet-five-characters-build.log`,
`/tmp/pet-five-characters-flash.log`, `/tmp/pet-five-characters-boot.log`,
`/tmp/pet-five-characters-gateway.log`. Original imagegen sources and exact
prompts are retained in `art/characters-v1`.
