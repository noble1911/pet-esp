# Firmware UI previews

These PNGs come from the real LVGL UI, rendered by `tests/host/test.c` at 368 × 448. The host uses RGB8888; the hardware uses RGB565. They are not photographs of the physical display.

`overview.png` shows the eight screens. `grown-pet.png` shows the flower decoration at 30 stars. The pet blinks during rendering, so some frames show closed eyes.

Validation on 19 September 2026:

- ESP-IDF 5.3.5 firmware build passed; app uses about 23% of the 3 MB app partition.
- USB flash data hashes verified by esptool.
- Three consecutive warm resets passed display/touch/PMIC/codec startup with no abort after the reset-sequence fix; full logs are in `boot-validation.log`.
- Host interaction and state tests passed, including real LVGL pointer input and 100 navigation cycles.
- The same host tests passed AddressSanitizer and UndefinedBehaviorSanitizer.
- All screens visually reviewed. Physical touch feel, audible volume and child playtesting were not observable remotely.

Regenerate with the commands in the repository README.
