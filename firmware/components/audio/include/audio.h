// audio — simple beep/chirp cues. Samples are loaded from SD
// (architecture §2). Synthesised-vs-sampled is a deferred decision
// (architecture §11) — keep this surface minimal.

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
} sfx_id_t;

// Bring up the audio codec. Safe no-op until audio is wired in.
void audio_init(void);

// TODO(build-order:12): non-blocking one-shot cue.
void audio_play(sfx_id_t sfx);
void audio_set_muted(bool muted);
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
