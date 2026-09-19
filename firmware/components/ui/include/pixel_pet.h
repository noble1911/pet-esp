#pragma once
#include "lvgl.h"
#include "pet_state.h"
#define PIXEL_PET_SIZE 72
typedef enum { PIXEL_IDLE, PIXEL_BLINK, PIXEL_HAPPY, PIXEL_EAT, PIXEL_SLEEP, PIXEL_TALK, PIXEL_LISTEN, PIXEL_THINK, PIXEL_BATH, PIXEL_PLAY } pixel_face_t;
typedef enum { PIXEL_APPLE, PIXEL_TOAST, PIXEL_COOKIE } pixel_food_t;
typedef struct {
    uint32_t pixels[PIXEL_PET_SIZE * PIXEL_PET_SIZE];
    lv_image_dsc_t image;
} pixel_pet_art_t;
void pixel_pet_render(pixel_pet_art_t *art, const Pet *pet, pixel_face_t face, unsigned phase, pixel_food_t food);
const lv_image_dsc_t *pixel_icon(unsigned kind);
const lv_image_dsc_t *pixel_collectible(unsigned kind); // full album, 0..17
const lv_image_dsc_t *pixel_decoration(unsigned kind); // room gifts, 0..5
