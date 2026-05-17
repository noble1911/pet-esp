// renderer — SH8601 AMOLED + LVGL display, LittleFS sprite loader, and the
// back-to-front layered/tinted pet composition.
//
// References: architecture.md §5, docs/sprite_format.md.

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "pet_state.h"

#ifdef __cplusplus
extern "C" {
#endif

// .bin sprite sheet header (docs/sprite_format.md / architecture §5.6).
#define SPRITE_MAGIC "PSPR"

typedef enum {
    SPRITE_FMT_GRAY_ALPHA = 0,  // [gray, alpha], tinted at draw time
    SPRITE_FMT_PALETTIZED = 1,
    SPRITE_FMT_RGB565     = 2,
} sprite_format_t;

typedef struct {
    char    magic[4];     // "PSPR"
    uint8_t width;
    uint8_t height;
    uint8_t num_frames;
    uint8_t format;       // sprite_format_t
    // pixel data follows: width * height * num_frames * bpp
} sprite_header_t;

// Layer stack, drawn back (0) to front (6) — architecture §5.1.
typedef enum {
    LAYER_BODY      = 0,
    LAYER_TAIL      = 1,
    LAYER_EARS      = 2,
    LAYER_MOUTH     = 3,
    LAYER_EYES      = 4,
    LAYER_PATTERN   = 5,
    LAYER_ACCESSORY = 6,
    LAYER_COUNT     = 7,
} sprite_layer_t;

// ---- Loaded sprite set (build-order step 5) --------------------------

// One layer's sheet, resident in PSRAM. `pixels` is NULL when the pet has
// no art for this layer — the loader skips missing layers, never fails.
typedef struct {
    sprite_header_t hdr;
    uint8_t        *pixels;   // width*height*num_frames*bpp, in PSRAM
} sprite_t;

// Body-relative attach offsets, decoded from anchors.bin ("PANC").
// File format v1 carried 5 entries (eyes, mouth, ears, tail, accessory);
// v2 adds `pattern` at the end. v1 files load with pattern defaulting to
// (0, 0) — pattern positioning works without an offset because the
// renderer masks pattern writes to body alpha automatically.
typedef struct {
    int16_t eyes[2];
    int16_t mouth[2];
    int16_t ears[2];
    int16_t tail[2];
    int16_t accessory[2];
    int16_t pattern[2];
} sprite_anchors_t;

// The full set selected by a pet's genes + stage.
typedef struct {
    bool             valid;             // false until a successful load
    sprite_t         layer[LAYER_COUNT];
    sprite_anchors_t anchors;           // from the body shape
} pet_sprites_t;

// Gene-indexed tint table (architecture §5.2 / docs/sprite_format.md).
// Loaded once at boot from assets/palettes/{body,eye}.pal ("PPAL").
typedef struct {
    uint8_t   entries;     // 16
    uint8_t   ramp;        // shades per entry
    uint16_t *rgb565;      // entries*ramp, entry-major, in PSRAM
} palette_t;

// Bring up the AMOLED panel, FT5x06 touch, and the LVGL port via the
// Waveshare BSP. After this, an LVGL `lv_display_t` is the active display
// and a touch `lv_indev_t` is registered.
void renderer_init(void);

// Lock LVGL for thread-safe object manipulation; pair with renderer_unlock().
// timeout_ms = 0 means block indefinitely (the right choice during one-shot
// UI construction at boot).
bool renderer_lock(uint32_t timeout_ms);
void renderer_unlock(void);

// Load the sprite set selected by pet->genes/stage from the LittleFS
// `assets` partition into PSRAM. Frees any previously loaded set, skips
// layers with no art, and returns true if at least the body loaded.
// NOT reentrant — mutates one static set and uses static path scratch;
// call from a single task only (the UI/render task). (build-order step 5)
bool renderer_load_pet_sprites(const Pet *pet);

// The currently loaded set, or NULL if nothing is loaded yet.
const pet_sprites_t *renderer_pet_sprites(void);

// Tint helpers (build-order step 5 phase ③). Palettes are loaded at boot;
// the accessors return NULL if the .pal was missing/invalid (callers then
// fall back to renderer_gray_rgb565). `gray` is a format-0 pixel's gray
// byte; `entry` is the body_color/eye_color gene (taken modulo entries).
const palette_t *renderer_palette_body(void);
const palette_t *renderer_palette_eye(void);
uint16_t renderer_tint_rgb565(const palette_t *pal, uint8_t entry,
                              uint8_t gray);
uint16_t renderer_gray_rgb565(uint8_t gray);   // untinted layers

// Draw the pet at (x, y). Step 3 is a placeholder circle that ignores
// `pet`; step 5 will compose layered, gene-tinted sprites from the same
// call site. Caller must hold renderer_lock().
void renderer_draw_pet(const Pet *pet, int x, int y);

// Personality animations (polish-gate "Move"). The pet hops in response
// to a tap, reacts to each care action with a distinct movement, and
// breathes gently while idle. All anims oscillate around the base (x, y)
// recorded by the most recent renderer_draw_pet call.
typedef enum {
    REACT_NONE   = 0,
    REACT_HOP    = 1,   // small vertical jump (feed, tap-the-pet)
    REACT_WIGGLE = 2,   // X oscillation, playful (play)
    REACT_SHAKE  = 3,   // rapid X tremor, shorter throw (clean)
} pet_reaction_t;

void renderer_pet_react(pet_reaction_t kind);

// Pop an emote bubble above the pet for `duration_ms`, then auto-dismiss.
// Single-slot: a new call replaces any in-flight emote (no queue). The
// bubble is positioned above the pet's current base coordinates, so it
// follows wherever renderer_draw_pet last placed the pet. `emote_id`
// uses the IDs in ui.h (emote_id_t); IDs without art draw nothing.
// Caller must hold renderer_lock() — same threading rule as the rest
// of the renderer API.
void renderer_show_emote(uint8_t emote_id, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif
