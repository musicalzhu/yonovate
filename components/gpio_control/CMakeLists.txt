idf_component_register(
    SRCS "gpio_control.cpp"
    INCLUDE_DIRS "include"
    REQUIRES gpio_manager json esp_http_server driver
)