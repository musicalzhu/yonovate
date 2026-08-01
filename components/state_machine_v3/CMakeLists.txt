idf_component_register(
    SRCS 
        "src/sm_core.cpp"       # Core 层（含 GPIO 硬件抽象）
        "src/sm_json.cpp"       # JSON 配置解析
        "src/sm_http.cpp"       # HTTP API
        "src/sm_version.cpp"    # 版本信息
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        json
        esp_http_server
        gpio_manager
        nvs_manager
        esp_timer
    PRIV_REQUIRES
        esp_common
        freertos
        esp_http_client
        log
        esp_wifi
        state_machine
)
