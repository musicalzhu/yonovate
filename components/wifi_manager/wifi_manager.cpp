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
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        uint8_t reason = event->reason;
        bool was_connected = s_wifi_connected;
        s_wifi_connected = false;
        
        // 记录断开原因码，便于诊断连接问题
        const char* reason_str = "UNKNOWN";
        switch (reason) {
            case WIFI_REASON_AUTH_EXPIRE:       reason_str = "AUTH_EXPIRE"; break;
            case WIFI_REASON_AUTH_LEAVE:        reason_str = "AUTH_LEAVE"; break;
            case WIFI_REASON_ASSOC_EXPIRE:      reason_str = "ASSOC_EXPIRE"; break;
            case WIFI_REASON_ASSOC_TOOMANY:     reason_str = "ASSOC_TOOMANY"; break;
            case WIFI_REASON_NOT_AUTHED:        reason_str = "NOT_AUTHED (WRONG PASSWORD)"; break;
            case WIFI_REASON_NOT_ASSOCED:       reason_str = "NOT_ASSOCED"; break;
            case WIFI_REASON_ASSOC_LEAVE:     reason_str = "ASSOC_LEAVE"; break;
            case WIFI_REASON_ASSOC_NOT_AUTHED:  reason_str = "ASSOC_NOT_AUTHED"; break;
            case WIFI_REASON_DISASSOC_PWRCAP_BAD: reason_str = "DISASSOC_PWRCAP_BAD"; break;
            case WIFI_REASON_DISASSOC_SUPCHAN_BAD: reason_str = "DISASSOC_SUPCHAN_BAD"; break;
            case WIFI_REASON_IE_INVALID:        reason_str = "IE_INVALID"; break;
            case WIFI_REASON_MIC_FAILURE:       reason_str = "MIC_FAILURE"; break;
            case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT: reason_str = "4WAY_HANDSHAKE_TIMEOUT (AUTH FAIL)"; break;
            case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT: reason_str = "GROUP_KEY_UPDATE_TIMEOUT"; break;
            case WIFI_REASON_IE_IN_4WAY_DIFFERS: reason_str = "IE_IN_4WAY_DIFFERS"; break;
            case WIFI_REASON_GROUP_CIPHER_INVALID: reason_str = "GROUP_CIPHER_INVALID"; break;
            case WIFI_REASON_PAIRWISE_CIPHER_INVALID: reason_str = "PAIRWISE_CIPHER_INVALID"; break;
            case WIFI_REASON_AKMP_INVALID:      reason_str = "AKMP_INVALID"; break;
            case WIFI_REASON_UNSUPP_RSN_IE_VERSION: reason_str = "UNSUPP_RSN_IE_VERSION"; break;
            case WIFI_REASON_INVALID_RSN_IE_CAP: reason_str = "INVALID_RSN_IE_CAP"; break;
            case WIFI_REASON_802_1X_AUTH_FAILED: reason_str = "802_1X_AUTH_FAILED"; break;
            case WIFI_REASON_CIPHER_SUITE_REJECTED: reason_str = "CIPHER_SUITE_REJECTED"; break;
            case WIFI_REASON_NO_AP_FOUND:       reason_str = "NO_AP_FOUND (SSID NOT FOUND)"; break;
            case WIFI_REASON_BEACON_TIMEOUT:    reason_str = "BEACON_TIMEOUT"; break;
            case WIFI_REASON_AUTH_FAIL:         reason_str = "AUTH_FAIL (PASSWORD WRONG)"; break;
            case WIFI_REASON_ASSOC_FAIL:        reason_str = "ASSOC_FAIL"; break;
            case WIFI_REASON_HANDSHAKE_TIMEOUT: reason_str = "HANDSHAKE_TIMEOUT"; break;
            case WIFI_REASON_CONNECTION_FAIL:   reason_str = "CONNECTION_FAIL"; break;
            default:                            reason_str = "OTHER"; break;
        }
        
        ESP_LOGW(TAG, "WiFi disconnected, reason: %d (%s)", reason, reason_str);
        
        // 特殊提示常见错误
        if (reason == WIFI_REASON_NOT_AUTHED || reason == WIFI_REASON_AUTH_FAIL) {
            ESP_LOGW(TAG, "==> PASSWORD ERROR: Check WiFi password correctness!");
        } else if (reason == WIFI_REASON_NO_AP_FOUND) {
            ESP_LOGW(TAG, "==> AP NOT FOUND: Check SSID and signal strength!");
        } else if (reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT) {
            ESP_LOGW(TAG, "==> AUTH TIMEOUT: Check router auth mode (WPA2)");
        }
        
        esp_wifi_connect();
        
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

    // 配置新的WiFi凭证（只保存到驱动，不触发连接）
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

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "设置WiFi配置失败: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "WiFi凭证已更新（保存到驱动），需重启后生效");
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

    // 设置APSTA双模式（允许STA同时尝试连接路由器）
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    
    // 确保STA配置仍然存在（在init中已设置）
    // 启动WiFi后STA会自动开始连接
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi APSTA dual mode started");

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

    // 如果STA已连接，保持STA，只关闭AP
    if (wifi_manager_is_connected()) {
        ESP_LOGI(TAG, "STA已连接，保持STA模式，关闭AP");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    } else {
        ESP_LOGW(TAG, "STA未连接，保持APSTA模式继续尝试连接");
        // 不切换模式，保持APSTA让STA继续尝试连接
    }
    
    s_ap_mode_active = false;

    ESP_LOGI(TAG, "AP配网模式已停止");
    return ESP_OK;
}

