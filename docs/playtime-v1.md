# Playtime and keepsakes

## Controls

- Home → **Play** opens Stars, Peekaboo and Bouncy ball.
- Stars: five stationary targets. Peekaboo: find Sprout behind three flowerpots, three times; leaves provide a hint. Wrong guesses reveal empty spots with no penalty. Bouncy ball: five taps on a slow moving ball, which stops under a held finger. All rounds are untimed.
- A completed round restores 30 happiness and earns **one** saved care star. Leaving a round cancels its unfinished progress. Feeding, naps and baths still earn one star each.
- Home → **star** opens three pages of stickers. A new sticker is earned every five care stars, through 90 stars. Tap a sticker for its name or remaining progress.
- **My room gifts** opens six decorations: flowers (10), bunting (20), teddy (30), moon lamp (45), rainbow cushion (60), trophy (90). Tap an earned gift to use it, or Plain room to remove it. One gift is displayed at a time. Nothing spends stars or loses earned gifts.
- Home occasionally has a butterfly visitor. Tap it for a happy reaction; no extra care star is awarded. It waits while voice is busy. Window weather cycles through sun, gentle rain and rainbow roughly every three minutes of uptime; this is pretend room weather, not a real weather service.
- BOOT / Hold to talk, voice options, Wi-Fi check and 100% startup volume are unchanged. The isolated Haiku pet route knows the new game events and receives derived sticker/gift state on every prompt.

## Save compatibility and rollback

`Pet` is still schema 1 with the identical structure and NVS key. Stickers and unlocked gifts derive from lifetime `evolution_progress`, so existing care counts automatically. The previously unused `inventory[15]` stores the equipped room gift as 100..105 (zero = plain). Invalid or locked IDs render as plain. Saving a selection persists a copy before changing live state; a failed save leaves the existing choice intact. Older firmware ignores this slot.

The source checkpoint before this update is **`pre-playtime-v1`** (`9693584`). To inspect/build it without changing the current checkout:

```sh
git worktree add ../pet-esp-before-playtime pre-playtime-v1
cp firmware/components/voice/secrets.h ../pet-esp-before-playtime/firmware/components/voice/secrets.h
cp firmware/sdkconfig ../pet-esp-before-playtime/firmware/sdkconfig
cd ../pet-esp-before-playtime
./scripts/device.sh build
./scripts/device.sh -p /dev/cu.usbmodem101 flash
```

Flashing these app/assets partitions preserves NVS. Do not erase flash or restore an old full-device image if you want to retain current care progress. The backend accepts both old and new firmware snapshots; its independent pre-change checkpoint is HomeServer `18b8d94`.

## Validation

- Actual LVGL pointer tests cover all three games; wrong guesses, held ball, rapid repeated taps, completing exactly once, leaving during a reveal, and no reward for partial play.
- Schema-1 save reload: old stars unlock rewards; decoration selection survives reboot; locked/out-of-range IDs are rejected; failed saves preserve live selection; plain-room selection works; uint32-max stars cap at eighteen stickers without overflow.
- All album pages, gift navigation, gift milestones, weather modes and butterfly interaction; visits award no stars and wait during speech; 100 mixed screen changes exercise object lifetimes.
- Existing food-specific frames, growth, six coats, care cancellation, volume/mute, voice controls and reaction cooldown tests still pass.
- Eight backend tests verify account/memory isolation, Haiku-only override, event validation and accurate derived rewards. Live Haiku checks verified Peekaboo, ball and butterfly reactions using a temporary pet account, removed afterwards.
- Flashed the connected ESP32-S3 over USB with verified hashes. A 32-second serial boot capture confirmed the existing Sprout identity and stage-3 save, 368×448 display/touch, speaker/microphone tasks, Wi-Fi and pet gateway ready, with no panic or reset loop.
- ESP-IDF 5.3.5 build; about 43% of the 3 MB application partition remains free. Generated art stays in flash, with no runtime image decoding or additional full-screen buffer.

[Actual UI captures and animations](previews/playtime-v1/README.md). These are renders of the production C UI at 368×448; touch tests use real LVGL input, mocked hardware and NVS. Physical child playtesting remains useful for difficulty and preference.
