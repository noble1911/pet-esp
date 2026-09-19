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

bool voice_auto_enabled(void);
void voice_set_auto_enabled(bool enabled);
bool voice_remark(const Pet *pet, const char *activity);

// Short-lived device context, copied with each pet snapshot (never child speech).
typedef enum { PET_EVENT_NONE, PET_EVENT_CUDDLE, PET_EVENT_APPLE, PET_EVENT_TOAST,
    PET_EVENT_COOKIE, PET_EVENT_STAR, PET_EVENT_PLAY, PET_EVENT_BUBBLE,
    PET_EVENT_BATH, PET_EVENT_NAP, PET_EVENT_HIDE, PET_EVENT_BALL, PET_EVENT_BUTTERFLY } pet_event_t;
void voice_note_event(pet_event_t event);
bool voice_react(const Pet *pet, const char *activity);
