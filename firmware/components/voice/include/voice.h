#pragma once
#include <stdbool.h>
#include <stddef.h>
#include "pet_state.h"
typedef enum { VOICE_OFFLINE, VOICE_READY, VOICE_LISTENING, VOICE_THINKING, VOICE_SPEAKING, VOICE_ERROR } voice_state_t;
void voice_init(void);
// Called by the LVGL owner; snapshots are copied before the background work.
void voice_start_talk(const Pet *pet, const char *activity);
void voice_end_talk(const Pet *pet, const char *activity);
void voice_cancel(void);
void voice_check(void);
voice_state_t voice_get_state(void);
void voice_status(char *out, size_t n);
void voice_caption(char *out, size_t n);
bool voice_boot_pressed(void);
