// ui — touch-driven screens: stat bars, feed/care actions, menus, the
// "!" meeting prompt, and emote selection/bubbles.
//
// References: architecture.md §7.4 (emotes), §10 step 6 (UI loop).
// No text in pet communication — emotes only (architecture §7.4).

#pragma once

#include <stdint.h>
#include "pet_state.h"

#ifdef __cplusplus
extern "C" {
#endif

// Emote IDs — wire-stable, mirror docs/radio_protocol.md (architecture §7.4).
typedef enum {
    EMOTE_NONE      = 0,
    EMOTE_WAVE      = 1,   // 👋
    EMOTE_AFFECTION = 2,   // 💕
    EMOTE_EXCITED   = 3,   // ✨
    EMOTE_HUNGRY    = 10,  // 🍎
    EMOTE_SLEEPY    = 11,  // 💤
    EMOTE_THIRSTY   = 12,  // 💧
    EMOTE_YES       = 20,  // 👍
    EMOTE_NO        = 21,  // 👎
    EMOTE_CONFUSED  = 22,  // ❓
    EMOTE_SURPRISED = 23,  // ❗
    EMOTE_LAUGH     = 24,  // 😂
    EMOTE_SAD       = 25,  // 😢
    EMOTE_PLAY      = 30,  // ⚽
    EMOTE_TRADE     = 31,  // 🎁
    EMOTE_DANCE     = 32,  // 🎵
} emote_id_t;

// Build LVGL screens + input. Safe no-op until build-order step 6.
void ui_init(void);

// Re-read pet_state and refresh the debug stat overlay. Cheap; safe to
// call from any task (locks LVGL internally). Step 4 surfaces hunger.
void ui_refresh_stats(void);

// TODO(build-order:6): main screen — stat bars, feed button, menus.
void ui_show_home(void);

// TODO(build-order:10): show an emote speech bubble for ~2 s.
void ui_show_emote(emote_id_t emote);

#ifdef __cplusplus
}
#endif
