idf_component_register(SRCS "voice.c" INCLUDE_DIRS "include"
    REQUIRES audio pet_state esp_wifi esp_event esp_netif esp_websocket_client json driver)
