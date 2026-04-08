idf_component_register(SRCS "motor_control.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server json gpio_manager)