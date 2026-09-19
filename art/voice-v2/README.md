# Creature voice auditions

Produced using the existing local Kokoro service and the gateway's existing ffmpeg pitch/formant processing. Audio is mono PCM16 at 16 kHz, matching the ESP stream. This is a stylised synthetic voice; no voice cloning or new paid voice API is used.

Same text in all three controlled auditions: **Ooh, it's you! Little leaf wiggles! That cupcake looks yummy.**

| File | Kokoro voice | Speed | Pitch/formant shift |
|---|---|---:|---:|
| `01-current.wav` | af_sky | 1.05 | +3 semitones |
| `02-tiny-sprout.wav` | af_heart | 1.12 | +6.5 semitones |
| `03-woodland-sprout.wav` | am_puck | 1.10 | +8 semitones |

**Tiny Sprout is the deployed first candidate**, selected as a stronger creature direction. The user and family should judge its sound on the real toy; automated waveform checks cannot decide cuteness. The two other files make comparison easy. `auditions.json` records duration, peak and RMS for each unnormalised production-format audition; the recordings have not been made louder to favour a candidate.

`creature-voice-*.wav` and `creature-live-results.json` are actual deployed Haiku → gateway → Kokoro replies using a temporary test pet. That account and its conversation history are deleted after the test. The real pet's memory is not touched. The self-directed teasing instruction was refined after a live check returned a less gentle answer.

The exact personality section is in [personality-prompt.md](personality-prompt.md). Full rules and the final speaking direction remain in `integrations/butler/pet.py`.

Reproduce the fixed-text auditions with the claude-esp gateway on `PYTHONPATH`, using its virtualenv to run `integrations/scripts/audition_pet_voice.py`. Run `integrations/scripts/live_creature_voice_test.py` from this Mac for a live deployed check. `audition_pet_personality.py` runs eight broader text scenarios inside butler-api; it is an evaluation script, not a deterministic assertion that a model will always follow the style.