bool wifi_manager_is_ap_mode(void)
{
    return s_ap_mode_active;
}

// 延迟关闭AP任务参数结构体
typedef struct {
    uint32_t delay_ms;
} delayed_ap_shutdown_params_t;

// 延迟关闭AP的后台任务
static void delayed_ap_shutdown_task(void *pvParameters)
{
    delayed_ap_shutdown_params_t *params = (delayed_ap_shutdown_params_t *)pvParameters;
    uint32_t delay_ms = params->delay_ms;
    
    ESP_LOGI(TAG, "Delayed AP shutdown: waiting %lu ms for STA connection...", delay_ms);
    
    // 延迟等待
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    
    // 检查是否已连接
    if (wifi_manager_is_connected()) {
        ESP_LOGI(TAG, "STA connected, shutting down AP to save power");
        esp_err_t err = wifi_manager_stop_ap_provisioning();
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "AP shutdown successful, now in pure STA mode");
        } else {
            ESP_LOGW(TAG, "AP shutdown failed: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGW(TAG, "STA not connected after %lu ms, keeping AP mode for reconfiguration", delay_ms);
    }
    
    // 释放参数内存
    free(params);
    
    // 删除任务
    vTaskDelete(NULL);
}

esp_err_t wifi_manager_start_delayed_ap_shutdown(uint32_t delay_ms)
{
    if (!s_wifi_initialized) {
        ESP_LOGE(TAG, "WiFi管理器未初始化");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (delay_ms == 0) {
        ESP_LOGE(TAG, "延迟时间不能为0");
        return ESP_ERR_INVALID_ARG;
    }
    
    // 分配参数内存
    delayed_ap_shutdown_params_t *params = (delayed_ap_shutdown_params_t *)malloc(sizeof(delayed_ap_shutdown_params_t));
    if (params == NULL) {
        ESP_LOGE(TAG, "分配延迟关闭AP任务参数内存失败");
        return ESP_ERR_NO_MEM;
    }
    
    params->delay_ms = delay_ms;
    
    // 创建后台任务
    BaseType_t result = xTaskCreate(
        delayed_ap_shutdown_task,
        "ap_shutdown",
        2048,  // 堆栈大小
        params,
        5,     // 优先级（较低）
        NULL
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "创建延迟关闭AP任务失败");
        free(params);
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Delayed AP shutdown scheduled in %lu ms", delay_ms);
    return ESP_OK;
}
