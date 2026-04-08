#pragma once

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wifi_connection_callback_t)(bool connected, const char* ip_address);

/**
 * @brief WiFi管理器配置结构体
 */
typedef struct {
    const char* ssid;         /*!< WiFi SSID */
    const char* password;     /*!< WiFi 密码 */
    wifi_connection_callback_t callback;  /*!< WiFi连接状态回调函数 */
} wifi_manager_config_t;

/**
 * @brief 初始化WiFi管理器
 * 
 * @param config WiFi管理器配置
 * @return esp_err_t
 */
esp_err_t wifi_manager_init(const wifi_manager_config_t* config);

/**
 * @brief 启动WiFi连接
 * 
 * @return esp_err_t
 */
esp_err_t wifi_manager_start(void);

/**
 * @brief 停止WiFi连接
 * 
 * @return esp_err_t
 */
esp_err_t wifi_manager_stop(void);

/**
 * @brief 获取当前IP地址
 * 
 * @return const char* 当前IP地址，如果未连接则返回"0.0.0.0"
 */
const char* wifi_manager_get_ip(void);

/**
 * @brief 获取当前连接状态
 * 
 * @return true 已连接
 * @return false 未连接
 */
bool wifi_manager_is_connected(void);

/**
 * @brief 重新连接WiFi
 * 
 * @return esp_err_t
 */
esp_err_t wifi_manager_reconnect(void);

/**
 * @brief 反初始化WiFi管理器
 * 
 * @return esp_err_t
 */
esp_err_t wifi_manager_deinit(void);

#ifdef __cplusplus
}
#endif
