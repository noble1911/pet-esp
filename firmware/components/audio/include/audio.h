// One speaker owner for distinct synthesized effects, MIDI songs and speech.

#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SFX_HATCH,
    SFX_FEED,
    SFX_HAPPY,
    SFX_MEET,      // beacon / "!" prompt
    SFX_EMOTE,
    SFX_APPLE, SFX_TOAST, SFX_COOKIE, SFX_CUDDLE, SFX_STAR,
    SFX_BUBBLE, SFX_BOUNCE, SFX_HIDE, SFX_FOUND, SFX_SLEEP,
    SFX_BATH, SFX_STICKER, SFX_GIFT, SFX_SELECT, SFX_BUTTERFLY,
    SFX_CUPCAKE, SFX_PANCAKES, SFX_JELLY, SFX_CAKE,
    SFX_COUNT,
} sfx_id_t;

// Bring up the audio codec. Safe no-op until audio is wired in.
void audio_init(void);

// Non-blocking, latest cue wins; mic and speech take priority.
void audio_play(sfx_id_t sfx);
void audio_set_muted(bool muted);
unsigned audio_tune_count(void);
const char *audio_tune_name(unsigned tune);
bool audio_play_tune(unsigned tune); // Offline MIDI library; indices below audio_tune_count().
void audio_stop_tune(void);
bool audio_tune_playing(void);
// Session volume, 0 (silent) to 100. Mute preserves the selected level.
void audio_set_volume(int percent);
int audio_get_volume(void);

#ifdef __cplusplus
}
#endif

#include <stdint.h>
#include <stddef.h>
typedef void (*audio_mic_cb_t)(const uint8_t *, size_t);
void audio_set_mic_callback(audio_mic_cb_t cb);
void audio_set_capture(bool enabled);
void audio_play_pcm(const uint8_t *data, size_t len);
void audio_voice_begin(void);
void audio_voice_end(void);
void audio_voice_stop(void);
bool audio_voice_playing(void);

bool audio_is_ready(void);
unsigned audio_mic_frames(void);
int audio_mic_level(void);
