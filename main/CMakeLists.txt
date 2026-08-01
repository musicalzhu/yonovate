# 根据构建模式选择源文件
if(IDF_BUILD_TESTS)
    set(MAIN_SRCS "test_app_main.cpp")
else()
    set(MAIN_SRCS "main.cpp")
endif()

idf_component_register(
    SRCS ${MAIN_SRCS}
    INCLUDE_DIRS "" "../components/i2c_manager/include" "../components/aw9523/include"
    REQUIRES driver ssd1306 wifi_manager web_server gpio_monitor i2c_manager aw9523 freertos state_machine_v3
    PRIV_REQUIRES unity
)


# 设置C++17编译选项，适用于.cpp文件
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++17)