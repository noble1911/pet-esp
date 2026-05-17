# Virtual Pet Project — Architecture

A Tamagotchi-Uni-inspired virtual pet running on ESP32-S3, with two devices able to meet wirelessly and share a play space across their screens.

This document is the design source of truth. Implementation should follow these decisions unless explicitly revised here.

---

## 1. Hardware

**Device:** Waveshare ESP32-S3-Touch-AMOLED-1.8 (also sold by The Pi Hut).

Relevant specs:

- ESP32-S3 dual-core Xtensa LX7 @ 240 MHz (silicon rev v0.2 on our unit)
- 16 MB flash (Winbond W25Q128, QIO), 8 MB embedded PSRAM (octal), 512 KB SRAM
- 1.8" AMOLED, 368 × 448, 16.7M colours
  - Display driver: SH8601 (QSPI). Reset routed via the TCA9554 I/O expander, not a direct GPIO.
  - Touch controller: capacitive, Focaltech FT5x06-compatible. The chip is marked FT3168 in the product brief, but Waveshare's BSP drives it with `esp_lcd_touch_ft5x06` (register-compatible). Touch reset also routed via the TCA9554.
- 6-axis IMU: **QMI8658** (accelerometer + gyroscope, I²C)
- External RTC: **PCF85063** on I²C, with battery backup pads. More accurate over week-scale power-off intervals than the S3's built-in RTC — important for the tick-on-wake model in §4.2.
- Speaker + microphone via **ES8311** audio codec (I²S)
- microSD slot (SDMMC)
- Wi-Fi 2.4 GHz + Bluetooth 5 (LE) — onboard antenna
- USB-C (native USB-Serial/JTAG on the S3, no bridge chip), LiPo battery header (MX1.25)
- Power management: **AXP2101** PMIC (USB-C charging, rail switching)
- I/O expander: **TCA9554** on I²C. Display reset, touch reset, and backlight enable all route through this expander rather than direct GPIOs — any driver that resets the display must bring up the expander first.
- Case included; no physical buttons beyond reset/boot

**Input model:** capacitive touch (primary), IMU for shake/tilt gestures, microphone reserved for later.

---

## 2. Software stack

- **Framework:** native ESP-IDF (`idf.py`) pinned to the **5.3.x LTS line**. Recommended IDE is VS Code with the Espressif ESP-IDF extension. v6.x is *not* supported until Waveshare publishes a BSP release that's been tested against it — internal API drift between 5.3 and 6.0 breaks BSP 1.1.3 in ways that can't be patched in one place. Arduino-ESP32 is no longer a documented fallback.
- **Board support:** Waveshare's published BSP (`waveshare/esp32_s3_touch_amoled_1_8`) is consumed as a managed component. It provides init for the display (SH8601 over QSPI), touch (FT5x06), audio (ES8311 over I²S), SD/MMC, and the TCA9554 I/O expander.
- **GUI:** LVGL (v9.x). Handles sprite blitting, animation, touch input. The BSP pins LVGL to `>=8,<10`, so 9.x is the target.
- **Persistence:** ESP-IDF NVS for pet state and small key-values. Sprite assets and sound files are flash-resident for the MVP (steps 1–7) — sprites ship in a LittleFS `assets` partition; SD is reserved for the extended asset library once we outgrow flash.
- **Networking:** ESP-NOW for device-to-device. Wi-Fi STA and BLE reserved for later (phone companion app, OTA).
- **Audio:** ES8311 codec via ESP-IDF audio drivers. Sample format and source (synth vs. PCM) still open — see §11.
- **Build:** `idf.py` with ESP-IDF Component Manager. Sprite assets built by a separate Python tool (see §8).

---

## 3. Core concept

A virtual pet that:

- Lives forever (no death), evolves through life stages, has genetic traits inherited from parents.
- Ages and accrues needs in real time via the RTC, including while powered off.
- Communicates through emoji emotes rather than text, making it readable for kids and language-independent.
- Can detect nearby pets via ESP-NOW and merge screens into a shared play space, where pets walk between devices, exchange items, play together, and breed.

Inspired by Tamagotchi Uni; visually closer to a layered pixel-art creature toy.

---

## 4. Pet model

### 4.1 State struct

Single struct, persisted as a single NVS blob. Designed to be small (under 256 bytes) for fast atomic writes.

