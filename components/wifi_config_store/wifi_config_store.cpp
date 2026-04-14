/**
 * @file wifi_config_store.cpp
 * @brief WiFi配置NVS存储组件
 * @details 提供WiFi SSID和密码的持久化存储功能，支持保存、加载和擦除操作
 * @author AI Assistant
 * @date 2024
 */

#include "wifi_config_store.h"
#include <cstring>
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "wifi_cfg";      // 日志标签
static const char *NVS_NS = "wifi_cfg";   // NVS命名空间
static const char *KEY_SSID = "ssid";     // SSID键名
static const char *KEY_PASS = "pass";     // 密码键名

/**
 * @brief 初始化WiFi配置存储
 * @return esp_err_t ESP_OK成功，其他错误码失败
 * @note 必须在应用启动时调用一次，初始化NVS分区
 * 
 * 示例:
 * @code
 * esp_err_t err = wifi_config_store_init();
 * if (err != ESP_OK) {
 *     ESP_LOGE(TAG, "WiFi配置存储初始化失败: %s", esp_err_to_name(err));
 * }
 * @endcode
 */
esp_err_t wifi_config_store_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

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

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    size_t len = ssid_len;
    err = nvs_get_str(h, KEY_SSID, ssid, &len);
    if (err != ESP_OK) {
        nvs_close(h);
        return err;
    }

    len = pass_len;
    err = nvs_get_str(h, KEY_PASS, pass, &len);
    
    // 调试日志：显示加载的配置（密码部分隐藏）
    if (err == ESP_OK) {
        // 隐藏密码中间部分，只显示前2和后2字符
        char pass_masked[64] = {0};
        size_t pass_real_len = strlen(pass);
        if (pass_real_len <= 4) {
            strncpy(pass_masked, "****", sizeof(pass_masked) - 1);
        } else {
            snprintf(pass_masked, sizeof(pass_masked), "%.2s****%.2s", pass, pass + pass_real_len - 2);
        }
        ESP_LOGI(TAG, "NVS loaded WiFi config: SSID=%s, Pass=%s (len=%d)", 
                 ssid, pass_masked, (int)pass_real_len);
    } else {
        ESP_LOGW(TAG, "NVS load password failed: %s", esp_err_to_name(err));
    }
    
    nvs_close(h);
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

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_str(h, KEY_SSID, ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(h, KEY_PASS, password);
        if (err == ESP_OK) {
            err = nvs_commit(h);
        }
    }
    
    nvs_close(h);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "WiFi配置保存成功");
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
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    
    err = nvs_erase_all(h);
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return err;
}
