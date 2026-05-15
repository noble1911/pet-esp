// renderer — SH8601 AMOLED + LVGL display, SD sprite loader, and the
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

// Bring up display + LVGL. Safe no-op until build-order step 2.
void renderer_init(void);

// TODO(build-order:5): load a stage's sprite library from SD into PSRAM.
bool renderer_load_pet_sprites(const Pet *pet);

// TODO(build-order:3/5): compose the layered, gene-tinted pet on screen.
void renderer_draw_pet(const Pet *pet, int x, int y);

#ifdef __cplusplus
}
#endif
