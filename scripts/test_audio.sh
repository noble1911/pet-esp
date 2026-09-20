#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
PET_AUDIO_TMP="$(mktemp -d)"
trap 'rm -rf "$PET_AUDIO_TMP"' EXIT
cc -O2 -std=c11 -Wall -Wextra -Werror -I firmware/components/audio/include tests/audio/test_sound.c firmware/components/audio/sound_synth.c -lm -o "$PET_AUDIO_TMP/sound_test"
cc -O2 -std=c11 -Wall -Wextra -Werror -I tests/audio/stubs -I firmware/components/audio/include tests/audio/test_playback.c firmware/components/audio/sound_synth.c -lm -o "$PET_AUDIO_TMP/playback_test"
"$PET_AUDIO_TMP/playback_test"
PET_AUDIO_OUTPUT="${1:-$PET_AUDIO_TMP/auditions}"
mkdir -p "$PET_AUDIO_OUTPUT"
(cd "$PET_AUDIO_OUTPUT" && "$PET_AUDIO_TMP/sound_test")
