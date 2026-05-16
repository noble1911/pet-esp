# renderer — LVGL display driver, sprite loader, layered composition
# (architecture §5). Bring-up runs through the Waveshare BSP, which itself
# pulls SH8601, FT5x06, esp_lvgl_port, and LVGL. joltwallet__littlefs backs
# the flash-resident sprite library (declared in main/idf_component.yml).
idf_component_register(
    SRCS "renderer.c"
    INCLUDE_DIRS "include"
    REQUIRES pet_state waveshare__esp32_s3_touch_amoled_1_8 lvgl__lvgl
             joltwallet__littlefs
)

# Suppress -Wformat-truncation: every snprintf in renderer.c builds paths
# from controlled internal strings (ASSETS_BASE, fixed STAGE/LAYER dirs,
# directory names read via readdir on our own LittleFS tree). GCC's worst-
# case analysis assumes NAME_MAX (255) for every %s and can't prove the
# combined inputs fit, even at PATH_MAX_LEN=512. No actual overflow risk.
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format-truncation)
