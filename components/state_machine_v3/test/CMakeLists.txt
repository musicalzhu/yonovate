# State Machine V3 Test Component
# ESP-IDF 官方标准测试子目录
# 仅在 IDF_BUILD_TESTS 时构建

idf_component_register(
    SRCS 
        "test_state_machine_v3.cpp"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        state_machine_v3
        gpio_manager
        i2c_manager
    PRIV_REQUIRES 
        unity
        freertos
        esp_common
)
