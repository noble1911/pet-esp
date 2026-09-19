# Before Little Meadow

`pre-pet-flash.bin` is the complete 16 MB flash read from USB-connected ESP32-S3 MAC `3c:dc:75:6e:31:04` before replacing its firmware, including its partition table and NVS. It is local-only and gitignored. `SHA256SUMS` records its digest. `ui-before-overhaul.c` preserves the pre-existing uncommitted UI scene-pivot fix along with the old interface.

To restore the **entire previous device state**, from the repository root (this overwrites the new game and its saved progress):

```sh
/Users/ron/.espressif/tools/python/v5.3.5/venv/bin/python -m esptool \
  --chip esp32s3 --port /dev/cu.usbmodem101 --baud 460800 \
  write_flash 0x0 backup/2026-09-19/pre-pet-flash.bin
```

There is no need to restore for ordinary play or firmware updates.
