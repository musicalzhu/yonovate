#include "wifi_manager.h"
#include <stdio.h>
#include <cstdio>
#include <string.h>
#include <cstring>
#include <assert.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

static const char *TAG = "wifi_manager";

// 静态变量
static bool s_wifi_initialized = false;
static bool s_wifi_connected = false;
static char s_ip_address[16] = "0.0.0.0";
static wifi_connection_callback_t s_connection_callback = NULL;
static esp_event_handler_instance_t instance_any_id = NULL;
static esp_event_handler_instance_t instance_got_ip = NULL;

// WiFi事件处理函数
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        bool was_connected = s_wifi_connected;
        s_wifi_connected = false;
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi连接断开，正在重新连接...");
        
        // 仅在状态变化时调用回调
        if (was_connected && s_connection_callback) {
            s_connection_callback(false, s_ip_address);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        snprintf(s_ip_address, sizeof(s_ip_address), IPSTR, IP2STR(&event->ip_info.ip));
        bool was_connected = s_wifi_connected;
        s_wifi_connected = true;
        ESP_LOGI(TAG, "WiFi连接成功，IP地址: %s", s_ip_address);
        
        // 仅在状态变化时调用回调
        if (!was_connected && s_connection_callback) {
            s_connection_callback(true, s_ip_address);
        }
    }
}

esp_err_t wifi_manager_init(const wifi_manager_config_t* config)
{
    // SSID必须非空，密码可为空字符串（表示开放网络）
    if (!config || !config->ssid || strlen(config->ssid) == 0) {
        ESP_LOGE(TAG, "WiFi初始化失败: SSID不能为空");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // 保存回调函数
    s_connection_callback = config->callback;

    // 初始化NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 初始化网络接口
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    
    // 创建AP网络接口（用于配网模式）
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);

    // 初始化WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 注册事件处理函数
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      &instance_got_ip));

    // 配置WiFi
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, config->ssid, sizeof(wifi_config.sta.ssid) - 1);
    // 密码可为空字符串表示开放网络（无密码）
    if (config->password && strlen(config->password) > 0) {
        strncpy((char*)wifi_config.sta.password, config->password, sizeof(wifi_config.sta.password) - 1);
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    } else {
        // 空密码时使用开放认证模式
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
        ESP_LOGI(TAG, "WiFi配置为开放网络（无密码）");
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    // 禁用WiFi电源管理，防止在接收配置时WiFi状态变化导致崩溃
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_LOGI(TAG, "WiFi电源管理已禁用");

    s_wifi_initialized = true;
    ESP_LOGI(TAG, "WiFi初始化完成");

    return ESP_OK;
}

esp_err_t wifi_manager_start(void)
{
    if (!s_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi启动失败: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "WiFi已启动");
    }
    return ret;
}

esp_err_t wifi_manager_stop(void)
{
    if (!s_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi停止失败: %s", esp_err_to_name(ret));
    } else {
        s_wifi_connected = false;
        strcpy(s_ip_address, "0.0.0.0");
        ESP_LOGI(TAG, "WiFi已停止");
        if (s_connection_callback) {
            s_connection_callback(false, s_ip_address);
        }
    }
    return ret;
}

const char* wifi_manager_get_ip(void)
{
    return s_ip_address;
}

bool wifi_manager_is_connected(void)
{
    return s_wifi_connected;
}

