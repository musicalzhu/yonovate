/**
 * @file wifi_config_store.h
 * @brief WiFi配置NVS存储组件头文件
 * @details 提供WiFi SSID和密码的持久化存储功能
 * @author AI Assistant
 * @date 2024
 */

#pragma once

#include "esp_err.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化WiFi配置存储
 * @return esp_err_t ESP_OK成功，其他错误码失败
 */
esp_err_t wifi_config_store_init(void);

/**
 * @brief 从NVS加载WiFi配置
 * @param ssid 输出缓冲区，存储SSID
 * @param ssid_len SSID缓冲区大小
 * @param pass 输出缓冲区，存储密码
 * @param pass_len 密码缓冲区大小
 * @return esp_err_t ESP_OK成功，其他错误码失败
 */
esp_err_t wifi_config_store_load(char *ssid, size_t ssid_len, char *pass, size_t pass_len);

/**
 * @brief 保存WiFi配置到NVS
 * @param ssid WiFi SSID字符串
 * @param password WiFi密码字符串
 * @return esp_err_t ESP_OK成功，其他错误码失败
 */
esp_err_t wifi_config_store_save(const char *ssid, const char *password);

/**
 * @brief 擦除NVS中存储的WiFi配置
 * @return esp_err_t ESP_OK成功，其他错误码失败
 */
esp_err_t wifi_config_store_erase(void);

#ifdef __cplusplus
}
#endif
