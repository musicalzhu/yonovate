#pragma once

#include "esp_err.h"
#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化WiFi管理器
 * 
 * @return esp_err_t ESP_OK 成功
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief 启动WiFi连接
 * 
 * 启动智能双模式：同时启动AP配网和STA连接尝试。
 * 连接成功则自动关闭AP进入纯STA模式，超时则保持AP配网模式。
 * 启动结果可通过 wifi_manager_is_connected(), wifi_manager_is_ap_mode(),
 * wifi_manager_get_ip() 等函数查询。
 * 
 * @return esp_err_t ESP_OK 启动成功（后续状态通过查询函数获取）
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
 * @brief 检查当前是否处于AP配网模式
 * @return true 当前处于AP模式
 * @return false 当前不是AP模式（STA或未启动）
 */
bool wifi_manager_is_ap_mode(void);

/**
 * @brief WiFi 状态枚举
 */
typedef enum {
    WIFI_STATUS_DISCONNECTED = 0,   /*!< 未连接 */
    WIFI_STATUS_CONNECTING,         /*!< 正在连接 */
    WIFI_STATUS_CONNECTED,          /*!< STA 已连接（有 IP） */
    WIFI_STATUS_AP_MODE           /*!< AP 模式启动 */
} wifi_status_t;

/**
 * @brief WiFi 状态变化回调函数类型
 * 
 * @param status 当前 WiFi 状态
 * @param ip_address IP 地址（CONNECTED 状态有效，其他为 NULL）
 * @param user_arg 用户数据（注册时传入）
 */
typedef void (*wifi_status_cb_t)(wifi_status_t status, const char* ip_address, void* user_arg);

/**
 * @brief 注册 WiFi 状态变化回调
 * 
 * 回调将在 WiFi 状态变化时被调用，支持多订阅者（最多4个）
 * 
 * @param cb 回调函数指针
 * @param user_arg 用户数据，回调时原样返回
 * @return int 回调 ID (>0)，失败返回 -1（数组已满）
 */
int wifi_manager_register_status_callback(wifi_status_cb_t cb, void* user_arg);

/**
 * @brief 注销 WiFi 状态回调
 * 
 * @param cb_id 注册时返回的 ID
 */
void wifi_manager_unregister_status_callback(int cb_id);

/**
 * @brief 获取当前 WiFi 状态
 * 
 * @return wifi_status_t 当前状态
 */
wifi_status_t wifi_manager_get_status(void);

/**
 * @brief 获取当前 IP 地址
 *
 * @return const char* IP 地址，未连接时返回 NULL
 */
const char* wifi_manager_get_ip_address(void);

/**
 * @brief 统一初始化网络服务（封装 WiFi + Web Server 初始化）
 * @return ESP_OK 成功
 * @note 内部自动处理 web_server_init 和 wifi_manager_start
 * @note 内部更新 gpio_monitor 状态
 */
esp_err_t net_init(void);

#ifdef __cplusplus
}
#endif
