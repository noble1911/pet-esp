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
typedef struct {
    int16_t eyes[2];
    int16_t mouth[2];
    int16_t ears[2];
    int16_t tail[2];
    int16_t accessory[2];
} sprite_anchors_t;

// The full set selected by a pet's genes + stage.
typedef struct {
    bool             valid;             // false until a successful load
    sprite_t         layer[LAYER_COUNT];
    sprite_anchors_t anchors;           // from the body shape
} pet_sprites_t;

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

// Draw the pet at (x, y). Step 3 is a placeholder circle that ignores
// `pet`; step 5 will compose layered, gene-tinted sprites from the same
// call site. Caller must hold renderer_lock().
void renderer_draw_pet(const Pet *pet, int x, int y);

#ifdef __cplusplus
}
#endif
