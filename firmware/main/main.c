// pet-esp firmware entry point.
//
// Build order (architecture §10) — each step ends with a working artifact:
//   1. Hardware smoke test (factory demo — not this firmware)
//   2. LVGL hello world          -> renderer            DONE
//   3. Pet exists (placeholder)  -> renderer + ui       DONE
//   4. Pet ticks + NVS           -> pet_state           DONE
//   5. Real renderer             -> renderer
//   6. UI loop                   -> ui
//   7. Evolution                 -> pet_state
//   8. ESP-NOW discovery         -> radio
//   9. Shared canvas             -> radio + renderer
//  10. Emotes                    -> radio + ui
//  11. Items in shared space     -> radio + ui
//  12. Mini-game + dance         -> radio + ui + audio
//  13. Breeding                  -> pet_state + radio

#include <time.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "pet_state.h"
#include "renderer.h"
#include "radio.h"
#include "ui.h"
#include "audio.h"
#include "power.h"

static const char *TAG = "pet";

// Architecture §4.2 calls for a slow (~60 s) tick. While step 4 is dev-
// tuned to faster decay for visibility, keeping the polling interval
// short means the overlay stays responsive to care actions when those
// land at step 6.
#define TICK_INTERVAL_US (10ULL * 1000 * 1000)

static void tick_cb(void *arg)
{
    (void)arg;
    pet_state_tick((uint32_t)time(NULL));
    ui_refresh_stats();
}

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
    // shared I²C bus inside bsp_display_start() (called from renderer
    // init). Without that, bsp_i2c_get_handle() returns NULL.
    power_init();
    audio_init();
    ui_init();
    radio_init();

    // First paint shouldn't show the "--" placeholder. last_tick is reset
    // on load (pet_state_init) so this initial tick is a clean no-op
    // beyond the label refresh.
    pet_state_tick((uint32_t)time(NULL));
    ui_refresh_stats();

    const esp_timer_create_args_t args = {
        .callback = tick_cb,
        .name     = "pet_tick",
    };
    esp_timer_handle_t timer;
    ESP_ERROR_CHECK(esp_timer_create(&args, &timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(timer, TICK_INTERVAL_US));

    ESP_LOGI(TAG, "init complete; tick every %llu s",
             (unsigned long long)(TICK_INTERVAL_US / 1000000ULL));
}
