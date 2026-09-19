// pet_state — the Pet struct, real-time tick/decay, evolution, NVS
// persistence and the deterministic breeding mixer.
//
// References: architecture.md §4 (model), §6 (genetics),
// docs/gene_spec.md. The struct layout is wire/storage sensitive — never
// reorder fields without bumping PET_SCHEMA_VERSION.

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PET_SCHEMA_VERSION 1

// Life stages (Pet.stage). Order is significant.
typedef enum {
    PET_STAGE_EGG   = 0,
    PET_STAGE_BABY  = 1,
    PET_STAGE_CHILD = 2,
    PET_STAGE_TEEN  = 3,
    PET_STAGE_ADULT = 4,
    PET_STAGE_ELDER = 5,
} pet_stage_t;

// 8-byte gene vector (architecture §6.1, docs/gene_spec.md).
typedef enum {
    GENE_BODY_SHAPE  = 0,
    GENE_BODY_COLOR  = 1,
    GENE_EYE_SHAPE   = 2,
    GENE_EYE_COLOR   = 3,
    GENE_EAR_SHAPE   = 4,
    GENE_MOUTH_SHAPE = 5,
    GENE_PATTERN     = 6,
    GENE_PERSONALITY = 7,
} pet_gene_t;

// Single struct, persisted as one NVS blob, kept < 256 bytes for fast
// atomic writes (architecture §4.1).
typedef struct {
    // Identity
    uint64_t pet_id;
    char     name[16];
    uint32_t birth_timestamp;
    uint32_t last_tick;

    // Genetics (architecture §6)
    uint8_t  genes[8];
    uint8_t  generation;
    uint64_t parent_a;
    uint64_t parent_b;

    // Life stage
    uint8_t  stage;              // pet_stage_t
    uint32_t evolution_progress; // completed care stars (Little Meadow)

    // Needs (0-100, decay over time)
    uint8_t  hunger;
    uint8_t  happiness;
    uint8_t  energy;
    uint8_t  hygiene;

    // Inventory: slot 13 = wall sticker (200..217), slot 14 = room mask (128..191),
    // slot 15 = legacy room gift (100..105), retained for rollback. Zero = empty.
    uint8_t  inventory[16];

    // Social
    uint16_t friends_met;
    uint64_t recent_friends[8];

    // Forward-compat
    uint8_t  version;
} Pet;

// Per-byte modulus for the breeding mixer (docs/gene_spec.md).
extern const uint8_t PET_GENE_MAX[8];

// The old eight-byte genes and trait catalogue remain for save/wire rollback.
// Current appearance is one complete character; personality still uses byte 7.
#define PET_CHARACTER_COUNT 6
#define PET_CHARACTER_MARKER 240
typedef struct { const char *name, *description; } pet_character_t;
const pet_character_t *pet_character(unsigned index);
unsigned pet_character_id(const Pet *pet); // unmarked legacy saves use original Sprout
bool pet_state_set_character(unsigned index); // save first; preserves all progress

// Legacy appearance catalogue; personality is the only active trait card.
typedef struct {
    const char *label, *mode;
    unsigned count;
    const char *values[16], *descriptions[16];
} pet_trait_t;
const pet_trait_t *pet_trait(unsigned gene); // NULL for an invalid gene
unsigned pet_trait_choice(const Pet *pet, unsigned gene);

// Lifecycle ------------------------------------------------------------

// Load the Pet from NVS, or leave an unhatched egg if none exists.
// Safe no-op until build-order step 4 (architecture §10).
void pet_state_init(void);

bool pet_state_load(Pet *out);
bool pet_state_save(const Pet *pet);

const Pet *pet_state_get(void);  // current in-RAM pet (NULL if none)

// Persist a fresh baby with new identity/genes, default name, full needs and
// no progress/rewards. Other NVS namespaces remain intact. Returns false on
// save failure without replacing the in-memory pet. Caller refreshes/reboots.
bool pet_state_reset(void);

// Real-time model (architecture §4.2) -----------------------------------

// Apply elapsed-time decay + evolution check using the RTC. Called on
// wake/boot and on the periodic tick.
void pet_state_tick(uint32_t now_unix);

// Care actions (build-order step 6) — each restores its respective need
// by a fixed amount (clamped to 100) and saves to NVS atomically. The UI
// is responsible for refreshing the on-screen state after a care action.
// Ordinary snacks (0..2) are always available. Special recipes unlock forever
// from care-star milestones, including existing saves. No stock is consumed.
#define PET_SPECIAL_FOOD_COUNT 4
#define PET_FOOD_COUNT (3 + PET_SPECIAL_FOOD_COUNT)
typedef struct { const char *name, *reaction; unsigned stars; } pet_special_food_t;
const pet_special_food_t *pet_special_food(unsigned index);
bool pet_food_unlocked(const Pet *pet, unsigned food);
int pet_food_milestone(uint32_t stars); // special index, or -1
bool pet_state_eat(unsigned food); // transactional save; false for locked/invalid

void pet_state_feed(void);    // hunger
void pet_state_play(void);    // happiness
void pet_state_rest(void);    // energy
void pet_state_clean(void);   // hygiene

// Return the most appropriate emote_id_t for the pet's current mood.
// Pure function — no state changes, no side effects. Returns 0 (NONE)
// when no need is urgent enough to warrant an autonomous bubble.
//
// Decision order (architecture §7.5):
//   1. Any need < CRITICAL → matching urgent emote (HUNGRY/SLEEPY/etc)
//   2. All needs > HAPPY → EXCITED/AFFECTION (content/sparkle)
//   3. Personality-driven ambient (CONFUSED/AFFECTION/etc) — when needs
//      are in the in-between band, the personality gene picks
//   4. NONE if nothing rises above the floor
//
// The UI is responsible for cooldowns + actually rendering the bubble.
uint8_t pet_state_mood_emote(void);

// Evolution (architecture §4.3) -----------------------------------------

// Care-star growth at 10, 30, 60 and 100 completed activities.
void pet_state_check_evolution(void);

// Breeding (architecture §6.2, docs/gene_spec.md) -----------------------

// Deterministic mixer — MUST be byte-identical across devices.
// TODO(build-order:13).
void pet_breed(const Pet *a, const Pet *b,
               uint32_t session_timestamp, Pet *child_out);

bool pet_state_set_name(const char *name);

#define PET_STICKER_COUNT 18
#define PET_DECORATION_COUNT 6
unsigned pet_sticker_count(const Pet *pet);
unsigned pet_decoration_threshold(unsigned decoration); // 0-based; invalid => UINT32_MAX
bool pet_decoration_unlocked(const Pet *pet, unsigned decoration);
unsigned pet_room_gifts(const Pet *pet); // unlocked decorations as a six-bit mask
bool pet_toggle_decoration(unsigned decoration); // add/remove one; preserve other gifts
int pet_wall_sticker(const Pet *pet); // -1 = none; unlocked sticker only
bool pet_set_wall_sticker(int sticker); // -1 clears; save before changing RAM
int pet_equipped_decoration(const Pet *pet); // first placed gift; legacy compatibility
bool pet_equip_decoration(int decoration); // -1 clears; save before changing RAM

#ifdef __cplusplus
}
#endif
