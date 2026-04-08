idf_component_register(
    SRCS "gpio_manager.cpp"
    INCLUDE_DIRS "include"
    REQUIRES ssd1306 json esp_adc driver aw9523
)