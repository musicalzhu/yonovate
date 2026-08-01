idf_component_register(SRCS "web_server.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server logic_circuit gpio_control motor_control system_status state_machine state_machine_v3 wifi_config_store wifi_manager gpio_monitor)