// audio — simple beep/chirp cues. Samples are loaded from SD
// (architecture §2). Synthesised-vs-sampled is a deferred decision
// (architecture §11) — keep this surface minimal.

#pragma once

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

#ifdef __cplusplus
}
#endif