esp_err_t wifi_manager_reconnect(void)
{
    if (!s_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return esp_wifi_connect();
}

esp_err_t wifi_manager_update_credentials(const char* ssid, const char* password)
{
    // SSID必须非空，密码可为空字符串（开放网络）
    if (!s_wifi_initialized || !ssid || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "更新凭证失败: SSID不能为空");
        return ESP_ERR_INVALID_ARG;
    }

    // 获取当前WiFi模式
    wifi_mode_t current_mode;
    esp_err_t err = esp_wifi_get_mode(&current_mode);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "获取WiFi模式失败: %s", esp_err_to_name(err));
        return err;
    }

    // 如果当前是AP模式，需要先切换到APSTA模式
    if (current_mode == WIFI_MODE_AP) {
        ESP_LOGI(TAG, "Current AP mode, switching to APSTA for STA connection");
        err = esp_wifi_set_mode(WIFI_MODE_APSTA);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Switch to APSTA failed: %s", esp_err_to_name(err));
            return err;
        }
        ESP_LOGI(TAG, "Mode switch command sent, waiting for stabilization");
    }

    // Check if STA interface is ready (with retry)
    wifi_mode_t new_mode;
    int retry_count = 0;
    const int max_retries = 10;  // 最多等待1秒
    
    do {
        esp_wifi_get_mode(&new_mode);
        if (new_mode == WIFI_MODE_STA || new_mode == WIFI_MODE_APSTA) {
            ESP_LOGI(TAG, "STA interface ready, mode: %d", new_mode);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
        retry_count++;
        ESP_LOGI(TAG, "Waiting for STA ready... (%d/%d)", retry_count, max_retries);
    } while (retry_count < max_retries);
    
    if (new_mode != WIFI_MODE_STA && new_mode != WIFI_MODE_APSTA) {
        ESP_LOGE(TAG, "WiFi mode not suitable for STA after %d retries: %d", max_retries, new_mode);
        return ESP_ERR_WIFI_STATE;
    }

    // Disconnect current STA if any
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(50));

    // 配置新的WiFi凭证
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    // 密码可为空字符串表示开放网络
    if (password && strlen(password) > 0) {
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
    } else {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;
        ESP_LOGI(TAG, "配置为开放网络（无密码）");
    }

    err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "设置WiFi配置失败: %s", esp_err_to_name(err));
        return err;
    }

    // 使用新凭证重新连接
    err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi连接失败: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "WiFi凭证已更新，正在使用新配置连接...");
    return ESP_OK;
}

esp_err_t wifi_manager_deinit(void)
{
    if (!s_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(wifi_manager_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    
    // 清理事件处理程序
    if (instance_any_id != NULL) {
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT,
                                                          ESP_EVENT_ANY_ID,
                                                          instance_any_id));
        instance_any_id = NULL;
    }
    if (instance_got_ip != NULL) {
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT,
                                                          IP_EVENT_STA_GOT_IP,
                                                          instance_got_ip));
        instance_got_ip = NULL;
    }

    s_wifi_initialized = false;
    s_wifi_connected = false;
    strcpy(s_ip_address, "0.0.0.0");
    s_connection_callback = NULL;

    ESP_LOGI(TAG, "WiFi已反初始化");
    return ESP_OK;
}

/**
 * @brief 智能双模式启动：先尝试连接STA，30秒内成功则关闭AP省电，失败则保持AP配网模式
 * 
 * @param config WiFi配置
 * @param ap_ssid AP模式的SSID
 * @param ap_password AP模式的密码
 * @param timeout_ms STA连接超时时间（毫秒）
 * @return esp_err_t ESP_OK表示成功启动
 */
