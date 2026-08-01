/**
 * @file wifi_config_store.cpp
 * @brief WiFi配置NVS存储组件 (使用 nvs_manager)
 * @details 简化版：使用统一 NVS 封装层
 */

#include "wifi_config_store.h"
#include <cstring>
#include "esp_log.h"
#include "nvs_manager.h"

static const char *TAG = "wifi_cfg";      // 日志标签
static const char *NVS_NS = "wifi_cfg";   // NVS命名空间
static const char *KEY_SSID = "ssid";     // SSID键名
static const char *KEY_PASS = "pass";     // 密码键名

/**
 * @brief 从NVS加载WiFi配置
 * @param ssid 输出缓冲区，存储SSID
 * @param ssid_len SSID缓冲区大小
 * @param pass 输出缓冲区，存储密码
 * @param pass_len 密码缓冲区大小
 * @return esp_err_t ESP_OK成功，其他错误码失败
 * @note 调用前需确保缓冲区足够大（SSID最大33字节，密码最大65字节）
 */
esp_err_t wifi_config_store_load(char *ssid, size_t ssid_len, char *pass, size_t pass_len)
{
    if (!ssid || !pass) return ESP_ERR_INVALID_ARG;

    esp_err_t err = nvs_mgr_get_string(NVS_NS, KEY_SSID, ssid, ssid_len);
    if (err != ESP_OK) return err;

    err = nvs_mgr_get_string(NVS_NS, KEY_PASS, pass, pass_len);
    
    if (err == ESP_OK) {
        char pass_masked[64] = {0};
        size_t pass_real_len = strlen(pass);
        if (pass_real_len <= 4) {
            strncpy(pass_masked, "****", sizeof(pass_masked) - 1);
        } else {
            snprintf(pass_masked, sizeof(pass_masked), "%.2s****%.2s", pass, pass + pass_real_len - 2);
        }
        ESP_LOGI(TAG, "WiFi config loaded: SSID=%s, Pass=%s", ssid, pass_masked);
    }
    return err;
}

/**
 * @brief 保存WiFi配置到NVS
 * @param ssid WiFi SSID字符串
 * @param password WiFi密码字符串
 * @return esp_err_t ESP_OK成功，其他错误码失败
 * @note SSID和密码长度必须符合802.11标准
 */
esp_err_t wifi_config_store_save(const char *ssid, const char *password)
{
    if (!ssid || !password) return ESP_ERR_INVALID_ARG;

    esp_err_t err = nvs_mgr_set_string(NVS_NS, KEY_SSID, ssid);
    if (err == ESP_OK) {
        err = nvs_mgr_set_string(NVS_NS, KEY_PASS, password);
    }
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "WiFi config saved: %s", ssid);
    }
    return err;
}

/**
 * @brief 擦除NVS中存储的WiFi配置
 * @return esp_err_t ESP_OK成功，其他错误码失败
 * @note 擦除后下次启动将使用默认配置
 */
esp_err_t wifi_config_store_erase(void)
{
    return nvs_mgr_erase_namespace(NVS_NS);
}
