# Sprout voice auditions

Generated through the existing local Kokoro service, 2026-09-19. No live
voice configuration was changed. These are audition candidates, not a chosen
production voice. Pitch shifts need a pet-only gateway implementation before
use on the ESP; the other Butler users should retain their current processing.

Phrase: "Hi! I'm Sprout! My little leaves are wiggling. Shall we find a snack, or play a game?"

- `01-current-emma.wav`: bf_emma, speed 1.0, no pitch change.
- `02-bright-sprout.wav`: af_sky, speed 1.05, raised 3 semitones.
- `03-little-creature.wav`: am_puck, speed 1.05, raised 6 semitones.

Each audition is mono PCM16 at 16 kHz, matching the ESP playback format.
Pitch processing uses ffmpeg asetrate followed by aresample and inverse atempo
so the pitch/formants rise without a corresponding jump in speaking speed.
This intentionally stylises the voice; it is not naturalistic voice cloning.
Raw service WAV files retain streaming length headers; use the numbered,
normalised WAVs for playback and duration measurements.