```c
typedef struct {
  // Identity
  uint64_t pet_id;           // Unique ID generated at hatching
  char     name[16];
  uint32_t birth_timestamp;  // Unix time of hatching
  uint32_t last_tick;        // Unix time of last state update

  // Genetics (see §6)
  uint8_t  genes[8];
  uint8_t  generation;
  uint64_t parent_a;
  uint64_t parent_b;

  // Life stage
  uint8_t  stage;            // 0=egg, 1=baby, 2=child, 3=teen, 4=adult, 5=elder
  uint32_t evolution_progress;

  // Needs (0-100, decay over time)
  uint8_t  hunger;
  uint8_t  happiness;
  uint8_t  energy;
  uint8_t  hygiene;

  // Inventory
  uint8_t  inventory[16];    // Item IDs, 0 = empty slot

  // Social
  uint16_t friends_met;          // Lifetime counter
  uint64_t recent_friends[8];    // For meeting cooldown

  // Schema version (for forward compat)
  uint8_t  version;
} Pet;
```

### 4.2 Tick model

The pet does **not** tick continuously. Instead, on every wake/boot and on a slow periodic timer (~60s) it computes:

```
now = RTC.now()
elapsed = now - pet.last_tick
apply_decay(elapsed)
check_evolution()
pet.last_tick = now
save_if_dirty()
```

This means a device powered off for a week wakes up to a very hungry, lonely pet. Decay rates are tunable constants per need.

### 4.3 Evolution

Stages: egg → baby → child → teen → adult → elder.

Transition triggers:

- **Time-based:** minimum days in stage.
- **Care-based:** average happiness/hygiene above threshold.
- **Branching:** stat profile at transition time selects between alternative forms within a stage (e.g. high-energy adults vs high-happiness adults look different).

Evolution does not reset genes — the same genes pick a different sprite from the new stage's library.

---

## 5. Rendering

### 5.1 Layered sprite composition

Every pet at every frame is a stack of transparent sprite layers, drawn back-to-front:

```
Layer 6: Accessory      (hat, glasses — optional)
Layer 5: Pattern        (stripes, spots — tinted)
Layer 4: Eyes           (shape + tint)
Layer 3: Mouth          (shape)
Layer 2: Ears/Horns     (shape + body tint)
Layer 1: Tail           (shape + body tint)
Layer 0: Body           (shape + primary tint)
```

### 5.2 Tinting

Part sprites are authored in grayscale + alpha. At draw time, grayscale values are mapped to shades of a tint colour from a palette indexed by genes. This means one body sprite × 16 colour palette entries = 16 visual variants from a single asset.

### 5.3 Anchor points

Each body shape defines anchor offsets for where each upper layer attaches (eyes, ears, mouth). Stored per-shape in metadata alongside sprites so layers compose correctly regardless of body proportions.

### 5.4 Frame budget

Pet rendered at ~128×128 px in a 368×448 display.

- Each part sprite ~64×64 px, 2–4 frames per animation.
- All sprites for the active pet + visiting pet held in PSRAM (~500 KB typical).
- Sprite library is flash-resident (a LittleFS `assets` partition) for the MVP (§2); loaded into PSRAM on hatch/evolution/meeting. SD hosts the extended library once flash is outgrown.

### 5.5 Animation states

Per body shape: `idle`, `walk`, `sleep`, `eat`, `play`.
Per eyes: `normal`, `blink`, `happy`, `sad`, `surprised`.
Per mouth: `neutral`, `smile`, `open`, `frown`.

Frames advance on a fixed timer (~5 fps for sprite animation; UI overlay runs at 30 fps).

### 5.6 Sprite file format

Custom binary `.bin` per sprite sheet:

```
4 bytes  magic "PSPR"
1 byte   width
1 byte   height
1 byte   num_frames
1 byte   format  (0 = grayscale+alpha, 1 = palettized, 2 = RGB565)
N bytes  pixel data (width * height * frames * bytes_per_pixel)
```

No PNG decoder needed on device. Built by sprite forge tool (§8) from PNG sources.

---

## 6. Genetics

### 6.1 Gene structure

8 bytes, each indexing into a part or palette table:

