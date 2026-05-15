# Radio protocol

Expands [architecture.md §7](architecture.md). The architecture doc is
authoritative; this file is the working reference for the `radio`
component.

Transport: **ESP-NOW**, fixed Wi-Fi channel, 2.4 GHz. Wi-Fi STA and BLE
are reserved for later and must not be brought up by this component.

`PROTOCOL_VER` is sent in every beacon. Mismatched versions ignore each
other silently.

## 1. Discovery — Beacon (broadcast every 5 s)

```c
typedef struct {
  uint32_t protocol_ver;
  uint64_t pet_id;
  uint8_t  generation;
  uint8_t  stage;
  uint8_t  mood;
  uint8_t  genes[8];   // preview render before handshake
  char     name[16];
} Beacon;
```

On receiving a beacon whose `pet_id` is not under the 1 h friend
cooldown, the pet plays a "!" notification. The user must tap to engage —
nothing happens automatically.

## 2. Handshake

Both users tap to opt in, then devices exchange:

- both `pet_id`s
- **host election**: lowest `pet_id` is host for the session
- `session_timestamp` (host's RTC)
- negotiated tick rate (default 10 Hz state sync)

On completion both devices enter shared canvas mode.

## 3. Shared canvas sync

World is 800 × 448 units. Device A renders x = 0..400, device B
x = 400..800; a pet in x ∈ [380, 420] straddles both screens.

State is sent **on change**, not per frame:

```c
typedef struct {
  uint8_t  pet_slot;   // 0 or 1
  int16_t  x, y;
  int8_t   vx, vy;
  uint8_t  facing;     // 0=left, 1=right
  uint8_t  action;     // idle, walk, sit, eat, sleep, dance
  uint8_t  emote;      // 0 = none, else emote ID
  uint32_t tick;       // tick when this state began
} PetUpdate;
```

Plus a **full snapshot every 2 s** as heartbeat / lost-packet recovery.
Host is authoritative on the tick counter; non-host resynchronises
periodically.

## 4. Trust & anti-griefing

- Never trust incoming stat values. Only `pet_id`, `genes`, `name`, and
  item IDs are accepted from a peer.
- Trades are atomic: each side decrements locally and waits for a peer
  ACK before committing.
- Cooldowns: 1 h same pet, 24 h same breed pair, 7 d same breeding pair.
- Both users must tap to initiate any meeting.

## 5. Disconnection

If RSSI drops below threshold or no packets for 5 s:

- the remote-half pet walks home with a 👋 emote;
- the local pet exits shared mode gracefully;
- the friendship counter still increments.

## Emote vocabulary

IDs are wire-stable; see [architecture.md §7.4](architecture.md) for the
full table (1 👋, 2 💕, 3 ✨, 10 🍎, 11 💤, 12 💧, 20 👍, 21 👎, 22 ❓,
23 ❗, 24 😂, 25 😢, 30 ⚽, 31 🎁, 32 🎵). `emote = 0` means none.
