#!/bin/bash
# Restore the last vector-art firmware; leaves the pet's NVS save untouched.
set -euo pipefail
cd "$(dirname "$0")"
PET_PYTHON="${PET_PYTHON:-$HOME/.espressif/tools/python/v5.3.5/venv/bin/python}"
exec "$PET_PYTHON" -m esptool --chip esp32s3 --port "${1:-/dev/cu.usbmodem101}" --baud 460800 write_flash --flash_mode dio --flash_freq 80m --flash_size 16MB 0x0 bootloader.bin 0x8000 partition-table.bin 0x10000 pet_esp.bin 0x310000 assets.bin
