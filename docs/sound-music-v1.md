# Sounds and little tunes

## Using it

- Care now has distinct sounds: apple/toast/cookie crunch patterns, cuddles, star sparkles, bubble pops, a pitch-bending ball bounce, Peekaboo hints/reveals, a sleepy phrase, clean-bath flourish, sticker chimes, room-gift fanfare, selection ticks and butterfly whistles.
- **Play → Music** opens three original offline tunes: Twinkly meadow (bells), Bouncy dance (plucked notes), Sleepy leaves (flute). Tap a different tune to replace the current one. Stop music or leave the music page to stop local playback.
- **Sprout, make me a tune!** asks Haiku for a new instrumental tune. It needs Wi-Fi and the pet voice server; it works even when Little chats is off. Or hold BOOT/Talk and ask for a happy tune, sleepy tune, etc. No separate conversation screen is needed.
- Generated tunes follow a brief introduction. Stop music or hold BOOT/Talk to interrupt. Existing mute and volume controls apply, with default volume still 100%. Music earns no care stars and changes no saved pet state.
- These are short instrumental melodies, not sung lyrics. The device does not import MIDI files; it plays local synthesized patterns or streamed PCM. An example score can be exported as a standard MIDI file for other players.

## Implementation

The ESP's pure C `sound_synth` renders bounded monophonic note sequences into the existing 16 kHz speaker stream. Six timbres (bell, pluck, flute, noise crunch, decaying pop and swept bounce) use short attack/release envelopes and modest fixed amplitude. It allocates no new streaming buffer. The one existing speaker task owns effects, tunes and speech; speech/microphone activity discards local sound, and generation counters prevent stopped or muted tunes from resuming. A single pending effect means rapid taps never create a long chirp backlog.

The dedicated pet Haiku route exposes `compose_tune` only during requested turns, alongside pet-scoped memory. It never gives this tool to normal Butler or spontaneous pet remarks. One score per turn: bell/pluck/flute, 60–150 BPM, 4–24 ordered `[MIDI pitch, duration ticks]` pairs, pitch 48–84 or 0 for rest, 1–8 ticks per note (four ticks per beat), and at most twelve seconds. Haiku is guided toward eight notes / at most 48 ticks. No executable code, audio URLs, files or paid music-generation API are involved.

The gateway revalidates each score, synthesizes PCM with NumPy, queues it after spoken text, and uses the existing paced and cancellable audio sender. Pet streaming now runs at playback speed after its initial one-second burst, avoiding a growing backlog in the ESP's 96 KB audio buffer. Normal Butler's model, voice and playback pace are unchanged. Generated tunes are not stored as a permanent on-device collection; the three offline tunes remain available without the server.

## Validation and auditions

- `./scripts/test_audio.sh`: actual C renderer verifies distinct effect waveforms, output level bounds, bounded duration, silent tails and deterministic chunk timing; actual speaker-owner tests cover Stop, mute, capture, speech priority, no resumed/stale tunes and latest-effect replacement. Writes WAV examples under `docs/previews/sound-music-v1`.
- Existing LVGL pointer suite plus Music navigation, three tune choices, compose, mute blocking, leaving/stopping, BOOT interruption/listening feedback, sound routing and unchanged care stars. [Actual Music screen](previews/sound-music-v1/music.png).
- Ten Butler tests cover account/memory/model isolation, bounded pattern validation, one score per turn and structured score output. Forty-eight gateway tests cover PCM pitch/rest/duration, malformed scores, voice-before-music ordering, pet-only/request-only playback and cancellation.
- Live Haiku generated bouncy and sleepy examples. A full speech → STT → Haiku tool → SSE → pet-voice introduction → PCM test produced **Happy Little Day**, a six-second tune. Exact score and audio are [here](previews/sound-music-v1/README.md). The temporary test account and history were deleted.
- ESP-IDF 5.3.5 build and verified USB flash. Boot confirms saved Sprout identity/stage, display/touch, 96 KB audio stream, Wi-Fi and pet gateway ready. About 43% of the 3 MB application partition remains free. A physical listening/playtesting pass can still refine timbre preferences.

## Rollback

Pet firmware checkpoint: **`pre-sound-music-v1`** (`d3fb8ee`). Follow the separate-worktree build/flash procedure in [playtime rollback](playtime-v1.md), substituting that tag. No NVS migration or reset is required.

Backend pre-change checkpoints: HomeServer `8304e27`; claude-esp gateway `1ae8cf6`. Current music backend: HomeServer `2f77060`, gateway `eef0d38`. Remote copies of the old pet route and gateway modules are under `~/pet-voice-backup/pet-pre-music.py` and `~/pet-voice-backup/gateway-pre-music/` on the Mac mini. Older firmware can still play generated PCM with the updated server; new firmware's offline effects/tunes also work without the new backend.
