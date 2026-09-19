// AXP2101 battery status, PWR short-press events and saved power-off.
// Board schematic: PWR -> PWRON, AXP_IRQ -> EXIO5 (no ESP wake GPIO).
// Datasheet: reg41[3] enable; reg49[3] W1C short press; reg10[0] power off.

#include "power.h"

#include "bsp/esp-bsp.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "power";

#define AXP2101_I2C_ADDR     0x34
#define AXP2101_I2C_HZ      400000
// Register addresses (AXP2101 datasheet, register map).
#define REG_CHARGE_STATUS    0x01   // bits[2:0] = charge state machine
#define REG_COMMON           0x10
#define REG_KEY_TIMING       0x27
#define REG_IRQ_ENABLE1      0x41
#define REG_IRQ_STATUS1      0x49
#define PWR_SHORT_IRQ        (1u << 3)
#define REG_BATT_PERCENT     0xA4   // 0..100, 0xFF when fuel gauge cold

static i2c_master_dev_handle_t s_dev;
static bool s_ok, s_key_ready;

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

static bool write_reg(uint8_t reg,uint8_t value)
{
    if(!s_ok)return false;
    const uint8_t data[]={reg,value};
    esp_err_t e=i2c_master_transmit(s_dev,data,sizeof data,50);
    if(e!=ESP_OK)ESP_LOGW(TAG,"i2c write reg 0x%02x failed: %s",reg,esp_err_to_name(e));
    return e==ESP_OK;
}

bool power_init(void)
{
    s_ok=false;s_key_ready=false;
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
    if(s_ok) {
        uint8_t enabled,timing;
        // Drop a latched boot/wake press before the UI starts polling. Set
        // only ONLEVEL to 128 ms; long-press shutdown timing stays unchanged.
        s_key_ready=read_reg(REG_IRQ_ENABLE1,&enabled) &&
            write_reg(REG_IRQ_ENABLE1,enabled|PWR_SHORT_IRQ) &&
            write_reg(REG_IRQ_STATUS1,PWR_SHORT_IRQ) &&
            read_reg(REG_KEY_TIMING,&timing) && write_reg(REG_KEY_TIMING,timing&~3u);
        if(!s_key_ready)ESP_LOGW(TAG,"PWR short-press setup unavailable");
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

bool power_take_short_press(void)
{
    uint8_t status;
    if(!s_key_ready || !read_reg(REG_IRQ_STATUS1,&status) || !(status&PWR_SHORT_IRQ))return false;
    // Return it only after acknowledgement succeeds, so a failed I2C write
    // cannot turn one latched event into repeated sleep requests.
    return write_reg(REG_IRQ_STATUS1,PWR_SHORT_IRQ);
}

bool power_request_off(void)
{
    uint8_t config;
    // Hardware power-off is the low-power sleep mode for this board. Preserve
    // reset, discharge and other common-config bits; never rewrite rail voltages.
    return s_key_ready && read_reg(REG_COMMON,&config) && write_reg(REG_COMMON,config|1u);
}
