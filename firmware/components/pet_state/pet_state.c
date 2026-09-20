// pet_state implementation — build-order step 6: all four needs (hunger,
// happiness, energy, hygiene) decay independently, and care actions can
// restore each one. Evolution / breeding stay stubbed.

#include "pet_state.h"
#include "pet_traits_data.h"
#include "pet_characters_data.h"

#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "esp_random.h"
#include "nvs.h"

static const char *TAG = "pet_state";

#define NVS_NAMESPACE "pet"
#define NVS_KEY       "blob"

const pet_trait_t *pet_trait(unsigned gene)
{
    return gene<8 ? &TRAITS[gene] : NULL;
}
unsigned pet_trait_choice(const Pet *pet, unsigned gene)
{
    const pet_trait_t *t=pet_trait(gene);
    return pet && t ? pet->genes[gene]%t->count : 0;
}

const pet_character_t *pet_character(unsigned index)
{
    return index<PET_CHARACTER_COUNT?&CHARACTERS[index]:NULL;
}
unsigned pet_character_id(const Pet *pet)
{
    unsigned stored=pet?pet->genes[GENE_PATTERN]:0;
    return stored>=PET_CHARACTER_MARKER && stored<PET_CHARACTER_MARKER+PET_CHARACTER_COUNT ? stored-PET_CHARACTER_MARKER : 0;
}

// Gentle active-time needs. Offline time is paused; needs never fall below 20.
#define HUNGER_DECAY_PERIOD_SEC    180
#define HAPPINESS_DECAY_PERIOD_SEC 240
#define ENERGY_DECAY_PERIOD_SEC    300
#define HYGIENE_DECAY_PERIOD_SEC   360

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
static void pet_roll_genes(Pet *pet)
{
    for (int i = 0; i < 8; i++) {
        pet->genes[i] = (uint8_t)(esp_random() % PET_GENE_MAX[i]);
    }
}

static void pet_hatch(Pet *pet)
{
    memset(pet, 0, sizeof(*pet));
    strcpy(pet->name, "Sprout");
    pet->version   = PET_SCHEMA_VERSION;
    // Skip the egg phase for now — the renderer's asset library only has
    // baby+ art, and the egg→baby hatching transition is step 7's job.
    pet->stage     = PET_STAGE_BABY;
    pet->hunger    = 100;
    pet->happiness = 100;
    pet->energy    = 100;
    pet->hygiene   = 100;
    pet->last_tick = (uint32_t)time(NULL);
    pet->birth_timestamp = pet->last_tick;
    pet->pet_id    = ((uint64_t)esp_random() << 32) | esp_random();
    pet_roll_genes(pet);
}

