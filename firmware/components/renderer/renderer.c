// renderer implementation — display + LVGL bring-up via the Waveshare BSP.
// Sprite loading (step 5) and gene-tinted layered composition (step 3/5)
// remain stubbed.

#include "renderer.h"
#include "esp_log.h"
#include "bsp/esp-bsp.h"

static const char *TAG = "renderer";
static lv_display_t *s_disp;

void renderer_init(void)
{
    ESP_ERROR_CHECK(bsp_i2c_init());
    s_disp = bsp_display_start();
    if (s_disp == NULL) {
        ESP_LOGE(TAG, "bsp_display_start failed");
        return;
    }
    bsp_display_brightness_set(80);
    ESP_LOGI(TAG, "display + touch + LVGL up (%" PRId32 "x%" PRId32 ")",
             lv_display_get_horizontal_resolution(s_disp),
             lv_display_get_vertical_resolution(s_disp));
}

bool renderer_lock(uint32_t timeout_ms)
{
    return bsp_display_lock(timeout_ms);
}

void renderer_unlock(void)
{
    bsp_display_unlock();
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
