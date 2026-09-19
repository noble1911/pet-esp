#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "audio.h"
// Pure, bounded PCM renderer shared by the ESP and host audio tests.
#define SOUND_RATE 16000
#define SOUND_MAX_NOTES 24
typedef enum { SOUND_BELL, SOUND_PLUCK, SOUND_FLUTE, SOUND_POP, SOUND_CRUNCH, SOUND_BOUNCE } sound_timbre_t;
typedef struct { uint8_t midi; uint16_t ms; } sound_note_t;
typedef struct { sound_note_t notes[SOUND_MAX_NOTES]; unsigned count, index, sample; sound_timbre_t timbre; float phase; uint32_t noise; } sound_synth_t;
void sound_start_effect(sound_synth_t *s, sfx_id_t fx);
bool sound_start_tune(sound_synth_t *s, unsigned tune);
void sound_render(sound_synth_t *s, int16_t *out, size_t samples);
bool sound_active(const sound_synth_t *s);
