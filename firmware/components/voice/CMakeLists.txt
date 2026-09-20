idf_component_register(SRCS "voice.c" INCLUDE_DIRS "include"
    REQUIRES audio pet_state esp_wifi esp_event esp_netif esp_websocket_client json driver nvs_flash)

if(PET_IDENTITY_HEADER)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE PET_IDENTITY_HEADER="${PET_IDENTITY_HEADER}")
endif()
