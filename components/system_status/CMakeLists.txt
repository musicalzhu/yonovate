idf_component_register(SRCS "system_status.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_server json gpio_manager
                    PRIV_REQUIRES spi_flash esp_timer)