# Osono and character idle speech

Osono is choice **8 / 8** in **Options → My pet → Choose character**.
The approved Osono-inspired bakery character has swept auburn hair, red earrings,
a green dress, cream apron and brown shoes. She has 26 complete painted poses:
idle, wave, blink, talk, listen, happy, reaching left/right, sleeping/breathing,
bathing/splashing, and hold/bite poses for all seven foods. No facial parts are
composed separately. The previous seven characters' compiled sprites are unchanged.

[Chooser](previews/osono-v1/choose-character-7.png) ·
[Face animation](previews/osono-v1/osono-face-loop.gif) ·
[All actions](previews/osono-v1/all-actions.png) ·
[All foods](previews/osono-v1/all-foods.png)

Source images were created with built-in imagegen, using the approved concept as
the character reference. The reproducible prompt set and source sheets are in
[art/characters-v1/osono](../art/characters-v1/osono/), including
[motion prompt](../art/characters-v1/osono/motion-prompt.txt) and
[food prompt](../art/characters-v1/osono/food-prompt.txt). The existing packer
extracts and scales only complete drawings to 72×72 and verifies lossless RGB565
RLE decoding. Osono adds 113,639 bytes including offsets, approximately 111 KiB.

Marker 247 extends the existing saved character selection. Pet structure/schema,
personal name, account, stats, rewards and memories remain unchanged. Osono is
the character label, not a forced personal name. The same selection, care,
progress and multiplayer rules apply to every character.

## Idle personality

Every character now has its own voice style, three topic families and examples
in `data/pet_characters.json`. The generated backend catalogue remains the source
for current character context:

- Sprout: growing, seeds, sunshine and raindrops.
- Cloud bunny: cloud hopping, burrows and cosy naps.
- Pebble penguin: waddling, snowflakes, sliding and flipper dances.
- Peach kitten: sunbeams, cardboard castles and yarn.
- Tiny dragon: little wings, pebble treasures and imaginary quests.
- Rosy pig: puddles, picnics and splashes.
- Larry: lunch breaks, imaginary meetings, paperwork and clocking off.
- Osono: bread dough, floury aprons, imaginary baking and tea breaks.

An idle turn selects a topic with the least word overlap against the last two
assistant replies, randomly choosing between ties. The model receives that focus
and a fresh current-character reminder, so switching characters overrides old
conversation descriptions. Examples guide tone rather than form a fixed playlist.
This reduces repetition; generated speech can still occasionally repeat a theme.
Fresh care reactions prioritise the actual event and exact food. Automatic turns
are explicitly identified as automatic rather than mislabelled player speech.
Name-only replies still use only the saved personal name.

The existing Little chats toggle, timing, mute and volume guards remain in place.
Only the pet uses Haiku. No extra model calls or database queries are added per
remark, and proactive turns still have no memory/music tools. Speech synthesis
settings are unchanged. Osono speaks as a toy friend, not the player's parent.

## Validation and installation

The full host gameplay/UI suite passes, including all 208 complete frames,
eight chooser options, save/reload and failed-save recovery, all 64 multiplayer
character pairings, and existing food/reward/voice guards. The actual renderer's
chooser, animation and food previews were visually inspected. Catalogue and
atlas regeneration checks pass.

The ESP-IDF firmware build passes: application size `0x29b9e0`, with `0x64620`
bytes (approximately 402 KiB, 13%) free in the current 3 MiB application partition.
**Firmware is built, not flashed in this change.** The second board's hardware
identity and separate provisioning are still pending.

Backend commit `1f55d3f` is deployed on Ron's Mac mini; all 20 tests pass against
the deployed route. Gateway commit `382552b` accepts Osono through discovery,
invitations and completed playdates; the gateway suite has 62 passing tests.
Both deployed services report healthy. Live Haiku auditions exercise every
character and repeated adult-character turns without accessing real pet data;
see `integrations/scripts/check_character_idle.py` and
[idle audition output](previews/osono-v1/idle-audition.jsonl).

Rollback firmware restore point: `multiplayer-v1.1` (`4f48508`). Its older
catalogue shows Sprout if marker 247 is selected; this release recognises Osono
again if the marker is retained. Never erase NVS for a rollback. Prior backend
commit is `4d25586`; prior gateway commit is `956a04d`, with Docker image
`esp-gateway:before-osono` and source copy in `~/pet-osono-backup` on the mini.
Do not roll back unrelated backend work or remove the playdate ledger volume.

Logs: `/tmp/pet-osono-build.log`, `/tmp/pet-osono-host-test.log`,
`/tmp/pet-osono-gateway-tests.log`, `/tmp/pet-osono-deployed-backend-tests.log`.
