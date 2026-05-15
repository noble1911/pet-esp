// pet_state implementation — skeleton.
//
// Stubs are intentional: features land per the build order
// (architecture §10), not ahead of it. Each stub names its step.

#include "pet_state.h"

#include <string.h>
#include "esp_log.h"

static const char *TAG = "pet_state";

// docs/gene_spec.md: GENE_MAX[8] = {8,16,8,16,8,8,8,8}
const uint8_t PET_GENE_MAX[8] = { 8, 16, 8, 16, 8, 8, 8, 8 };

static Pet  s_pet;
static bool s_have_pet = false;

void pet_state_init(void)
{
    // TODO(build-order:4): load from NVS; if absent, stay an unhatched egg.
    memset(&s_pet, 0, sizeof(s_pet));
    s_pet.version = PET_SCHEMA_VERSION;
    s_have_pet = false;
    ESP_LOGI(TAG, "init (skeleton) — sizeof(Pet)=%u", (unsigned)sizeof(Pet));
}

bool pet_state_load(Pet *out)
{
    (void)out;
    return false;  // TODO(build-order:4): nvs_get_blob
}

bool pet_state_save(const Pet *pet)
{
    (void)pet;
    return false;  // TODO(build-order:4): nvs_set_blob + commit (atomic)
}

const Pet *pet_state_get(void)
{
    return s_have_pet ? &s_pet : NULL;
}

void pet_state_tick(uint32_t now_unix)
{
    (void)now_unix;
    // TODO(build-order:4): elapsed = now - last_tick; apply_decay(elapsed);
    //                      check_evolution(); last_tick = now; save_if_dirty.
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
