// ui implementation — skeleton.
// "Now it's a pet" is build-order step 6.

#include "ui.h"
#include "esp_log.h"

static const char *TAG = "ui";

void ui_init(void)
{
    // TODO(build-order:6): LVGL screen graph + touch handlers.
    ESP_LOGI(TAG, "init (skeleton)");
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
