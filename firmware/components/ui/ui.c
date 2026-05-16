// ui implementation — build-order step 6: "now it's a pet". Stat bars at
// the top, pet placeholder in the middle, four care buttons at the bottom.
// Touch the buttons to restore the matching need; bars update immediately.

#include "ui.h"
#include "renderer.h"
#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "ui";

// Stat bar handles — index matches need_t order below.
enum { NEED_HUNGER, NEED_HAPPINESS, NEED_ENERGY, NEED_HYGIENE, NEED_COUNT };
static lv_obj_t *s_bars[NEED_COUNT];

// Bar styling — distinct hues per need so all four stay readable at a glance.
static const uint32_t s_bar_colors[NEED_COUNT] = {
    0xff8a40,  // hunger — warm orange (food)
    0xffd166,  // happiness — yellow
    0x4a9aff,  // energy — blue
    0x66d9c2,  // hygiene — cyan
};
static const char *s_bar_labels[NEED_COUNT] = {
    "Hunger", "Happiness", "Energy", "Hygiene"
};

// Care action callbacks. Each restores its need and immediately refreshes
// the bars — don't wait the up-to-10s for the next periodic tick.
static void on_feed_cb(lv_event_t *e)  { (void)e; pet_state_feed();  ui_refresh_stats(); }
static void on_play_cb(lv_event_t *e)  { (void)e; pet_state_play();  ui_refresh_stats(); }
static void on_rest_cb(lv_event_t *e)  { (void)e; pet_state_rest();  ui_refresh_stats(); }
static void on_clean_cb(lv_event_t *e) { (void)e; pet_state_clean(); ui_refresh_stats(); }

// Pixel-art aesthetic: sharp corners (radius 0), thin solid borders on
// every interactive element. Bars and buttons share the same color slot
// so the screen reads as one toy, not a mix of widget kits.
#define UI_FRAME_COLOR    0x4a5566      // muted slate — borders + bar bg
#define UI_BG_COLOR       0x101820      // screen background
#define UI_FG_COLOR       0xe5e5e5      // text on dark
#define UI_BTN_BG         0x202830      // care-button face
#define UI_BTN_BG_PRESSED 0x303a45      // pressed feedback

static lv_obj_t *make_stat_bar(lv_obj_t *parent, const char *name,
                               int y, uint32_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, name);
    lv_obj_set_style_text_color(label, lv_color_hex(UI_FG_COLOR), 0);
    lv_obj_set_pos(label, 8, y);

    lv_obj_t *bar = lv_bar_create(parent);
    lv_bar_set_range(bar, 0, 100);
    lv_obj_set_size(bar, 220, 14);
    lv_obj_set_pos(bar, 120, y + 4);
    // Sharp rectangle main + indicator; 1 px slate border around the bar.
    lv_obj_set_style_radius(bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x303744), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(color), LV_PART_INDICATOR);
    lv_obj_set_style_border_width(bar, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(bar, lv_color_hex(UI_FRAME_COLOR),
                                  LV_PART_MAIN);
    return bar;
}

static void make_care_button(lv_obj_t *parent, const char *text,
                             int x, int y, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 80, 70);
    lv_obj_set_pos(btn, x, y);
    // Pixel-art button: sharp corners, 2 px slate border, dark face with
    // a slightly lighter pressed state for tactile feedback.
    lv_obj_set_style_radius(btn, 0, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_BTN_BG), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UI_BTN_BG_PRESSED),
                              LV_STATE_PRESSED);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(UI_FRAME_COLOR), 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_hex(UI_FG_COLOR), 0);
    lv_obj_center(lbl);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
}

void ui_init(void)
{
    if (!renderer_lock(0)) {
        ESP_LOGE(TAG, "could not lock LVGL");
        return;
    }

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(UI_BG_COLOR), 0);

    // Stat bars — 4 rows of (label + bar), top 92px of the screen.
    for (int i = 0; i < NEED_COUNT; i++) {
        s_bars[i] = make_stat_bar(scr, s_bar_labels[i],
                                  4 + i * 22, s_bar_colors[i]);
    }

    // Pet placeholder — slightly higher than step 3 to make room for the
    // bars; still horizontally centered on the 368-wide AMOLED.
    renderer_draw_pet(pet_state_get(), 120, 130);

    // Care buttons — bottom row, ~80x70 each, 8px gutter; total width
    // (4*80 + 3*8) = 344, centered in 368 by starting at x=12.
    make_care_button(scr, "Feed",  12,  368, on_feed_cb);
    make_care_button(scr, "Play",  100, 368, on_play_cb);
    make_care_button(scr, "Rest",  188, 368, on_rest_cb);
    make_care_button(scr, "Clean", 276, 368, on_clean_cb);

    renderer_unlock();
    ui_refresh_stats();
    ESP_LOGI(TAG, "stat bars + care buttons up (build-order:6)");
}

void ui_refresh_stats(void)
{
    const Pet *p = pet_state_get();
    if (p == NULL || s_bars[0] == NULL) return;
    if (!renderer_lock(0)) return;
    lv_bar_set_value(s_bars[NEED_HUNGER],    p->hunger,    LV_ANIM_OFF);
    lv_bar_set_value(s_bars[NEED_HAPPINESS], p->happiness, LV_ANIM_OFF);
    lv_bar_set_value(s_bars[NEED_ENERGY],    p->energy,    LV_ANIM_OFF);
    lv_bar_set_value(s_bars[NEED_HYGIENE],   p->hygiene,   LV_ANIM_OFF);
    renderer_unlock();
}

void ui_show_home(void)
{
    // TODO(build-order:6+): split current home screen into reusable screens
    //                       once menus / settings need to coexist.
}

void ui_show_emote(emote_id_t emote)
{
    (void)emote;
    // TODO(build-order:10): ~2 s speech bubble; synced via radio.
}
