// radio implementation — skeleton.
// Make-or-break milestone is build-order step 8 (ESP-NOW discovery).

#include "radio.h"
#include "esp_log.h"

static const char *TAG = "radio";

void radio_init(void)
{
    // TODO(build-order:8): esp_wifi init (no STA assoc) + esp_now init,
    //                      fixed channel, register recv cb.
    ESP_LOGI(TAG, "init (skeleton) — proto v%u", (unsigned)RADIO_PROTOCOL_VER);
}

void radio_start_beacon(void) { /* TODO(build-order:8) */ }
void radio_stop_beacon(void)  { /* TODO(build-order:8) */ }

bool radio_begin_handshake(uint64_t peer_pet_id)
{
    (void)peer_pet_id;
    return false;  // TODO(build-order:8): opt-in + host election
}

void radio_send_update(const PetUpdate *u)
{
    (void)u;
    // TODO(build-order:9): on-change send + 2 s snapshot heartbeat.
}
