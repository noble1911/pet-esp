// ui implementation — build-order step 2 hello-world: a rectangle that
// follows the finger. The pet-aware UI loop (stat bars, feed button, menus)
// lands at step 6.

#include "ui.h"
#include "renderer.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "ui";
static lv_obj_t *s_cursor;

// Centers the cursor on the active indev's current point. Fires on every
// LVGL frame while the screen is being pressed — this is the simplest LVGL
// pattern for "follow my finger" without writing a custom input device.
static void cursor_follow_cb(lv_event_t *e)
{
    (void)e;
    lv_indev_t *indev = lv_indev_active();
    if (indev == NULL) return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_obj_set_pos(s_cursor, p.x - 24, p.y - 24);  // 48x48 centered
}

void ui_init(void)
{
    if (!renderer_lock(0)) {
        ESP_LOGE(TAG, "could not lock LVGL");
        return;
    }

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101820), 0);

    s_cursor = lv_obj_create(scr);
    lv_obj_set_size(s_cursor, 48, 48);
    lv_obj_set_pos(s_cursor, 160, 200);
    lv_obj_set_style_bg_color(s_cursor, lv_color_hex(0xff4080), 0);
    lv_obj_set_style_border_width(s_cursor, 0, 0);
    lv_obj_set_style_radius(s_cursor, 8, 0);

    lv_obj_add_event_cb(scr, cursor_follow_cb, LV_EVENT_PRESSING, NULL);

    renderer_unlock();
    ESP_LOGI(TAG, "hello-world cursor wired (build-order:2)");
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
