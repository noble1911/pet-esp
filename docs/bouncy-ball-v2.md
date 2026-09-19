# Bouncy ball v2

Play → Bouncy ball now responds directly to each hit. The ball waits on the floor until tapped, squashes under a held finger, then spins along a 720 ms arc to a new landing spot with three trailing stars. The pet reaches toward the flight, and a landing ring, little hop and chime mark the catch. A visible count and five round pips show progress.

After landing and a brief 160 ms settling interval, it waits again. The 96 × 96 hit target stays generous. There is no time limit, missed-tap penalty or moving target to chase while deciding where to tap. The route alternates sides, mirrored between rounds for variety. Tapping during flight cannot add more bounces.

The fifth bounce still plays its full flight, then holds the landing for 650 ms before the existing celebration. Only that completed round restores happiness and earns one care star. Leaving during a flight or even the final landing pause cancels unfinished play without an award. Voice and sound continue to use their existing priority/mute controls.

## Validation

- Production LVGL pointer tests cover holding/squashing, launch feedback, new landing positions, stationary waiting, rapid extra taps, full fifth-bounce feedback, exactly one reward, and cancellation during flight/final pause.
- Both mirrored routes are checked frame by frame: the hit target stays below navigation and above the footer, within screen bounds.
- Full host regression suite and ESP-IDF 5.3.5 firmware build passed. No save schema, unlock thresholds, backend or model changes.
- The user has the ESP disconnected. This update is in the pending build and **has not been flashed or physically playtested**.

Rollback: `pre-bouncy-ball-v2` points to `1ddb93d`, retaining the earlier speech/eating and back/PWR improvements.

[Production UI animation and screenshots](previews/bouncy-ball-v2/README.md).