| Byte | Meaning           | Range  |
|------|-------------------|--------|
| 0    | body_shape        | 0–7    |
| 1    | body_color        | 0–15   |
| 2    | eye_shape         | 0–7    |
| 3    | eye_color         | 0–15   |
| 4    | ear_shape         | 0–7    |
| 5    | mouth_shape       | 0–7    |
| 6    | pattern           | 0–7    |
| 7    | personality       | 0–7    |

`personality` does not affect visuals; it biases emote frequencies and behaviour (see §7.4).

### 6.2 Breeding

Both pets must be `adult` stage. Each emits its `genes[8]` array. Both devices independently compute the same child genes using a deterministic mixer:

```c
uint32_t seed = hash(min(pa.pet_id, pb.pet_id),
                     max(pa.pet_id, pb.pet_id),
                     session_timestamp);
for (int i = 0; i < 8; i++) {
  child.genes[i] = (rand(seed) & 1) ? pa.genes[i] : pb.genes[i];
  if (rand(seed) % 100 < MUTATION_PCT) {
    child.genes[i] = rand(seed) % GENE_MAX[i];
  }
  seed = next_rand(seed);
}
```

Generation increments. Parent IDs recorded. One device "carries" the egg (chosen at random or by user); egg hatches after a real-world delay (e.g. 24h).

Cooldown: same pet can breed once per 24h; same pair once per 7 days.

---

## 7. Shared canvas (meeting protocol)

### 7.1 Discovery

Every device broadcasts a small beacon every 5 seconds via ESP-NOW on a fixed channel:

```c
typedef struct {
  uint32_t protocol_ver;
  uint64_t pet_id;
  uint8_t  generation;
  uint8_t  stage;
  uint8_t  mood;
  uint8_t  genes[8];     // For preview rendering before handshake
  char     name[16];
} Beacon;
```

When a beacon arrives for a `pet_id` not in `recent_friends` cooldown (1 hour), the pet plays a "!" notification animation. User must tap to engage.

### 7.2 Handshake

Both users tap to opt in. Devices exchange session info:

