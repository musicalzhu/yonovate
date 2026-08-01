idf_component_register(
    SRCS "gpio_monitor.cpp"
    INCLUDE_DIRS "include"
    REQUIRES gpio_manager
    PRIV_REQUIRES state_machine state_machine_v3 esp_netif esp_wifi
)