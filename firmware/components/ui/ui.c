// ui implementation — build-order step 3: pet exists on screen as a
// placeholder. Real screens (stat bars, feed button, menus) land at step 6.

#include "ui.h"
#include "renderer.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "ui";

void ui_init(void)
{
    if (!renderer_lock(0)) {
        ESP_LOGE(TAG, "could not lock LVGL");
        return;
    }

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101820), 0);

    // 128x128 pet placeholder centered on the 368x448 AMOLED. The render
    // footprint matches architecture §5.4's per-pet budget — genes and
    // animations slot into the same call site at step 5.
    renderer_draw_pet(pet_state_get(), 120, 160);

    renderer_unlock();
    ESP_LOGI(TAG, "pet placeholder drawn (build-order:3)");
}

void ui_show_home(void)
{
    // TODO(build-order:6): stat bars, feed/care actions, menus.
}

void ui_show_emote(emote_id_t emote)
{
    (void)emote;
    // TODO(build-order:10): ~2 s speech bubble; synced via radio.
}
