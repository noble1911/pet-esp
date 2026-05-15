// renderer implementation — skeleton.
// LVGL bring-up is build-order step 2; layered composition is step 5.

#include "renderer.h"
#include "esp_log.h"

static const char *TAG = "renderer";

void renderer_init(void)
{
    // TODO(build-order:2): SH8601 QSPI panel + FT3168 touch + LVGL display.
    ESP_LOGI(TAG, "init (skeleton)");
}

bool renderer_load_pet_sprites(const Pet *pet)
{
    (void)pet;
    return false;  // TODO(build-order:5): SD -> PSRAM sprite library load
}

void renderer_draw_pet(const Pet *pet, int x, int y)
{
    (void)pet; (void)x; (void)y;
    // TODO(build-order:3): placeholder circle.
    // TODO(build-order:5): real back-to-front tinted layer stack.
}
