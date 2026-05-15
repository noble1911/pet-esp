# radio — ESP-NOW beacon, handshake, shared-canvas sync, anti-griefing
# (architecture §7, docs/radio_protocol.md).
idf_component_register(
    SRCS "radio.c"
    INCLUDE_DIRS "include"
    REQUIRES pet_state esp_wifi esp_now
)
