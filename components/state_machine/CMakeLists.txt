idf_component_register(
    SRCS "state_machine_extra.cpp" "state_machine.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_timer gpio_manager freertos json esp_http_server esp_wifi motor_control
)

# 禁用结构体初始化警告（视为错误）
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)

# 确保编译器能找到标准库和ESP-IDF头文件
target_include_directories(${COMPONENT_LIB} PRIVATE
    # 标准C库头文件路径
    $ENV{IDF_PATH}/components/newlib/include
    $ENV{IDF_PATH}/components/newlib/include/newlib
    $ENV{IDF_PATH}/components/esp_common/include
    
    # FreeRTOS头文件路径
    $ENV{IDF_PATH}/components/freertos/FreeRTOS-Kernel/include
    $ENV{IDF_PATH}/components/freertos/esp_additions/include/freertos
    
    # ESP-IDF组件头文件路径
    $ENV{IDF_PATH}/components/esp_http_server/include
    $ENV{IDF_PATH}/components/esp_system/include
    $ENV{IDF_PATH}/components/log/include
    $ENV{IDF_PATH}/components/esp_timer/include
)