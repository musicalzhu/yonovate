# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# Add additional component directories
set(EXTRA_COMPONENT_DIRS
    components/ssd1306
    components/wifi_manager
    components/web_server
    components/gpio_manager
    components/gpio_monitor
    components/i2c_manager
    components/state_machine
    components/logic_circuit
    components/gpio_control
    components/motor_control
    components/system_status
    components/aw9523
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(yonovate-v2)