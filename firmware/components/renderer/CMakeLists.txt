# renderer — LVGL display driver, sprite loader, layered composition
# (architecture §5). LVGL is added as a dependency at build-order step 2.
idf_component_register(
    SRCS "renderer.c"
    INCLUDE_DIRS "include"
    REQUIRES pet_state
)
