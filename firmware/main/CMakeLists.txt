# Main component. Pulls in every feature component (architecture §9)
# plus the esp_timer + nvs_flash deps main.c uses directly.
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES pet_state renderer radio ui audio esp_timer nvs_flash
)
