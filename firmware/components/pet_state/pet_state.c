// pet_state implementation — build-order step 6: all four needs (hunger,
// happiness, energy, hygiene) decay independently, and care actions can
// restore each one. Evolution / breeding stay stubbed.

#include "pet_state.h"

#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "esp_random.h"
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

// Roll all 8 genes uniformly within their valid ranges (gene_spec.md
// GENE_MAX). Out-of-range values clamp at render time, so this is safe
// even if the per-byte modulus is later widened.
static void pet_roll_genes(void)
{
    for (int i = 0; i < 8; i++) {
        s_pet.genes[i] = (uint8_t)(esp_random() % PET_GENE_MAX[i]);
    }
}

static void pet_hatch(void)
{
    memset(&s_pet, 0, sizeof(s_pet));
    s_pet.version   = PET_SCHEMA_VERSION;
    // Skip the egg phase for now — the renderer's asset library only has
    // baby+ art, and the egg→baby hatching transition is step 7's job.
    s_pet.stage     = PET_STAGE_BABY;
    s_pet.hunger    = 100;
    s_pet.happiness = 100;
    s_pet.energy    = 100;
    s_pet.hygiene   = 100;
    s_pet.last_tick = (uint32_t)time(NULL);
    s_pet.birth_timestamp = s_pet.last_tick;
    s_pet.pet_id    = ((uint64_t)esp_random() << 32) | esp_random();
    pet_roll_genes();
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
        // Migration: NVS blobs from before the baby-on-hatch fix carry
        // stage=egg, but the renderer has no egg art. Bump to baby in place.
        if (s_pet.stage == PET_STAGE_EGG) {
            s_pet.stage = PET_STAGE_BABY;
            pet_state_save(&s_pet);
        }
        // Migration: NVS blobs from before random-gene seeding stored all
        // zeros. Roll a fresh vector so the existing pet isn't trapped in
        // the (single-shape, palette-entry-0) corner of the gene space.
        bool zero_genes = true;
        for (int i = 0; i < 8; i++) {
            if (s_pet.genes[i] != 0) { zero_genes = false; break; }
        }
        if (zero_genes) {
            pet_roll_genes();
            if (s_pet.pet_id == 0) {
                s_pet.pet_id = ((uint64_t)esp_random() << 32) | esp_random();
            }
            pet_state_save(&s_pet);
            ESP_LOGI(TAG, "migrated: rolled fresh gene vector");
        }
        ESP_LOGI(TAG, "loaded from NVS: stage=%d H=%d Hp=%d E=%d Hy=%d",
                 s_pet.stage, s_pet.hunger, s_pet.happiness,
                 s_pet.energy, s_pet.hygiene);
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

// Mood-emote thresholds (architecture §7.5). CRITICAL is the urgency
// floor — needs below this trigger an autonomous bubble asking for the
// matching care action. HAPPY is the contentment ceiling — all needs
// above this triggers an "I'm great" bubble. The band in between is
// "personality territory" where the pet's personality gene picks an
// ambient emote.
#define MOOD_CRITICAL 30
#define MOOD_HAPPY    75

// emote_id_t values (mirror firmware/components/ui/include/ui.h enum).
// Hard-coded here as raw ints to avoid pulling ui.h across the component
// boundary — ui_state is the consumer, not the producer.
#define EMOTE_NONE       0
#define EMOTE_AFFECTION  2
#define EMOTE_EXCITED    3
#define EMOTE_HUNGRY    10
#define EMOTE_SLEEPY    11
#define EMOTE_THIRSTY   12
#define EMOTE_CONFUSED  22
#define EMOTE_SAD       25

uint8_t pet_state_mood_emote(void)
{
    if (!s_have_pet) {
        return EMOTE_NONE;
    }
    const Pet *p = &s_pet;

    // 1. Critical-need urgency. The single lowest stat under the floor
    //    decides which urgent bubble fires — that's the most pressing
    //    request from the pet's perspective.
    uint8_t lowest_v = MOOD_CRITICAL;
    uint8_t lowest_emote = EMOTE_NONE;
    if (p->hunger    < lowest_v) { lowest_v = p->hunger;    lowest_emote = EMOTE_HUNGRY;  }
    if (p->energy    < lowest_v) { lowest_v = p->energy;    lowest_emote = EMOTE_SLEEPY;  }
    if (p->hygiene   < lowest_v) { lowest_v = p->hygiene;   lowest_emote = EMOTE_THIRSTY; }
    if (p->happiness < lowest_v) { lowest_v = p->happiness; lowest_emote = EMOTE_SAD;     }
    if (lowest_emote != EMOTE_NONE) {
        return lowest_emote;
    }

    // 2. Contentment ceiling — if every need is high, the pet beams.
    if (p->hunger > MOOD_HAPPY && p->happiness > MOOD_HAPPY
        && p->energy > MOOD_HAPPY && p->hygiene > MOOD_HAPPY) {
        // Personality-tinted "I'm content" bubble. Sweet personality
        // shows hearts; everyone else shows the universal sparkle.
        uint8_t personality = p->genes[7];   // GENE_PERSONALITY
        return (personality == 3 /*sweet*/) ? EMOTE_AFFECTION : EMOTE_EXCITED;
    }

    // 3. In-between band — personality-driven ambient emote (docs/
    //    gene_spec.md §personality archetypes). Most personalities
    //    don't pop ambient emotes — only the chatty/curious ones do.
    //    Keeps the device from feeling noisy by default.
    uint8_t personality = p->genes[7];
    switch (personality) {
    case 1: /* shy */     return EMOTE_CONFUSED;    // ❓ — uncertain
    case 2: /* energetic */ return EMOTE_EXCITED;   // ✨
    case 3: /* sweet */   return EMOTE_AFFECTION;   // 💕
    case 7: /* curious */ return EMOTE_CONFUSED;    // ❓
    default:              return EMOTE_NONE;        // quiet personalities
    }
}

void pet_state_reset(void)
{
    // Targeted erase of the pet blob key — leaves other future namespaces
    // intact, unlike nvs_flash_erase() which nukes everything in NVS.
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_key(h, NVS_KEY);
        nvs_commit(h);
        nvs_close(h);
    }
    // Drop banked decay so a fresh pet doesn't bleed needs from the
    // previous one's accumulated time.
    memset(s_decay_acc, 0, sizeof(s_decay_acc));
    pet_hatch();
    pet_state_save(&s_pet);
    ESP_LOGI(TAG, "reset: rolled fresh pet");
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
