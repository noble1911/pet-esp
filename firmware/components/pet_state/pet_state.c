// pet_state implementation — build-order step 6: all four needs (hunger,
// happiness, energy, hygiene) decay independently, and care actions can
// restore each one. Evolution / breeding stay stubbed.

#include "pet_state.h"

#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "pet_state";

#define NVS_NAMESPACE "pet"
#define NVS_KEY       "blob"

// Dev tuning — visible decay in tens of seconds. Hunger decays fastest so
// food is the most common care need; the others trail. Architecture §11
// flags exact per-need rates as an open question.
#define HUNGER_DECAY_PERIOD_SEC    10
#define HAPPINESS_DECAY_PERIOD_SEC 15
#define ENERGY_DECAY_PERIOD_SEC    20
#define HYGIENE_DECAY_PERIOD_SEC   25

// Care actions add this much to their target need, clamped to 100.
#define CARE_RESTORE_AMOUNT 30

// docs/gene_spec.md: GENE_MAX[8] = {8,16,8,16,8,8,8,8}
const uint8_t PET_GENE_MAX[8] = { 8, 16, 8, 16, 8, 8, 8, 8 };

static Pet  s_pet;
static bool s_have_pet = false;

// Per-need elapsed accumulators so slow-decaying needs aren't starved when
// the tick interval is shorter than their period. RAM-only — reset on
// reboot. Worst-case loss is (period - 1) seconds of progress per power-off,
// which is invisible at current dev tuning. Promoting these to persistent
// fields on `Pet` would require a schema bump.
enum { ACC_HUNGER, ACC_HAPPINESS, ACC_ENERGY, ACC_HYGIENE, ACC_COUNT };
static uint32_t s_decay_acc[ACC_COUNT];

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
        ESP_LOGI(TAG, "loaded from NVS: H=%d Hp=%d E=%d Hy=%d",
                 s_pet.hunger, s_pet.happiness, s_pet.energy, s_pet.hygiene);
    } else {
        pet_hatch();
        pet_state_save(&s_pet);
        ESP_LOGI(TAG, "hatched fresh egg");
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

// Bank elapsed seconds into a per-need accumulator and apply decay when it
// crosses the period (keeping the remainder for next time). Returns true
// if the value changed.
static bool decay_one(uint8_t *value, uint32_t *acc,
                      uint32_t elapsed, uint32_t period)
{
    if (period == 0) return false;
    *acc += elapsed;
    uint32_t d = *acc / period;
    if (d == 0) return false;
    *acc -= d * period;
    *value = (d >= *value) ? 0 : (uint8_t)(*value - d);
    return true;
}

void pet_state_tick(uint32_t now_unix)
{
    if (!s_have_pet) return;
    if (now_unix <= s_pet.last_tick) return;

    uint32_t elapsed = now_unix - s_pet.last_tick;
    bool dirty = false;
    dirty |= decay_one(&s_pet.hunger,    &s_decay_acc[ACC_HUNGER],
                       elapsed, HUNGER_DECAY_PERIOD_SEC);
    dirty |= decay_one(&s_pet.happiness, &s_decay_acc[ACC_HAPPINESS],
                       elapsed, HAPPINESS_DECAY_PERIOD_SEC);
    dirty |= decay_one(&s_pet.energy,    &s_decay_acc[ACC_ENERGY],
                       elapsed, ENERGY_DECAY_PERIOD_SEC);
    dirty |= decay_one(&s_pet.hygiene,   &s_decay_acc[ACC_HYGIENE],
                       elapsed, HYGIENE_DECAY_PERIOD_SEC);

    // Always advance last_tick — the accumulators captured the elapsed
    // seconds, so re-using last_tick=old would double-count next tick.
    s_pet.last_tick = now_unix;
    if (dirty) pet_state_save(&s_pet);
}

static void care_restore(uint8_t *value, uint8_t boost)
{
    if (!s_have_pet) return;
    uint16_t v = (uint16_t)*value + boost;
    *value = (v > 100) ? 100 : (uint8_t)v;
    pet_state_save(&s_pet);
}

void pet_state_feed(void)  { care_restore(&s_pet.hunger,    CARE_RESTORE_AMOUNT); }
void pet_state_play(void)  { care_restore(&s_pet.happiness, CARE_RESTORE_AMOUNT); }
void pet_state_rest(void)  { care_restore(&s_pet.energy,    CARE_RESTORE_AMOUNT); }
void pet_state_clean(void) { care_restore(&s_pet.hygiene,   CARE_RESTORE_AMOUNT); }

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
