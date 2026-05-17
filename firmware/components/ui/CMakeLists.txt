# ui — screens, menus, stat bars, touch handling, emote bubbles
# (architecture §6 loop / §7.4). Built on LVGL via the renderer component.
idf_component_register(
    SRCS "ui.c"
    INCLUDE_DIRS "include"
    REQUIRES pet_state renderer radio power lvgl__lvgl
)
