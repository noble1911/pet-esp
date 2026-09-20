// Little Meadow boot: NVS, display/touch, PMIC, audio, then the UI.
#include <time.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "pet_state.h"
#include "renderer.h"
#include "radio.h"
#include "ui.h"
#include "audio.h"
#include "power.h"
#include "voice.h"
#include "multiplayer.h"
#include "motion.h"

static const char *TAG = "pet";

void app_main(void)
{
    ESP_LOGI(TAG, "pet-esp boot");

    // NVS must come up first: the Pet blob lives here (architecture §4.1).
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }

    pet_state_init();
    renderer_init();
    // power_init must come AFTER renderer_init: the BSP brings up the
    // shared I²C bus inside renderer_init. Without that,
    // bsp_i2c_get_handle() returns NULL.
    power_init();
    motion_init();
    audio_init();
    voice_init();
    multiplayer_init();
    ui_init();
    radio_init();

    // First paint shouldn't show the "--" placeholder. last_tick is reset
    // on load (pet_state_init) so this initial tick is a clean no-op
    // beyond the label refresh.
    pet_state_tick((uint32_t)time(NULL));
    ui_refresh_stats();

    // The LVGL timer owns needs and care mutations, avoiding races with NVS saves.
    ESP_LOGI(TAG, "Little Meadow ready; touch activities and active-time care enabled");
}
