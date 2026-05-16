// pet_state implementation — build-order step 4: hunger decay + NVS
// persistence. Evolution / happiness / energy / hygiene wait their turn.

#include "pet_state.h"

#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "pet_state";

#define NVS_NAMESPACE "pet"
#define NVS_KEY       "blob"

// Dev tuning — visible decay in tens of seconds. Architecture §11 flags
// "exact decay rates per need" as an open question; this is the placeholder.
#define HUNGER_DECAY_PERIOD_SEC 10

// docs/gene_spec.md: GENE_MAX[8] = {8,16,8,16,8,8,8,8}
const uint8_t PET_GENE_MAX[8] = { 8, 16, 8, 16, 8, 8, 8, 8 };

static Pet  s_pet;
static bool s_have_pet = false;

static void pet_hatch(void)
{
    memset(&s_pet, 0, sizeof(s_pet));
    s_pet.version   = PET_SCHEMA_VERSION;
    s_pet.hunger    = 100;
    s_pet.happiness = 100;
    s_pet.energy    = 100;
    s_pet.hygiene   = 100;
    s_pet.last_tick = (uint32_t)time(NULL);
    s_have_pet = true;
}

void pet_state_init(void)
{
    if (pet_state_load(&s_pet)) {
        s_have_pet = true;
        // System clock may be unset across reboot (no NTP / RTC sync yet);
        // resetting last_tick avoids a billion-second elapsed jump that
        // would instantly zero every need.
        s_pet.last_tick = (uint32_t)time(NULL);
        ESP_LOGI(TAG, "loaded from NVS: hunger=%d", s_pet.hunger);
    } else {
        pet_hatch();
        pet_state_save(&s_pet);
        ESP_LOGI(TAG, "hatched fresh egg: hunger=%d", s_pet.hunger);
    }
}

bool pet_state_load(Pet *out)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) return false;
    size_t len = sizeof(*out);
    esp_err_t err = nvs_get_blob(h, NVS_KEY, out, &len);
    nvs_close(h);
    if (err != ESP_OK) return false;
    if (len != sizeof(*out)) return false;
    if (out->version != PET_SCHEMA_VERSION) return false;
    return true;
}

bool pet_state_save(const Pet *pet)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) return false;
    esp_err_t err = nvs_set_blob(h, NVS_KEY, pet, sizeof(*pet));
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err == ESP_OK;
}

const Pet *pet_state_get(void)
{
    return s_have_pet ? &s_pet : NULL;
}

void pet_state_tick(uint32_t now_unix)
{
    if (!s_have_pet) return;
    if (now_unix <= s_pet.last_tick) return;

    uint32_t elapsed = now_unix - s_pet.last_tick;
    uint32_t decay = elapsed / HUNGER_DECAY_PERIOD_SEC;
    if (decay == 0) return;

    s_pet.hunger = (decay >= s_pet.hunger) ? 0 : (uint8_t)(s_pet.hunger - decay);
    // Advance last_tick by the consumed periods only — sub-period remainder
    // accrues toward the next tick so decay rate is exact over the long run.
    s_pet.last_tick += decay * HUNGER_DECAY_PERIOD_SEC;
    // TODO(prod tuning): batch saves to reduce flash wear (~8k writes/day at
    // 10 s tick is fine for dev, not for years of always-on runtime).
    pet_state_save(&s_pet);
}

void pet_state_check_evolution(void)
{
    // TODO(build-order:7): time + care thresholds with branching forms.
}

void pet_breed(const Pet *a, const Pet *b,
               uint32_t session_timestamp, Pet *child_out)
{
    (void)a; (void)b; (void)session_timestamp; (void)child_out;
    // TODO(build-order:13): deterministic mixer, byte-identical on both
    //                       devices (docs/gene_spec.md).
}
