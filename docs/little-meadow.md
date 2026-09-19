# Little Meadow — September 2026 revision

This revision supersedes the old pixel-art, text-free and real-time/offline-decay requirements in architecture.md and design.md for the single-device game. The existing hardware, framework, gene identity, storage layout and future radio protocol are retained.

## Why the game changed

The old Play action only incremented a need, other actions mostly did the same, and development decay emptied hunger in about 17 minutes. The UI mixed enlarged sprite packs and lacked a play objective. The replacement is an illustrated touch toy: snack choice, a five-target star hunt, a five-bubble bath and a short nap. Activities are forgiving, untimed (except the automatic nap) and always escapable. Instructions supplement recognizable pictures; action targets are 70–96 pixels and back buttons 48 pixels.

Native LVGL rounded shapes produce a consistent palette and crisp artwork at 368 × 448. The pet breathes, blinks and hops; genes choose one of six coats. Growth brings permanent visible decorations. Stars unlock a six-sticker album and cannot be spent or lost. Replay continues after the album is full. Activities have no streaks, deadlines or random paid rewards.

## State and timing

The existing unused `evolution_progress` field now stores completed care stars (saturating uint32). Stages are derived at 10 / 30 / 60 / 100. `Pet` size, schema and gene values are unchanged. NVS writes once per completed care and when a need changes; no writes happen on animation frames or individual game taps. A nap or feeding sequence cancelled with Back gives no reward. Screens have no independent timers: a single LVGL timer checks activity deadlines and updates needs, so destroyed screens cannot leave callbacks or rewards behind. This also removes the old esp_timer/LVGL concurrent mutation of the pet.

Needs fall one point per 180 / 240 / 300 / 360 active seconds, with a floor of 20. Care adds 30, capped at 100. Loading clamps old exhausted/out-of-range needs, retains identity, derives growth and resets the time baseline. No RTC is synchronized, so offline decay is deliberately disabled. Fractional need-decay time is RAM-only, as before. Long forward clock jumps are capped at an hour per tick; backward jumps do nothing. Mute is session-local.

## Hardware startup repair

The managed BSP declares both display and touch reset GPIOs as NC and never operates their expander reset lines in `bsp_display_start`. An initial warm-reset test failed touch initialization and rebooted before recovering. The renderer now explicitly pulses expander pins 0/1/2 low for 20 ms, raises them, and waits 200 ms before BSP display/touch startup. This follows the original-board reset sequence in [Waveshare's LVGL example](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.8/blob/main/examples/arduino/examples/08_LVGL_Animation/ui/ui.ino). The change is in our component, not a patch to generated managed dependencies.

## Validation and limits

The host harness runs the production UI and pet-state C against the same managed LVGL source. It injects pointer presses/releases, checks activity transitions and exact reward counts, tests cancellation and growth boundaries, and renders every screen for visual inspection. Hardware build and USB flash are tested separately; boot logs confirm display, touch, PSRAM, PMIC and codec startup. Physical finger accuracy, audible volume and enjoyment still require hands-on play; host pointer tests and serial logs cannot measure those.

Legacy sprite files, radio skeleton and breeding API are retained, but this release does not use multiplayer or the old layered pet renderer. There is no ambient day/night clock, motion input, automatic display sleep or battery-life claim.

## Pet artwork refinement

The pet now uses an oversized shaded head, a separate belly and paws, a cream muzzle, soft coloured outlines and asymmetric tilted ears. Six matched light/midtone/outline palettes preserve gene-based coat colour. Larger eyes have moving pupils and two highlights; curved eyelids replace compressed eye rectangles when blinking or sleeping. Cuddles and celebrations use smiling eyes, an open mouth, a floating heart, fluttering ears and moving paws. Feeding alternates the mouth for chewing; sleeping slows breathing. Changing screens clears transient celebration state.

Growth accessories are scaled to preserve the face: a small tuft, an ear flower, a scarf and a golden badge. The real LVGL previews in `previews/pet-refresh/` show before/after and all coats/stages. Host tests now check cuddle and sleeping expressions as well as the existing gameplay checks. Firmware builds and address/undefined-behavior sanitizer runs pass with the new artwork.
