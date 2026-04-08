idf_component_register(
    SRCS "aw9523.cpp"
    INCLUDE_DIRS "include"
    REQUIRES i2c_manager driver esp_timer
)