idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "" "../components/i2c_manager/include" "../components/aw9523/include"
    REQUIRES driver ssd1306 wifi_manager web_server gpio_monitor i2c_manager aw9523 freertos wifi_config_store
)

# 设置C++17编译选项，适用于.cpp文件
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++17)