void pet_state_init(void)
{
    if (pet_state_load(&s_pet)) {
        s_have_pet = true;
        s_pet.name[15] = 0;
        if (!s_pet.name[0]) { strcpy(s_pet.name,"Sprout"); pet_state_save(&s_pet); }
        // Accept old saves without retaining exhausted development-mode needs.
        uint8_t *needs[] = {&s_pet.hunger, &s_pet.happiness, &s_pet.energy, &s_pet.hygiene};
        for (int i=0; i<4; i++) {
            if (*needs[i] < 20) *needs[i] = 20;
            if (*needs[i] > 100) *needs[i] = 100;
        }
        pet_state_check_evolution();
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
            pet_roll_genes(&s_pet);
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
        pet_hatch(&s_pet);
        s_have_pet = true;
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
    if (err != ESP_OK) ESP_LOGW(TAG, "could not persist pet state: %d", (int)err);
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

bool pet_state_reset(void)
{
    // Replace the blob only after the new pet is ready. Never erase the old
    // save first; a failed write must leave the current pet available.
    Pet fresh;
    pet_hatch(&fresh);
    if (!pet_state_save(&fresh)) return false;
    s_pet = fresh;
    s_have_pet = true;
    memset(s_decay_acc, 0, sizeof(s_decay_acc));
    ESP_LOGI(TAG, "reset: rolled fresh pet");
    return true;
}

// Stable preset order is shared with esp_gateway/pet_voices.py. Keep old IDs.
static const pet_voice_t s_voices[PET_VOICE_COUNT]={
    {"tiny_sprout","Tiny Sprout","Our familiar bright little squeak"},
    {"sunny","Sunny","Bouncy, bright and playful"},
    {"soft","Soft cloud","Gentle and softly squeaky"},
    {"warm","Warm story","Warm and friendly, lovely for Osono"},
    {"low","Calm & low","Low and calm, lovely for Larry"},
};
const pet_voice_t *pet_voice(unsigned index) { return index<PET_VOICE_COUNT?&s_voices[index]:NULL; }
unsigned pet_voice_id(const Pet *pet)
{
    unsigned v=pet?pet->inventory[9]:0;
    return v>=160 && v<160+PET_VOICE_COUNT?v-160:0;
}
bool pet_state_set_voice(unsigned index)
{
    if(!s_have_pet || index>=PET_VOICE_COUNT)return false;
    Pet next=s_pet;next.inventory[9]=(uint8_t)(160+index);
    if(!pet_state_save(&next))return false;
    s_pet=next;return true;
}

bool pet_state_set_character(unsigned index)
{
    if(!s_have_pet || index>=PET_CHARACTER_COUNT)return false;
    Pet next=s_pet;
    next.genes[GENE_PATTERN]=(uint8_t)(PET_CHARACTER_MARKER+index);
    if(!pet_state_save(&next))return false;
    s_pet=next;
    return true;
}

// Bank elapsed seconds into a per-need accumulator and apply decay when it
// crosses the period (keeping the remainder for next time). Returns true
// if the value changed.
static bool decay_one(uint8_t *value, uint32_t *acc,
                      uint32_t elapsed, uint32_t period)
{
    if (period == 0) return false;
    *acc += elapsed > 3600 ? 3600 : elapsed;
    uint32_t d = *acc / period;
    if (d == 0) return false;
    *acc -= d * period;
    *value = (*value <= 20 || d >= (uint32_t)(*value - 20)) ? 20 : (uint8_t)(*value - d);
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
    if (s_pet.evolution_progress < UINT32_MAX) s_pet.evolution_progress++;
    pet_state_check_evolution();
    pet_state_save(&s_pet);
}

static uint8_t stage_for_stars(uint32_t n)
{
    return n>=100 ? PET_STAGE_ELDER : n>=60 ? PET_STAGE_ADULT :
           n>=30 ? PET_STAGE_TEEN : n>=10 ? PET_STAGE_CHILD : PET_STAGE_BABY;
}
static const pet_special_food_t SPECIAL_FOODS[PET_SPECIAL_FOOD_COUNT] = {
    {"Star cupcake", "A tiny star on my cupcake!", 10},
    {"Berry pancakes", "Fluffy pancakes and juicy berries!", 25},
    {"Rainbow jelly", "Wibble wobble! Rainbow jelly!", 50},
    {"Party cake", "A slice of cake to celebrate!", 100},
};
const pet_special_food_t *pet_special_food(unsigned index)
{
    return index<PET_SPECIAL_FOOD_COUNT ? &SPECIAL_FOODS[index] : NULL;
}
bool pet_food_unlocked(const Pet *pet, unsigned food)
{
    return pet && food<PET_FOOD_COUNT && (food<3 || pet->evolution_progress>=SPECIAL_FOODS[food-3].stars);
}
int pet_food_milestone(uint32_t stars)
{
    for(unsigned i=0;i<PET_SPECIAL_FOOD_COUNT;i++)if(stars==SPECIAL_FOODS[i].stars)return (int)i;
    return -1;
}
bool pet_state_eat(unsigned food)
{
    if(!s_have_pet || !pet_food_unlocked(&s_pet,food))return false;
    Pet next=s_pet;
    unsigned fullness=next.hunger+(food<3?30:40);
    next.hunger=fullness>100?100:fullness;
    if(food>=3) {unsigned joy=next.happiness+10;next.happiness=joy>100?100:joy;}
    if(next.evolution_progress<UINT32_MAX)next.evolution_progress++;
    next.stage=stage_for_stars(next.evolution_progress);
    if(!pet_state_save(&next))return false;
    s_pet=next;
    return true;
}

void pet_state_feed(void)  { care_restore(&s_pet.hunger,    CARE_RESTORE_AMOUNT); }
void pet_state_play(void)  { care_restore(&s_pet.happiness, CARE_RESTORE_AMOUNT); }
void pet_state_rest(void)  { care_restore(&s_pet.energy,    CARE_RESTORE_AMOUNT); }
void pet_state_clean(void) { care_restore(&s_pet.hygiene,   CARE_RESTORE_AMOUNT); }

void pet_state_check_evolution(void)
{
    if (!s_have_pet) return;
    s_pet.stage=stage_for_stars(s_pet.evolution_progress);
}

void pet_breed(const Pet *a, const Pet *b,
               uint32_t session_timestamp, Pet *child_out)
{
    (void)a; (void)b; (void)session_timestamp; (void)child_out;
    // TODO(build-order:13): deterministic mixer, byte-identical on both
    //                       devices (docs/gene_spec.md).
}

// Keep names short and readable on the toy and in the voice context.
bool pet_state_set_name(const char *name)
{
    size_t n = name ? strlen(name) : 0;
    if (!n || n > 15 || !((name[0]>='A' && name[0]<='Z') || (name[0]>='a' && name[0]<='z'))) return false;
    for(size_t i=0;i<n;i++) if(!((name[i]>='A' && name[i]<='Z') || (name[i]>='a' && name[i]<='z') || name[i]==' ' || name[i]=='-' || name[i]=='\'')) return false;
    Pet updated=s_pet;
    strcpy(updated.name,name);
    if(!pet_state_save(&updated)) return false;
    s_pet=updated;
    return true;
}

// Rewards are derived from lifetime care stars, so every old save receives full
// credit. No new fields/currency and no NVS schema change are required.
unsigned pet_sticker_count(const Pet *pet)
{
    unsigned n=pet ? pet->evolution_progress/5 : 0;
    return (n>PET_CARE_STICKER_COUNT ? PET_CARE_STICKER_COUNT : n)+(pet && pet->friends_met>0 ? 1:0);
}
bool pet_sticker_unlocked(const Pet *pet,unsigned sticker)
{
    return pet && sticker<PET_STICKER_COUNT && (sticker==PET_CARE_STICKER_COUNT?pet->friends_met>0:pet->evolution_progress>=(sticker+1)*5);
}
bool pet_state_playdate(uint64_t receipt,uint64_t pet_id,uint64_t friend_id,unsigned friends)
{
    if(!s_have_pet || !receipt || pet_id!=s_pet.pet_id || !friend_id || friend_id==pet_id || !friends || friends>65535)return false;
    uint64_t last=0;
    if(s_pet.inventory[8]==215)for(unsigned i=0;i<8;i++)last|=(uint64_t)s_pet.inventory[i]<<(i*8);
    if(receipt<=last)return true; // Persisted already; safe to acknowledge a replay.
    Pet next=s_pet;
    for(unsigned i=0;i<8;i++)next.inventory[i]=(uint8_t)(receipt>>(i*8));
    next.inventory[8]=215;
    if(next.evolution_progress<UINT32_MAX)next.evolution_progress++;
    next.stage=stage_for_stars(next.evolution_progress);
    unsigned happy=next.happiness+30;next.happiness=happy>100?100:happy;
    if(friends>next.friends_met)next.friends_met=(uint16_t)friends;
    bool known=false;for(unsigned i=0;i<8;i++)if(next.recent_friends[i]==friend_id)known=true;
    if(!known) {memmove(next.recent_friends+1,next.recent_friends,7*sizeof(uint64_t));next.recent_friends[0]=friend_id;}
    if(!pet_state_save(&next))return false;
    s_pet=next;return true;
}
unsigned pet_decoration_threshold(unsigned decoration)
{
    static const unsigned thresholds[PET_DECORATION_COUNT]={10,20,30,45,60,90};
    return decoration<PET_DECORATION_COUNT ? thresholds[decoration] : UINT32_MAX;
}
bool pet_decoration_unlocked(const Pet *pet, unsigned decoration)
{
    return pet && decoration<PET_DECORATION_COUNT && pet->evolution_progress>=pet_decoration_threshold(decoration);
}
unsigned pet_room_gifts(const Pet *pet)
{
    if(!pet)return 0;
    unsigned value=pet->inventory[14], mask=0;
    if(value>=128 && value<=191) mask=value & 63;
    else if(pet->inventory[15]>=100 && pet->inventory[15]<106)
        mask=1u<<(pet->inventory[15]-100); // schema-1 single-gift save
    for(unsigned i=0;i<PET_DECORATION_COUNT;i++)
        if(!pet_decoration_unlocked(pet,i))mask&=~(1u<<i);
    return mask;
}
int pet_equipped_decoration(const Pet *pet)
{
    unsigned mask=pet_room_gifts(pet);
    for(unsigned i=0;i<PET_DECORATION_COUNT;i++)if(mask&(1u<<i))return (int)i;
    return -1;
}
static bool save_room_gifts(unsigned mask)
{
    Pet next=s_pet;
    next.inventory[14]=(uint8_t)(128|mask);
    next.inventory[15]=0;
    // Older firmware still sees one of the placed gifts after rollback.
    for(unsigned i=0;i<PET_DECORATION_COUNT;i++)if(mask&(1u<<i)) {
        next.inventory[15]=(uint8_t)(100+i);break;
    }
    if(!pet_state_save(&next))return false;
    s_pet=next;return true;
}
bool pet_equip_decoration(int decoration)
{
    if(!s_have_pet || decoration < -1 || (decoration>=0 && !pet_decoration_unlocked(&s_pet,(unsigned)decoration)))return false;
    return save_room_gifts(decoration<0?0:1u<<decoration);
}
bool pet_toggle_decoration(unsigned decoration)
{
    if(!s_have_pet || !pet_decoration_unlocked(&s_pet,decoration))return false;
    return save_room_gifts(pet_room_gifts(&s_pet)^(1u<<decoration));
}
int pet_wall_sticker(const Pet *pet)
{
    if(!pet)return -1;
    int sticker=(int)pet->inventory[13]-200;
    return sticker>=0 && pet_sticker_unlocked(pet,(unsigned)sticker)?sticker:-1;
}
bool pet_set_wall_sticker(int sticker)
{
    if(!s_have_pet || sticker < -1 || (sticker>=0 && !pet_sticker_unlocked(&s_pet,(unsigned)sticker)))return false;
    uint8_t value=sticker<0?0:(uint8_t)(200+sticker);
    if(s_pet.inventory[13]==value)return true;
    Pet next=s_pet;next.inventory[13]=value;
    if(!pet_state_save(&next))return false;
    s_pet=next;return true;
}
