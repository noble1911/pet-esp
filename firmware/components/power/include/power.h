// power — AXP2101 PMIC accessors. Reports battery percent and charging
// state for the device-status row in the menu modal.
//
// Architecture §1: the board has an AXP2101 for USB-C charging and rail
// switching; the Waveshare BSP doesn't wrap it, so we talk to it
// directly over the shared I²C bus the BSP already initialised.

#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Attach an I²C device handle for the AXP2101 onto the bus that
// bsp_i2c_init() already brought up. Safe to call after renderer_init.
// Returns true on success; false leaves the rest of the API as no-ops
// (so a bad PMIC doesn't take the whole UI down).
bool power_init(void);

// Battery percent 0..100, or -1 if the gauge isn't ready / I²C failed.
// The AXP2101's on-chip fuel gauge needs a few seconds after boot to
// settle; expect -1 reads early on.
int power_battery_percent(void);

// True while USB-C is supplying the battery. Distinct from "plugged in
// but full" — that returns false here.
bool power_is_charging(void);

#ifdef __cplusplus
}
#endif
