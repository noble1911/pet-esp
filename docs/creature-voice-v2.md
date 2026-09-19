# Sprout's creature voice

The old personality was described as “warm, playful, gentle”, with up to 45 words per reply. That left room for adult assistant language, a list of activities and a closing question. Its sound came from Kokoro `af_sky` at speed 1.05 with a +3-semitone shift. The database's `bf_emma` field and handshake log were not the pet's effective voice: the pet gateway overrides them. Kokoro receives the spoken text and voice settings, not the language model's personality prompt.

## Current version

- **Personality:** usually 6–18 words, normally at most 24; simple creature reactions and imaginary leaf/tummy language, without baby talk or constant catchphrases. Occasional short interjections are allowed. No default assistant offers or appended questions. Suggest one game when asked what to play. Describe needs in words instead of numeric stats unless numbers are requested; reward goals remain factual. Direct questions about being real get a short, honest answer. Cheeky jokes should be about Sprout itself. Spontaneous remarks and care reactions aim for at most ten words.
- **Voice:** Tiny Sprout, Kokoro `af_heart`, speed **1.12**, pitch/formants **+6.5 semitones**, with duration compensation. This changes the actual acoustic output independently of the personality. The processing is the existing cancellable ffmpeg path, not a second audio player.
- **Phrasing:** the short pet reply is synthesized once instead of separately restarting speech after every sentence or interjection. Text captions still stream immediately; speech starts after the complete short model reply. This can slightly delay the first audio compared with sentence streaming, but gives one coherent phrase. Normal Butler keeps its sentence streaming and voice settings.

Pet-only environment overrides: `PET_TTS_VOICE`, `PET_TTS_SPEED` (0.85–1.3), `PET_TTS_PITCH` (0–12 semitones). Set these in the gateway container environment and recreate it. The original acoustic profile is `af_sky`, `1.05`, `3`. No change is made to `KOKORO_VOICE`, the rest of Butler, Haiku, Wi-Fi, firmware, saved pet identity or memory.

## Auditions and checks

[Controlled current/Tiny/Woodland samples, full settings and prompt](../art/voice-v2/README.md). Tiny Sprout is a first candidate to test with the family, not a claim that perceptual preference has been established automatically.

- 52 gateway tests pass: pet-only voice selection, environment scoping/bounds, pitch and duration at +3/+6.5/+8 semitones, ordinary Butler synthesis, captions, one pet synthesis call, existing cancellation and music order.
- 14 Butler route tests pass, retaining snapshot, account, memory, model, reward and tool isolation.
- Live Haiku evaluation covered greetings, a game suggestion, pancakes, low fullness, locked cake, pretend identity, a spontaneous thought and a feisty reaction. Review prompted tighter instructions against numeric-stat narration and teasing the child.
- Final deployed end-to-end samples check actual speech output, non-silent PCM, no clipping, short responses and the configured pet voice. Test accounts/history are removed. Service health and the real ESP's reconnection are verified. No firmware flash is needed.

## Checkpoints

HomeServer current: `4363779` (base before change `c00e0e3`). Gateway current: `a549ea8` (base `eef0d38`). Route backup: `~/pet-voice-backup/pet-pre-creature-v2.py`; gateway backup: `~/pet-voice-backup/gateway-pre-creature-v2/`. Pet workspace checkpoint before this change: `pre-creature-voice-v2` (`3566fa7`). Rollback must retain newer unrelated features; restoring the old acoustic settings via the pet-only environment overrides is the smallest change.
