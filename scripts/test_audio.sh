#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
PET_AUDIO_TMP="$(mktemp -d)"
trap 'rm -rf "$PET_AUDIO_TMP"' EXIT
cc -std=c11 -Wall -Wextra -Werror -I firmware/components/audio/include tests/audio/test_sound.c firmware/components/audio/sound_synth.c -lm -o "$PET_AUDIO_TMP/sound_test"
cc -std=c11 -Wall -Wextra -Werror -I tests/audio/stubs -I firmware/components/audio/include tests/audio/test_playback.c firmware/components/audio/sound_synth.c -lm -o "$PET_AUDIO_TMP/playback_test"
"$PET_AUDIO_TMP/playback_test"
mkdir -p docs/previews/sound-music-v1
(cd docs/previews/sound-music-v1 && "$PET_AUDIO_TMP/sound_test")
