idf_component_register(
    SRCS "wifi_manager.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi nvs_flash esp_netif nvs_manager web_server)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++17)
