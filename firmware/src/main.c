// pet-esp firmware entry point.
//
// Skeleton only. The build order (architecture §10) is followed step by
// step; each step ends with a working, demoable artifact. This file wires
// components together as those steps land — it does not implement features
// ahead of the list.
//
//   1. Hardware smoke test (factory demo — not this firmware)
//   2. LVGL hello world          -> renderer
//   3. Pet exists (placeholder)  -> renderer
//   4. Pet ticks + NVS           -> pet_state
//   5. Real renderer             -> renderer
//   6. UI loop                   -> ui
//   7. Evolution                 -> pet_state
//   8. ESP-NOW discovery         -> radio
//   9. Shared canvas             -> radio + renderer
//  10. Emotes                    -> radio + ui
//  11. Items in shared space     -> radio + ui
//  12. Mini-game + dance         -> radio + ui + audio
//  13. Breeding                  -> pet_state + radio

#include "esp_log.h"
#include "nvs_flash.h"

#include "pet_state.h"
#include "renderer.h"
#include "radio.h"
#include "ui.h"
#include "audio.h"

static const char *TAG = "pet";

void app_main(void)
{
    ESP_LOGI(TAG, "pet-esp boot (skeleton)");

    // NVS must come up first: the Pet blob lives here (architecture §4.1).
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }

    // Components self-init into a no-op until their build-order step lands.
    pet_state_init();
    renderer_init();
    audio_init();
    ui_init();
    radio_init();

    // TODO(build-order:4): periodic tick (~60 s) -> pet_state_tick().
    // TODO(build-order:6): hand the main loop to the UI / LVGL task.
    ESP_LOGI(TAG, "init complete; no feature loop yet");
}
