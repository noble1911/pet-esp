// radio — device-to-device over ESP-NOW: discovery beacon, handshake,
// shared-canvas state sync, disconnection handling.
//
// References: architecture.md §7, docs/radio_protocol.md.
// Wi-Fi STA and BLE are reserved for later and must NOT be started here.

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "pet_state.h"

#ifdef __cplusplus
extern "C" {
#endif

// Bump on any wire-format change; mismatches are ignored silently.
#define RADIO_PROTOCOL_VER 1u

// Broadcast every 5 s (architecture §7.1).
typedef struct __attribute__((packed)) {
    uint32_t protocol_ver;
    uint64_t pet_id;
    uint8_t  generation;
    uint8_t  stage;
    uint8_t  mood;
    uint8_t  genes[8];     // preview render before handshake
    char     name[16];
} Beacon;

// Shared-canvas per-pet state, sent on change (architecture §7.3).
typedef struct __attribute__((packed)) {
    uint8_t  pet_slot;     // 0 or 1
    int16_t  x, y;
    int8_t   vx, vy;
    uint8_t  facing;       // 0=left, 1=right
    uint8_t  action;       // idle, walk, sit, eat, sleep, dance
    uint8_t  emote;        // 0 = none, else emote ID (architecture §7.4)
    uint32_t tick;         // tick when this state began
} PetUpdate;

// Bring up ESP-NOW only. Safe no-op until build-order step 8.
void radio_init(void);

// TODO(build-order:8): start/stop the 5 s discovery beacon.
void radio_start_beacon(void);
void radio_stop_beacon(void);

// TODO(build-order:8): begin handshake after both users opt in;
//                      lowest pet_id becomes host.
bool radio_begin_handshake(uint64_t peer_pet_id);

// TODO(build-order:9): push/apply shared-canvas state + 2 s heartbeat.
void radio_send_update(const PetUpdate *u);

#ifdef __cplusplus
}
#endif
