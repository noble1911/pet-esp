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
    uint32_t evolution_progress;

    // Needs (0-100, decay over time)
    uint8_t  hunger;
    uint8_t  happiness;
    uint8_t  energy;
    uint8_t  hygiene;

    // Inventory — item IDs, 0 = empty slot
    uint8_t  inventory[16];

    // Social
    uint16_t friends_met;
    uint64_t recent_friends[8];

    // Forward-compat
    uint8_t  version;
} Pet;

// Per-byte modulus for the breeding mixer (docs/gene_spec.md).
extern const uint8_t PET_GENE_MAX[8];

// Lifecycle ------------------------------------------------------------

// Load the Pet from NVS, or leave an unhatched egg if none exists.
// Safe no-op until build-order step 4 (architecture §10).
void pet_state_init(void);

bool pet_state_load(Pet *out);
bool pet_state_save(const Pet *pet);

const Pet *pet_state_get(void);  // current in-RAM pet (NULL if none)

// Real-time model (architecture §4.2) -----------------------------------

// Apply elapsed-time decay + evolution check using the RTC. Called on
// wake/boot and on a slow (~60 s) timer. TODO(build-order:4).
void pet_state_tick(uint32_t now_unix);

// Evolution (architecture §4.3) -----------------------------------------

// TODO(build-order:7): time/care-based stage transition with branching.
void pet_state_check_evolution(void);

// Breeding (architecture §6.2, docs/gene_spec.md) -----------------------

// Deterministic mixer — MUST be byte-identical across devices.
// TODO(build-order:13).
void pet_breed(const Pet *a, const Pet *b,
               uint32_t session_timestamp, Pet *child_out);

#ifdef __cplusplus
}
#endif
