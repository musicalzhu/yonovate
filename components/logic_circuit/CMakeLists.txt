idf_component_register(
    SRCS "logic_circuit.cpp"
    INCLUDE_DIRS "include"
    REQUIRES gpio_manager json freertos esp_http_server driver
)