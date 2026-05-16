// ui implementation — build-order step 4: pet placeholder + debug
// hunger overlay. Stat bars, feed button, menus land at step 6.

#include "ui.h"
#include "renderer.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "ui";
static lv_obj_t *s_hunger_label;

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

    // Step 4 debug overlay — top-center, light gray, Montserrat 24 so the
    // value is readable from arm's length. Tick callback updates it via
    // ui_refresh_stats().
    s_hunger_label = lv_label_create(scr);
    lv_obj_align(s_hunger_label, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_text_color(s_hunger_label, lv_color_hex(0xe5e5e5), 0);
    lv_obj_set_style_text_font(s_hunger_label, &lv_font_montserrat_24, 0);
    lv_label_set_text(s_hunger_label, "Hunger: --");

    renderer_unlock();
    ESP_LOGI(TAG, "pet + hunger overlay drawn (build-order:4)");
}

void ui_refresh_stats(void)
{
    const Pet *p = pet_state_get();
    if (p == NULL || s_hunger_label == NULL) return;
    if (!renderer_lock(0)) return;
    lv_label_set_text_fmt(s_hunger_label, "Hunger: %d", p->hunger);
    renderer_unlock();
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
