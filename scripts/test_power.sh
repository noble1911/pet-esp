#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
PET_POWER_TEST=$(mktemp -d /tmp/pet-power-test.XXXXXX)
trap 'rm -rf "$PET_POWER_TEST"' EXIT
cc -std=c11 -Wall -Wextra -Werror -Itests/power/stubs -Itests/host/stubs \
  -Ifirmware/components/power/include tests/power/test_power.c -o "$PET_POWER_TEST/test_power"
"$PET_POWER_TEST/test_power"
