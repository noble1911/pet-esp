// power implementation — minimal AXP2101 register reads over I²C.
//
// We only need two pieces of state for the menu's battery row:
//   - battery percent (register 0xA4, single byte, 0..100, 0xFF=not ready)
//   - charging state  (register 0x01, bit 0..1 of CHARGE_STATUS encodes
//                      the charge phase; non-zero = actively charging)
//
// Datasheet refs: AXP2101 datasheet rev. 1.0, sections 7.2 (status reg)
// and 7.10 (battery fuel-gauge). The board's I²C address is the
// default 0x34.

#include "power.h"

#include "bsp/esp-bsp.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "power";

#define AXP2101_I2C_ADDR     0x34
#define AXP2101_I2C_HZ      400000
// Register addresses (AXP2101 datasheet, register map).
#define REG_CHARGE_STATUS    0x01   // bits[2:0] = charge state machine
#define REG_BATT_PERCENT     0xA4   // 0..100, 0xFF when fuel gauge cold

static i2c_master_dev_handle_t s_dev;
static bool s_ok;

// Single-byte register read. Returns false on I²C error; *out untouched.
// 50 ms timeout is generous — the PMIC is fast, but the shared bus has
// touch / RTC / codec on it and may be busy.
static bool read_reg(uint8_t reg, uint8_t *out)
{
    if (!s_ok) {
        return false;
    }
    esp_err_t e = i2c_master_transmit_receive(s_dev, &reg, 1, out, 1, 50);
    if (e != ESP_OK) {
        ESP_LOGW(TAG, "i2c read reg 0x%02x failed: %s", reg, esp_err_to_name(e));
        return false;
    }
    return true;
}

bool power_init(void)
{
    i2c_master_bus_handle_t bus = bsp_i2c_get_handle();
    if (bus == NULL) {
        ESP_LOGE(TAG, "BSP I²C bus not initialised — call bsp_i2c_init first");
        return false;
    }
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = AXP2101_I2C_ADDR,
        .scl_speed_hz    = AXP2101_I2C_HZ,
    };
    esp_err_t e = i2c_master_bus_add_device(bus, &cfg, &s_dev);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "add AXP2101 device failed: %s", esp_err_to_name(e));
        return false;
    }
    s_ok = true;
    // Touch the chip once at boot to log presence — surfaces a wiring
    // issue early instead of waiting for the first menu open to notice
    // -1 / false readings.
    uint8_t status;
    if (read_reg(REG_CHARGE_STATUS, &status)) {
        ESP_LOGI(TAG, "AXP2101 online (status=0x%02x)", status);
    } else {
        ESP_LOGW(TAG, "AXP2101 not responding — battery readouts will be -1");
        s_ok = false;
    }
    return s_ok;
}

int power_battery_percent(void)
{
    uint8_t v;
    if (!read_reg(REG_BATT_PERCENT, &v)) {
        return -1;
    }
    if (v == 0xFF) {
        // Fuel gauge hasn't settled (typical right after boot). Caller
        // can show a "—" instead of a percent until this returns >=0.
        return -1;
    }
    if (v > 100) {
        v = 100;
    }
    return v;
}

bool power_is_charging(void)
{
    uint8_t status;
    if (!read_reg(REG_CHARGE_STATUS, &status)) {
        return false;
    }
    // AXP2101: bits[2:0] of REG_CHARGE_STATUS encode the charge-state
    // machine. 0b001 = trickle, 0b010 = pre-charge, 0b011 = constant
    // current, 0b100 = constant voltage. All others (incl. 0=standby,
    // 5=done, 6=not charging, 7=fault) we treat as "not charging" so
    // the UI shows a steady bar instead of a charging glyph.
    uint8_t phase = status & 0x07;
    return phase >= 1 && phase <= 4;
}
