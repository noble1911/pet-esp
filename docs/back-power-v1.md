# Back button and PWR sleep

## Back

Every top-left back button now has a 64 × 56 visible tile inside an 88 × 76 corner touch target. The area extends to the screen edges, making imprecise finger taps easier. All screens share this helper, including Special treats, My pet, individual traits and Start fresh. Destinations stay the same; the hit area stops above Options' first row.

## PWR behaviour

A single short PWR press saves the current pet, cancels incomplete activities, stops speech and mutes the speaker. A brief sleeping-pet screen appears before the AXP2101 is asked to power the toy off. The next PWR press powers it back on at home, loading the same pet and rewards. Needs remain paused during the powered-off period, as on every existing cold boot.

This is **saved power-off**, not RAM-retaining standby or ESP light sleep. Wake is a normal boot, so it takes a few seconds and existing session-only settings (volume/mute) return to their startup defaults. The old long-press hardware behaviour is left intact. No new care star is given for sleeping this way.

The original Waveshare board routes PWR to the AXP2101 PWRON input and AXP_IRQ to TCA9554 EXIO5; the expander interrupt is not wired to an ESP wake pin. Using the PMIC's normal power-off command avoids guessing an unavailable ESP wake GPIO or altering individual rail voltages.

Register handling uses AXP2101 short-press enable `0x41[3]`, write-one-to-clear status `0x49[3]`, and power-off `0x10[0]`. Read/modify/write preserves unrelated settings. Only the ONLEVEL field in `0x27[1:0]` is set to 128 ms to allow a short wake press; OFFLEVEL and IRQLEVEL are preserved. Boot clears a stale PWR event, and the UI ignores presses during its first second to avoid immediately shutting off after wake.

If the save fails, no power command is issued. If I2C fails, the toy stays awake and displays an error. If the chip acknowledges but the ESP remains running after 1.5 seconds, the UI restores the home screen and the previous mute state. This makes USB-specific behaviour visible rather than leaving a stuck goodbye screen.

## Validation and pending hardware check

- Production PMIC code tested with a register-level I2C fake: short press consumed once, stale boot flag, unrelated/long-press flags preserved, wake timing fields, power-off read/modify/write, and read/write/setup failure handling.
- Real LVGL tests hit all four back-target corners across nine screens and check that the adjacent Options row remains tappable.
- PWR UI tests cover startup suppression, saving the complete pet, cancelling an unfinished meal, one power command, save failure, I2C failure, hardware-not-off timeout and mute restoration.
- Existing full host regression suite and ESP-IDF 5.3.5 build pass. `scripts/test_power.sh` runs the focused PMIC tests.
- **Not flashed:** the user disconnected the device for testing. Actual power-down, short-press wake, USB-connected behaviour, battery drain and physical finger accuracy remain to be checked on the board. No battery-current reduction has been measured.

Rollback: `pre-back-power-v1` points to `53990ef` (speech/meal polish). No save schema, Wi-Fi credentials, backend or model changes.

Hardware references: [original board schematic](https://files.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.8/ESP32-S3-Touch-AMOLED-1.8.pdf), [AXP2101 datasheet, sections 6.5.4 and 6.13](https://files.waveshare.com/wiki/common/X-power-AXP2101_SWcharge_V1.0.pdf).

[UI previews](previews/back-power-v1/README.md).
