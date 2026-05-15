# pet_state — Pet struct, NVS save/load, decay, evolution (architecture §4, §6)
idf_component_register(
    SRCS "pet_state.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash
)
