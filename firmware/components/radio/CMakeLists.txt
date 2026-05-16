# radio — ESP-NOW beacon, handshake, shared-canvas sync, anti-griefing
# (architecture §7, docs/radio_protocol.md).
# The radio.c stub doesn't yet call any ESP-NOW APIs, so `esp_wifi` alone
# is enough. When build-order step 8 lands, add `esp_now` to REQUIRES (it
# is a standalone component in IDF 5.3.x; merged into esp_wifi in 6.x).
idf_component_register(
    SRCS "radio.c"
    INCLUDE_DIRS "include"
    REQUIRES pet_state esp_wifi
)