- Both `pet_id`s
- World host election: lowest `pet_id` is host for the session
- Session timestamp (host's RTC)
- Negotiated tick rate (default 10 Hz state sync)

Once handshake completes, both devices enter **shared canvas mode**.

### 7.3 Shared canvas

Conceptually one wide play area split across two physical screens.

- World coordinate system: 800 units wide × 448 units tall.
- Device A renders world x = 0..400; Device B renders x = 400..800.
- A pet at x ∈ [380, 420] renders partially on both devices (the "crossing the gap" effect).
- Host is authoritative on tick counter; non-host syncs to host periodically.

Synced state per pet (sent on change, not per frame):

```c
typedef struct {
  uint8_t  pet_slot;     // 0 or 1
  int16_t  x, y;
  int8_t   vx, vy;
  uint8_t  facing;       // 0=left, 1=right
  uint8_t  action;       // idle, walk, sit, eat, sleep, dance
  uint8_t  emote;        // 0 = none, else emote ID
  uint32_t tick;         // when this state began
} PetUpdate;
```

Plus a full snapshot every 2 seconds as a heartbeat / lost-packet recovery.

### 7.4 Emote vocabulary

Pets communicate via emoji emotes. No text, ever.

| ID  | Glyph | Meaning              |
|-----|-------|----------------------|
| 1   | 👋    | Wave / hello / bye    |
| 2   | 💕    | Affection             |
| 3   | ✨    | Excited               |
| 10  | 🍎    | Hungry / want food    |
| 11  | 💤    | Sleepy / tired        |
| 12  | 💧    | Thirsty               |
| 20  | 👍    | Yes / agree           |
| 21  | 👎    | No / refuse           |
| 22  | ❓    | Confused              |
| 23  | ❗    | Surprised             |
| 24  | 😂    | Happy laugh           |
| 25  | 😢    | Sad                   |
| 30  | ⚽    | Wanna play?           |
| 31  | 🎁    | Wanna trade?          |
| 32  | 🎵    | Wanna dance?          |

Emotes appear as a speech bubble above the pet for ~2 seconds. Both devices render the bubble at the same time because the emote is part of synced state.

### 7.5 Reactive behaviour

The receiving pet's reaction is rule-driven, biased by its `personality` gene:

| Incoming | Possible reactions                              |
|----------|-------------------------------------------------|
| 👋        | 👋, 💕                                          |
| 🍎        | 🎁 (if has food), 😢 (if also hungry)           |
| ⚽        | 👍 → start mini-game, 👎, 💤                    |
| 🎁        | 👍, ❓                                          |
| 💕        | 💕, ✨                                          |
| 🎵        | 👍 → start dance, 👎                            |

Personality byte tweaks weights (e.g. a shy pet has high ❓ weight; an energetic pet has high ✨/⚽).

### 7.6 Shared activities

Reachable from emote interactions, not menus:

- **Walk together** — implicit; pets just move in shared space.
- **Drop & share items** — drag item from inventory onto shared canvas; the other pet can pick up.
- **Mini-game** — synchronised rhythm/reflex game; host generates sequence, both run identical loop.
- **Dance** — both screens flash to the same beat; both pets bob in sync.
- **Mirror** — IMU shake on one device → both pets jump.
- **Breeding ritual** — special animation when breed is initiated.

### 7.7 Anti-griefing

- Trades are atomic: each side decrements inventory locally, awaits peer ACK before committing.
- Never trust incoming stat values. Only `pet_id`, `genes`, `name`, item IDs are exchanged.
- Friend cooldown (1h same pet, 24h same breed pair, 7d same breeding pair).
- Both users must tap to initiate any meeting.

### 7.8 Disconnection

If RSSI drops below threshold or no packets received for 5 seconds:

- Pet on "remote" half of canvas walks home with a 👋 emote.
- Local pet exits shared mode gracefully.
- Friendship counter still increments.

---

## 8. Sprite forge (tooling)

A Python tool that runs on the developer's laptop. **Not** firmware.

Responsibilities:

- Load part PNGs from `sprite_forge/parts/{stage}/{layer}/{shape}/{animation}.png`
- Load palettes from `sprite_forge/palettes/*.png`
- Given a gene vector, compose a preview pet (PIL composite + tinting)
- Show a grid of N random pets for quick visual review
- Build the `.bin` sprite files into `firmware/assets/` matching the on-device format
- Validate anchor metadata

Run on every art change before flashing. Lets art iteration happen without a device in the loop.

Suggested CLI:

```
python sprite_forge/build.py        # build all .bin assets
python sprite_forge/preview.py      # interactive preview window
python sprite_forge/random_grid.py  # save a 4x4 PNG of random pets
```

---

## 9. Repository layout

```
pet-project/
  firmware/
    platformio.ini
    src/
      main.c
    components/
      pet_state/        Pet struct, NVS save/load, decay, evolution
      renderer/         LVGL display driver, sprite loader, composition
      radio/            ESP-NOW: beacon, handshake, sync, protocol
      ui/               Screens, menus, touch handling
      audio/            Beep playback
    assets/             Built .bin sprite files (gitignored; built by sprite_forge)
  sprite_forge/
    parts/              Source PNG art
    palettes/
    build.py
    preview.py
  docs/
    architecture.md     (this file)
    design.md           visual & UX direction (polish-gate authority)
    gene_spec.md
    radio_protocol.md
    sprite_format.md
    references/         mood-board screenshots (gitignored if heavy)
```

---

## 10. Build order

Each step ends with a working, demoable artifact. Do not skip ahead.

1. **Hardware smoke test.** Flash Waveshare's factory demo. Confirm display, touch, IMU, SD all work.
2. **LVGL hello world.** Minimal LVGL app drawing a rectangle following touch.
3. **Pet exists.** Static placeholder sprite (a circle) drawn on screen.
4. **Pet ticks.** Pet has hunger; debug overlay shows the number; decays over time; survives reboot via NVS.
5. **Real renderer.** Layered sprite composition with tinting, loading from the flash LittleFS `assets` partition (SD deferred to the extended library — §2). Placeholder art OK.
6. **UI loop.** Stat bars, feed button, basic menus, touch input. Now it's a pet.
7. **Evolution.** Sprites change at life-stage transitions.

**Polish gate — the single-device pet must feel like a toy before networking begins.**

This is a basket of small tracks, not a numbered step — items can land in any
order after step 6. Step 8 (and everything after it) is on hold until the
single-device experience reads as a toy rather than a tech demo. Concretely:

- **Move.** Sprite animation cycles at ~5 fps (§5.5); idle frames loop, eye
  blink fires every few seconds, and care actions briefly switch the body to
  `eat` / `play` / `sleep` / `clean` states. The pet visibly breathes or
  drifts within its area, never holds a static frame.
  - ✓ idle frame cycling at 5 fps (`2a11c48`)
  - ✓ tap-the-pet hop + differentiated care reactions (hop/wiggle/shake) + idle breathe (`795ad86`)
  - ✓ horizontal wander on the floor — random target every 2-3.5 s within `[56, 240]` object-space X, ease_in_out, pauses during wiggle/shake reactions (`33b474b`)
  - pending: eye blink (needs `blink` eye animation art)
  - pending: per-care body state overrides (needs `eat`/`sleep`/`play`/`clean` body art)
  - pending: 2D wander (depth) — gated on Track S scene art with perspective
- **Vary.** Fresh hatches get random genes within their valid ranges; palettes
  have multiple meaningful entries. Two devices' default pets should be
  unmistakably different at first glance.
  - ✓ random gene seeding at hatch + back-fill migration for zero-gene pets (`714e45c`)
  - pending: multi-entry palettes (art-side)
- **Care visibly.** Each care action plays a short audio cue from the ES8311
  codec in addition to the transient animation state. Resolves the §11 sound
  design question by forcing a concrete choice (synth vs. sampled).
  - ✓ Feed: 5-item picker, food sprite drops on screen, hunger restored after 1.5 s (`69c4ce2`)
  - ✓ Rest: dim overlay + animated "Z z z" for 6 s, energy restored (`7167ae8`)
  - ✓ Care-action emote bubbles: heart (Feed) / star (Play) / zzz (Rest) / drop (Clean), 2 s auto-dismiss, anchored above the pet's live position (`9e0cc1b`) — single-device slice of the §7.4 emote vocabulary; full system lands at build-order step 10.
  - pending: Play body-state art (currently only the wiggle reaction + emote), Clean shower art, audio cues (audio component still stubbed)
  - pending: mood-based emote selection from stats + personality (EM-4..7 in §7.4)
- **Look like a toy.** See `docs/design.md` for the full visual identity
  and layout. Short version: a scene around the pet (not a void), an icon
  vocabulary in place of text labels, a compact stat strip plus an action
  dock, and a corner menu. Tamagotchi-Uni as the primary reference.
  - ✓ UNSCII pixel font as project default, sharp-cornered stat bars, pixel-bordered care buttons (`4e23612`)
  - pending: see `design.md` §9 implementation tracks (L → I → S → M → F → X)
- **Art coverage.** Multiple body / eye / mouth / ear / pattern / accessory
  shapes per layer (`gene_spec.md` allows 8 per layer + 16 palette entries).
  Without this, gene variation has nothing to express no matter how good the
  renderer is.
  - parallel session — additions land independently as the art track produces them.

8. **ESP-NOW discovery.** Two devices detect each other and play a 👋 animation. Make-or-break milestone.
9. **Shared canvas, no interaction.** Two devices show one pet walking across both screens. Position sync working.
10. **Emote system.** Tap emotes; both screens render bubbles; reactions fire. (Single-device groundwork — converter, `renderer_show_emote`, care-action bubbles — landed during the polish gate, `9e0cc1b`. This step adds the wire sync, tap-emote UI, and mood-driven selection on top.)
11. **Items in shared space.** Drag, drop, pick up, cross-device.
12. **Mini-game + dance.** Synchronised activities.
13. **Breeding.** Egg hatches into gene-mixed offspring.

---

## 11. Open questions / decisions deferred

- Exact decay rates per need.
- How egg hatching is gated (real time only? or speed-up by interaction?).
- Whether to support more than two pets in shared canvas (start with two).
- Phone companion app: Wi-Fi server vs BLE direct (not in MVP).
- OTA update strategy (not in MVP).
- Sound design: synthesised tones vs sampled clips on SD.

---

## 12. Glossary

- **Beacon** — periodic ESP-NOW broadcast advertising a pet's presence.
- **Tick** — discrete update of pet needs based on elapsed real time.
- **Gene vector** — the 8-byte `genes[]` array fully describing a pet's appearance and personality.
- **Host** — in a shared session, the device elected to be authoritative on tick/world state.
- **Sprite forge** — laptop-side Python tool for building sprite assets from PNG sources.
