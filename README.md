# Little Meadow

Voice v1: Sprout can now chat using the existing Mac mini voice stack. Tap its name or hold BOOT to talk. Rename it and check Wi-Fi in Options. See [voice controls, setup and rollback](docs/voice-v1.md).

A gentle touchscreen pet for the **Waveshare ESP32-S3-Touch-AMOLED-1.8 (original SH8601 / FT3168 board)**, built with ESP-IDF 5.3.5 and LVGL 9.

![Screens](docs/previews/pixel-v1/overview.png)

## Pixel-art v1

The current branch `codex/pixel-art-v1` implements the approved Tamagotchi-inspired direction: warm illustrated room, night room, an animated pixel sprout creature, colourful icon buttons and pixel headings. Existing gameplay and NVS saves are unchanged. Six coat colours and five growth stages remain supported.

The previous shaded-bunny version is preserved at Git tag **`vector-art-v1`**. To return the device to that version, run:

```sh
./backup/vector-v1/restore.sh /dev/cu.usbmodem101
```

This uses a separate verified binary bundle and does not erase NVS. For the old source in a separate folder, use `git worktree add ../pet-esp-vector vector-art-v1`. See [rollback instructions](backup/vector-v1/README.md).

## Playing

- **Food:** pick an illustrated snack; your pet eats it.
- **Play:** tap five golden stars. Each waits for you; there is no timer or losing.
- **Sleep:** a six-second nap restores energy. Back cancels the nap.
- **Bath:** pop five big bubbles.
- Tap your pet for a cuddle and a little hop.
- Each completed activity earns one saved star. Collect six stickers, one every five stars, via the star button at home. Growth adds a tuft, flower, scarf and golden badge at 10, 30, 60 and 100 stars.
- The matching coloured bars show food, happiness, energy and cleanliness. They also open their activities.
- The cog opens battery information, the sound toggle and a 0–100% volume slider. Releasing the slider previews the level unless muted. Sound settings are session-local; volume starts at 35% after restart. There is no destructive reset button in the child-facing UI.

Needs decay gently while powered on (one point per 3 / 4 / 5 / 6 minutes), stop at 20, and pause while powered off. There is no death, lost progress or punishment for leaving the toy. Each care action restores 30 points, capped at 100.

## Build and flash on this Mac

```sh
./scripts/device.sh build
./scripts/device.sh -p /dev/cu.usbmodem101 flash
./scripts/device.sh -p /dev/cu.usbmodem101 monitor
```

The helper selects the Python interpreter matching the existing build cache. On another machine, install ESP-IDF 5.3.x, source `export.sh`, then use `idf.py` from `firmware/`. The BSP and LVGL resolve through the component manager. The pixel creature is drawn in C; the two room images are compiled RGB565 assets. Their generated header is checked in, so building does not require image-generation tools. Legacy sprite tooling and the assets partition remain available for future work.

## Test and render without the device

```sh
cmake -S tests/host -B /tmp/pet-host-build
cmake --build /tmp/pet-host-build -j8
(cd docs/previews/pixel-v1 && /tmp/pet-host-build/pet_test)
python3 tests/host/render_previews.py docs/previews/pixel-v1
```

Uses the managed LVGL source installed by the firmware build, with mocked NVS/audio/power and actual LVGL pointer input. Tests cover decay boundaries, need floors, capped restoration, save/reload, all growth thresholds, activity completion, duplicate taps, cancelled feeding/naps, mute and repeated navigation. The PNG previews are renders of the actual C UI, not separate mockups. Pillow is needed only to convert the test's PPMs to PNG.

## Implementation

- `firmware/components/ui/ui.c`: screens, animation and activities; one LVGL timer owns the pet's periodic updates.
- `firmware/components/ui/pixel_pet.c`: the 56 × 56 pixel pet and icon artwork.
- `art/pixel-v1/`: approved concept, generated room source images and device-size previews; regenerate their header using `python3 scripts/pack_pixel_rooms.py`.
- `firmware/components/pet_state/`: backwards-compatible NVS pet blob. Previously unused `evolution_progress` stores care stars; no struct layout change.
- `firmware/components/audio/`: quiet, asynchronous codec chirps; mute is session-local.
- `firmware/components/renderer/`: BSP display/touch initialization and retained legacy sprite renderer.
- `docs/little-meadow.md`: design decisions, validation and limitations.

ESP-NOW multiplayer, breeding, IMU gestures and RTC synchronization remain unimplemented; the finished play loop is single-device.

## Recovery

The firmware that was on the connected device before this update is backed up locally at `backup/2026-09-19/pre-pet-flash.bin` (16 MB). See that folder's README for restoration. Flashing the pet does not erase NVS; valid existing pet identity, genes and progress are retained.
