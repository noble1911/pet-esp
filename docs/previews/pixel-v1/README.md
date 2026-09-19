# Pixel-art v1: real firmware UI renders

These are actual LVGL renders of the production C code at 368 × 448, not the imagegen concept. `overview.png` shows eight screens; `before-after.png` compares the preserved vector version with pixel v1; `coats-and-growth.png` shows the variants. Individual screens and expression frames are included.

Validation: ESP-IDF 5.3.5 build; gameplay, state and expression tests using LVGL pointer input; 100 screen changes; AddressSanitizer and UndefinedBehaviorSanitizer; visual review of all screens/colours/stages; SHA256 verification of the separate vector-v1 rollback binaries.

The ESP was not connected during this build. On-device rendering speed, RAM headroom, touch/audio and boot still need validation after upload. Earlier boot logs in other preview folders belong to the previous vector firmware.

No changes to care rewards, decay, identity, save format, or growth thresholds. Night background is tied to the nap activity, not an RTC day/night cycle. The artwork is a first implementation of the approved direction; not every decorative detail from the concept is reproduced.
