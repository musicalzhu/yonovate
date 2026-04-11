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
    if (!config || !config->ssid || !config->password) {
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
    strncpy((char*)wifi_config.sta.password, config->password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

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
    if (!s_wifi_initialized || !ssid || !password) {
        return ESP_ERR_INVALID_ARG;
    }

    // 断开当前连接
    esp_wifi_disconnect();

    // 配置新的WiFi凭证
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
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
