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
 * @brief 更新WiFi凭证并重新连接
 * 
 * @param ssid 新的SSID
 * @param password 新的密码
 * @return esp_err_t
 */
esp_err_t wifi_manager_update_credentials(const char* ssid, const char* password);

/**
 * @brief 反初始化WiFi管理器
 * 
 * @return esp_err_t
 */
esp_err_t wifi_manager_deinit(void);

/**
 * @brief 智能双模式启动：先尝试连接STA，超时成功后关闭AP省电，失败则保持AP配网模式
 * 
 * @param config WiFi配置（STA要连接的WiFi）
 * @param ap_ssid AP模式的SSID（设备发出的热点名）
 * @param ap_password AP模式的密码
 * @param timeout_ms STA连接超时时间（毫秒），建议30000（30秒）
 * @return esp_err_t ESP_OK表示成功启动
 * @note 连接成功后会关闭AP进入纯STA模式；连接失败则进入纯AP配网模式
 */
esp_err_t wifi_manager_start_smart_dual_mode(const wifi_manager_config_t* config,
                                              const char* ap_ssid,
                                              const char* ap_password,
                                              uint32_t timeout_ms);

/**
 * @brief 启动AP配网模式
 * @details 开启设备热点，提供配网页面/API供用户配置WiFi
 * @param ap_ssid AP热点SSID名称（设备发出的热点名）
 * @param ap_password AP热点密码（建议8位以上，NULL表示开放网络）
 * @return esp_err_t ESP_OK成功，其他错误码失败
 * @note 启动后会停止STA模式，仅作为AP工作
 */
esp_err_t wifi_manager_start_ap_provisioning(const char* ap_ssid, const char* ap_password);

/**
 * @brief 停止AP配网模式
 * @details 关闭AP热点，切换回STA模式
 * @return esp_err_t ESP_OK成功，其他错误码失败
 */
esp_err_t wifi_manager_stop_ap_provisioning(void);

/**
 * @brief 启动延迟关闭AP配网模式
 * @details 在指定延迟时间后，如果STA已连接则自动关闭AP以省电
 * @param delay_ms 延迟时间（毫秒），建议30000（30秒）
 * @return esp_err_t ESP_OK成功启动延迟关闭任务，其他错误码失败
 * @note 此函数用于保存WiFi配置后，给设备时间连接WiFi，成功后自动关闭AP
 */
esp_err_t wifi_manager_start_delayed_ap_shutdown(uint32_t delay_ms);

/**
 * @brief 检查当前是否处于AP配网模式
 * @return true 当前处于AP模式
 * @return false 当前不是AP模式（STA或未启动）
 */
bool wifi_manager_is_ap_mode(void);

#ifdef __cplusplus
}
#endif
