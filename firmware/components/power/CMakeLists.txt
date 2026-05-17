# power — AXP2101 PMIC accessors (battery percent / charging state).
# Reuses the I²C bus the Waveshare BSP brings up in bsp_i2c_init().
idf_component_register(
    SRCS "power.c"
    INCLUDE_DIRS "include"
    REQUIRES waveshare__esp32_s3_touch_amoled_1_8 driver
)