esp_err_t wifi_manager_start_smart_dual_mode(const wifi_manager_config_t* config,
                                               const char* ap_ssid,
                                               const char* ap_password,
                                               uint32_t timeout_ms)
{
    if (!config || !config->ssid || strlen(config->ssid) == 0) {
        ESP_LOGE(TAG, "Smart dual mode: Invalid config, SSID empty");
        return ESP_ERR_INVALID_ARG;
    }

    if (!ap_ssid || strlen(ap_ssid) == 0) {
        ESP_LOGE(TAG, "Smart dual mode: AP SSID cannot be empty");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "========== Smart Dual Mode Start ==========");
    ESP_LOGI(TAG, "Trying to connect WiFi: %s", config->ssid);
    ESP_LOGI(TAG, "AP provision: %s", ap_ssid);
    ESP_LOGI(TAG, "Timeout: %lu ms", timeout_ms);

    // 1. Init WiFi manager (STA mode)
    esp_err_t err = wifi_manager_init(config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Smart dual mode: WiFi manager init failed: %s", esp_err_to_name(err));
        return err;
    }

    // 2. Start AP provisioning (keep APSTA mode)
    err = wifi_manager_start_ap_provisioning(ap_ssid, ap_password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Smart dual mode: AP provision start failed: %s", esp_err_to_name(err));
        wifi_manager_deinit();
        return err;
    }

    ESP_LOGI(TAG, "APSTA dual mode started, AP: %s", ap_ssid);

    // 3. Wait STA connection or timeout
    ESP_LOGI(TAG, "Waiting for STA connection result...");
    
    TickType_t start_tick = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    bool connected = false;

    while ((xTaskGetTickCount() - start_tick) < timeout_ticks) {
        if (wifi_manager_is_connected()) {
            connected = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Check every 100ms
    }

    // 4. Adjust mode based on connection result
    if (connected) {
        ESP_LOGI(TAG, "OK STA connected! IP: %s", wifi_manager_get_ip());
        ESP_LOGI(TAG, "Closing AP to save power...");
        
        err = wifi_manager_stop_ap_provisioning();
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "OK AP closed, entering pure STA mode");
        } else {
            ESP_LOGW(TAG, "Close AP failed (non-fatal): %s", esp_err_to_name(err));
        }
        
        ESP_LOGI(TAG, "========== Smart Dual Mode: STA Mode ==========");
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "FAIL STA connection timeout (%lu ms), keep AP provision mode", timeout_ms);
        
        // Disconnect STA attempt, keep pure AP mode
        esp_wifi_disconnect();
        
        // Switch to pure AP mode (optional, reduce interference)
        err = esp_wifi_set_mode(WIFI_MODE_AP);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Switched to pure AP mode");
        }
        
        ESP_LOGI(TAG, "========== Smart Dual Mode: AP Provision Mode ==========");
        ESP_LOGI(TAG, "Connect to AP: %s (192.168.4.1) to reconfig WiFi", ap_ssid);
        return ESP_OK;
    }
}

// AP配网模式静态变量
static bool s_ap_mode_active = false;

esp_err_t wifi_manager_start_ap_provisioning(const char* ap_ssid, const char* ap_password)
{
    if (!s_wifi_initialized) {
        ESP_LOGE(TAG, "WiFi管理器未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    if (!ap_ssid || strlen(ap_ssid) == 0) {
        ESP_LOGE(TAG, "AP SSID不能为空");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "启动AP配网模式，SSID: %s", ap_ssid);

    // 断开当前STA连接
    esp_wifi_disconnect();
    esp_wifi_stop();

    // 配置AP模式
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.ap.ssid, ap_ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = strlen(ap_ssid);
    wifi_config.ap.max_connection = 4;  // 最多4个客户端
    wifi_config.ap.channel = 1;

    if (ap_password && strlen(ap_password) >= 8) {
        strncpy((char*)wifi_config.ap.password, ap_password, sizeof(wifi_config.ap.password) - 1);
        wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    } else {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;  // 开放网络（无密码）
        ESP_LOGW(TAG, "AP使用开放网络（无密码），建议设置8位以上密码");
    }

    // 设置AP模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_ap_mode_active = true;

    ESP_LOGI(TAG, "AP配网模式已启动");
    ESP_LOGI(TAG, "请连接WiFi: %s", ap_ssid);
    ESP_LOGI(TAG, "然后访问 http://192.168.4.1 进行配置");

    return ESP_OK;
}

esp_err_t wifi_manager_stop_ap_provisioning(void)
{
    if (!s_ap_mode_active) {
        ESP_LOGW(TAG, "当前不在AP模式");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "停止AP配网模式");

    // 停止AP
    esp_wifi_stop();
    s_ap_mode_active = false;

    // 切换回STA模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_LOGI(TAG, "AP模式已停止，切换回STA模式");
    return ESP_OK;
}

bool wifi_manager_is_ap_mode(void)
{
    return s_ap_mode_active;
}
