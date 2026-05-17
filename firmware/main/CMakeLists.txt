# Main component. Pulls in every feature component (architecture §9)
# plus the esp_timer + nvs_flash deps main.c uses directly.
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES pet_state renderer radio ui audio power esp_timer nvs_flash
)

# Pack the sprite_forge-built assets (firmware/assets — generated, gitignored)
# into the `assets` LittleFS partition so `idf.py flash` ships them
# (architecture §2/§5: sprite library is flash-resident for the MVP).
littlefs_create_partition_image(assets "${CMAKE_CURRENT_SOURCE_DIR}/../assets" FLASH_IN_PROJECT)
