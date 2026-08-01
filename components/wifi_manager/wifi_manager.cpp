/**
 * @file wifi_manager.cpp
 * @brief ESP32 WiFi 管理器实现
 * 
 * 【ESP-IDF WiFi 系统函数说明】
 * 
 * 1. esp_wifi_init() - 初始化 WiFi 驱动
 *    - 分配 WiFi 驱动内部资源
 *    - 必须在其他 WiFi 函数之前调用
 *    - 参数: wifi_init_config_t* cfg (通常使用 WIFI_INIT_CONFIG_DEFAULT())
 * 
 * 2. esp_wifi_set_mode() - 设置 WiFi 工作模式
 *    - WIFI_MODE_NULL: 禁用 WiFi
 *    - WIFI_MODE_STA: 仅 Station 模式（连接路由器）
 *    - WIFI_MODE_AP: 仅 AP 模式（作为热点）
 *    - WIFI_MODE_APSTA: 同时启用 STA + AP（配网模式）
 * 
 * 3. esp_wifi_set_config() - 设置 WiFi 配置参数
 *    - 接口: WIFI_IF_STA (Station) 或 WIFI_IF_AP (Access Point)
 *    - 参数: wifi_config_t* 包含 SSID/密码/加密方式等
 * 
 * 4. esp_wifi_connect() - 启动 WiFi 连接
 *    - 触发 STA 尝试连接已配置的 AP
 *    - 异步执行，连接结果通过事件回调通知
 *    - 必须在 STA 配置设置完成后调用
 * 
 * 5. esp_wifi_start() / esp_wifi_stop() - 启动/停止 WiFi 硬件
 *    - 根据当前模式启动 STA 和/或 AP 硬件
 *    - 启动后会触发 WIFI_EVENT_STA_START 事件
 * 
 * 6. esp_wifi_disconnect() - 断开当前连接
 *    - 停止 STA 连接并保持停止状态（不自动重连）
 *    - 触发 WIFI_EVENT_STA_DISCONNECTED 事件
 * 
 * 【事件系统说明】
 * ESP-IDF 使用事件循环机制通知 WiFi 状态变化：
 * - WIFI_EVENT_STA_START: WiFi 启动完成，可以开始连接
 * - WIFI_EVENT_STA_CONNECTED: 成功连接到 AP
 * - WIFI_EVENT_STA_DISCONNECTED: 与 AP 断开连接
 * - IP_EVENT_STA_GOT_IP: 成功获取 IP 地址
 * - WIFI_EVENT_AP_START: AP 模式启动
 * - WIFI_EVENT_AP_STACONNECTED: 有客户端连接到 AP
 * 
 * 事件处理通过 esp_event_handler_instance_register() 注册回调函数
 */

#include "wifi_manager.h"
#include <stdio.h>
#include <cstdio>
#include <string.h>
#include <cstring>
#include <assert.h>
#include "esp_wifi.h"        /* ESP-IDF WiFi 核心 API */
#include "esp_mac.h"         /* MAC 地址相关功能 */
#include "esp_log.h"         /* ESP-IDF 日志系统 */
#include "esp_netif.h"       /* 网络接口抽象层 */
#include "esp_event.h"       /* ESP-IDF 事件循环 */
#include "web_server.h"
#include "wifi_config_store.h"  /* WiFi配置存储组件 */

static const char *TAG = "wifi_manager";

// 静态变量
static bool s_wifi_initialized = false;
static bool s_wifi_connected = false;
static bool s_ap_mode_active = false;
static char s_ip_address[16] = "0.0.0.0";
static esp_event_handler_instance_t instance_any_id = NULL;
static esp_event_handler_instance_t instance_got_ip = NULL;

// 状态管理（新）
static wifi_status_t s_current_status = WIFI_STATUS_DISCONNECTED;

// 回调数组（新）- 最多3个订阅者
#define MAX_STATUS_CALLBACKS 3
static struct {
    wifi_status_cb_t cb;
    void* arg;
    bool active;
} s_status_callbacks[MAX_STATUS_CALLBACKS];

// 前向声明（新）
static void notify_status_change(wifi_status_t status);

// 状态通知函数（新）- 自动获取对应模式的 IP
static void notify_status_change(wifi_status_t status)
{
    s_current_status = status;

    // 根据状态自动获取对应接口的 IP
    const char* ifkey = (status == WIFI_STATUS_AP_MODE) ? "WIFI_AP_DEF" : "WIFI_STA_DEF";
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey(ifkey);

    if (netif && (status == WIFI_STATUS_CONNECTED || status == WIFI_STATUS_AP_MODE)) {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            snprintf(s_ip_address, sizeof(s_ip_address), IPSTR, IP2STR(&ip_info.ip));
        }
    } else if (status == WIFI_STATUS_DISCONNECTED) {
        s_ip_address[0] = '\0';
    }

    ESP_LOGD(TAG, "WiFi 状态变化: %d, IP: %s", status, s_ip_address);

    // 通知所有订阅者
    for (int i = 0; i < MAX_STATUS_CALLBACKS; i++) {
        if (s_status_callbacks[i].active && s_status_callbacks[i].cb) {
            s_status_callbacks[i].cb(status, s_ip_address, s_status_callbacks[i].arg);
        }
    }
}

/**
 * @brief WiFi 事件处理回调函数
 * @param arg 用户参数（未使用）
 * @param event_base 事件基类（WIFI_EVENT / IP_EVENT）
 * @param event_id 具体事件 ID
 * @param event_data 事件数据（含断开原因等）
 * 
 * 【事件处理说明】
 * - WIFI_EVENT_STA_START: WiFi 硬件启动完成，自动调用 esp_wifi_connect() 开始连接
 * - WIFI_EVENT_STA_DISCONNECTED: 连接断开，根据原因码判断问题类型
 * - esp_wifi_connect() 会在断开时自动重连（代码第 124 行）
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        /* 
         * 当 WiFi 硬件启动完成后，自动开始连接已配置的 AP
         * 此事件由 esp_wifi_start() 触发
         * esp_wifi_connect() 使用之前通过 esp_wifi_set_config() 设置的配置
         */
        esp_wifi_connect();
        notify_status_change(WIFI_STATUS_CONNECTING);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        uint8_t reason = event->reason;
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
        
        // 通知状态变化（新）
        notify_status_change(WIFI_STATUS_DISCONNECTED);
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        snprintf(s_ip_address, sizeof(s_ip_address), IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_connected = true;
        ESP_LOGI(TAG, "WiFi连接成功，IP地址: %s", s_ip_address);
        
        // 通知状态变化（新）
        notify_status_change(WIFI_STATUS_CONNECTED);
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        /*
         * AP模式启动完成，由事件驱动状态更新
         * 此事件由 esp_wifi_start() 在AP模式触发
         */
        s_ap_mode_active = true;
        ESP_LOGI(TAG, "AP模式已启动（事件驱动）");
        notify_status_change(WIFI_STATUS_AP_MODE);
        
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STOP) {
        /*
         * AP模式已停止，由事件驱动状态更新
         * 状态通知由 stop_ap_provisioning() 根据STA连接状态决定
         */
        s_ap_mode_active = false;
        ESP_LOGI(TAG, "AP模式已停止（事件驱动）");
        // 注意：状态变化通知由调用方（stop_ap_provisioning）根据上下文决定
    }
}

esp_err_t wifi_manager_init(void)
{
    if (s_wifi_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // 注：NVS初始化由 nvs_manager 在 main.cpp 中完成
    // 此处不再重复调用 nvs_flash_init()

    // 初始化网络接口
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);
    
    // 创建AP网络接口（用于配网模式）
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    assert(ap_netif);

    // 初始化WiFi
    /* 
     * WIFI_INIT_CONFIG_DEFAULT() 提供默认初始化配置
     * 包括: nvs_enable=1(从NVS恢复配置), 任务优先级, 堆栈大小等
     * esp_wifi_init() 必须在所有其他 WiFi 函数之前调用
     */
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

    // WiFi 配置将在连接时设置（通过 wifi_manager_update_credentials 设置）
    /* 
     * 设置 WiFi 为 APSTA 模式（Smart Dual Mode）
     * 这会同时启用 AP 配网和 STA 连接尝试
     * ⚠️ 注意：必须在事件注册之后设置模式，否则事件可能丢失
     */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    
    // 禁用WiFi电源管理，防止在接收配置时WiFi状态变化导致崩溃
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_LOGI(TAG, "WiFi电源管理已禁用");

    s_wifi_initialized = true;
    ESP_LOGI(TAG, "WiFi初始化完成");

    return ESP_OK;
}

esp_err_t wifi_manager_start(void)
{
    // 1. 从 NVS 加载配置（使用 wifi_config_store 组件）
    char nvs_ssid[33] = {0};
    char nvs_pass[65] = {0};
    bool has_nvs_config = false;

    esp_err_t err = wifi_config_store_load(nvs_ssid, sizeof(nvs_ssid), nvs_pass, sizeof(nvs_pass));
    if (err == ESP_OK) {
        has_nvs_config = true;
        ESP_LOGI(TAG, "WiFi config loaded from NVS: SSID=%s", nvs_ssid);
    } else {
        ESP_LOGI(TAG, "No WiFi config in NVS, will use default");
    }

    // 2. 准备配置
    const char* target_ssid = has_nvs_config ? nvs_ssid : CONFIG_WIFI_DEFAULT_SSID;
    const char* target_pass = has_nvs_config ? nvs_pass : CONFIG_WIFI_DEFAULT_PASSWORD;

    // 使用 Kconfig 默认值作为 AP 配置
    const char* ap_ssid = CONFIG_WIFI_AP_SSID;
    const char* ap_pass = CONFIG_WIFI_AP_PASSWORD;
    const uint32_t timeout_ms = 30000;  // 30秒超时

    ESP_LOGI(TAG, "========== Smart Dual Mode Start ==========");
    ESP_LOGI(TAG, "AP=%s, STA target=%s (source=%s)",
             ap_ssid, target_ssid, has_nvs_config ? "NVS" : "default");

    // 2. 设置 STA 配置（注：WiFi初始化由调用方通过 wifi_manager_init() 完成）
    ESP_LOGI(TAG, "[DEBUG] Step 1: update_credentials('%s')", target_ssid);
    err = wifi_manager_update_credentials(target_ssid, target_pass);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi start: STA config failed: %s", esp_err_to_name(err));
        wifi_manager_deinit();
        return err;
    }
    ESP_LOGI(TAG, "[DEBUG] Step 2: STA config set OK");

    // 3. 启动 AP 配网模式
    err = wifi_manager_start_ap_provisioning(ap_ssid, ap_pass);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WiFi start: AP provision failed: %s", esp_err_to_name(err));
        wifi_manager_deinit();
        return err;
    }

    ESP_LOGI(TAG, "APSTA dual mode started, AP: %s", ap_ssid);

    // 4. 等待 STA 连接或超时
    ESP_LOGI(TAG, "Waiting for STA connection result...");
    
    TickType_t start_tick = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
    bool connected = false;

    while ((xTaskGetTickCount() - start_tick) < timeout_ticks) {
        if (wifi_manager_is_connected()) {
            connected = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));  // Check every 100ms
    }

    // 5. 根据结果调整模式
    if (connected) {
        ESP_LOGI(TAG, "OK STA connected! IP: %s", wifi_manager_get_ip());
        ESP_LOGI(TAG, "Closing AP to save power...");
        
        err = wifi_manager_stop_ap_provisioning();
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "OK AP closed, entering pure STA mode");
        } else {
            ESP_LOGW(TAG, "Close AP failed (non-fatal): %s", esp_err_to_name(err));
        }
        
        ESP_LOGI(TAG, "WiFi STA connected: IP=%s", wifi_manager_get_ip());
        ESP_LOGI(TAG, "========== Smart Dual Mode: STA Mode ==========");
    } else {
        ESP_LOGW(TAG, "FAIL STA connection timeout (%lu ms), keep AP provision mode", timeout_ms);
        
        // Disconnect STA attempt, keep pure AP mode
        esp_wifi_disconnect();
        
        // Switch to pure AP mode
        err = esp_wifi_set_mode(WIFI_MODE_AP);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Switched to pure AP mode");
            notify_status_change(WIFI_STATUS_AP_MODE);
        }

        ESP_LOGI(TAG, "WiFi AP mode active (provisioning)");
        ESP_LOGI(TAG, "========== Smart Dual Mode: AP Provision Mode ==========");
        ESP_LOGI(TAG, "Connect to AP: %s (192.168.4.1) to reconfig WiFi", ap_ssid);
    }

    return ESP_OK;
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
        // 通知状态变化（新机制）
        notify_status_change(WIFI_STATUS_DISCONNECTED);
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

bool wifi_manager_is_ap_mode(void)
{
    return s_ap_mode_active;
}

esp_err_t wifi_manager_update_credentials(const char* ssid, const char* password)
{
    // SSID必须非空，密码可为空字符串（开放网络）
    if (!s_wifi_initialized || !ssid || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "更新凭证失败: SSID不能为空");
        return ESP_ERR_INVALID_ARG;
    }

    // 配置新的WiFi凭证（只保存到驱动，不触发连接）
    /* 
     * wifi_config_t 结构体定义 STA 连接参数
     * .sta.ssid: 目标 WiFi 名称（必须，最多32字节）
     * .sta.password: WiFi 密码（开放网络可留空）
     * .sta.threshold.authmode: 最低安全等级要求
     * .sta.sae_pwe_h2e: WPA3 SAE 配置
     */
    wifi_config_t wifi_config = {};
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    // 密码可为空字符串表示开放网络
    if (password && strlen(password) > 0) {
        strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;  // 要求 WPA2 安全
        wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;           // 支持 WPA3
    } else {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_OPEN;        // 开放网络
        ESP_LOGI(TAG, "配置为开放网络（无密码）");
    }

    /* 
     * esp_wifi_set_config() 将配置写入 WiFi 驱动
     * 参数1: WIFI_IF_STA (Station 接口) 或 WIFI_IF_AP (AP 接口)
     * 参数2: wifi_config_t 结构体
     * 
     * 注意：此函数不触发连接，仅保存配置。
     * 连接需要通过 esp_wifi_connect() 或等待事件触发。
     */
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

    ESP_LOGI(TAG, "WiFi已反初始化");
    return ESP_OK;
}

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
    /* 
     * WIFI_MODE_APSTA: 同时启用 Station 和 Access Point
     * - STA 部分：尝试连接已配置的路由器
     * - AP 部分：创建热点供其他设备连接
     * 
     * 配网模式使用此配置，设备既作为客户端又作为服务端
     */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));  // 设置 AP 参数
    
    // 确保STA配置仍然存在（通过 wifi_manager_update_credentials 设置）
    // esp_wifi_start() 会触发 WIFI_EVENT_STA_START 事件
    // 进而自动调用 esp_wifi_connect() 开始 STA 连接
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi APSTA dual mode started");

    // 注：s_ap_mode_active 由 WIFI_EVENT_AP_START 事件处理设置
    // 状态变更由事件驱动，此处不再直接赋值

    ESP_LOGI(TAG, "AP配网模式已启动");
    ESP_LOGI(TAG, "请连接WiFi: %s", ap_ssid);
    ESP_LOGI(TAG, "然后访问 http://192.168.4.1 进行配置");
    
    // 注：状态变化通知由 WIFI_EVENT_AP_START 事件处理触发

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
    
    // 注：s_ap_mode_active 由 WIFI_EVENT_AP_STOP 事件处理设置
    // 状态变更由事件驱动，此处不再直接赋值

    ESP_LOGI(TAG, "AP配网模式已停止");
    
    // AP 停止后，如果 STA 已连接则保持 CONNECTED，否则通知 DISCONNECTED
    // 注意：此通知在事件处理前执行，确保状态及时更新
    if (wifi_manager_is_connected()) {
        notify_status_change(WIFI_STATUS_CONNECTED);
    } else {
        notify_status_change(WIFI_STATUS_DISCONNECTED);
    }
    
    return ESP_OK;
}

// ============================================================================
// 状态管理 API 实现（新）
// ============================================================================

int wifi_manager_register_status_callback(wifi_status_cb_t cb, void* user_arg)
{
    if (!cb) {
        ESP_LOGE(TAG, "回调函数不能为空");
        return -1;
    }
    
    for (int i = 0; i < MAX_STATUS_CALLBACKS; i++) {
        if (!s_status_callbacks[i].active) {
            s_status_callbacks[i].cb = cb;
            s_status_callbacks[i].arg = user_arg;
            s_status_callbacks[i].active = true;
            ESP_LOGD(TAG, "注册状态回调 ID=%d", i + 1);
            return i + 1;  // 返回 1-based ID
        }
    }
    
    ESP_LOGW(TAG, "回调数组已满（最大 %d 个）", MAX_STATUS_CALLBACKS);
    return -1;
}

void wifi_manager_unregister_status_callback(int cb_id)
{
    if (cb_id < 1 || cb_id > MAX_STATUS_CALLBACKS) {
        ESP_LOGW(TAG, "无效的回调 ID: %d", cb_id);
        return;
    }
    
    int idx = cb_id - 1;  // 转换为 0-based
    if (s_status_callbacks[idx].active) {
        s_status_callbacks[idx].active = false;
        s_status_callbacks[idx].cb = NULL;
        s_status_callbacks[idx].arg = NULL;
        ESP_LOGD(TAG, "注销状态回调 ID=%d", cb_id);
    }
}

wifi_status_t wifi_manager_get_status(void)
{
    return s_current_status;
}

const char* wifi_manager_get_ip_address(void)
{
    if (s_current_status == WIFI_STATUS_CONNECTED && s_ip_address[0]) {
        return s_ip_address;
    }
    return NULL;
}


esp_err_t net_init(void)
{
    ESP_LOGI(TAG, "初始化网络服务...");

    // 注册 Web Server 回调，在 WiFi 就绪时自动启动
    web_server_init();

    // 分离设计：先初始化 WiFi，再启动连接
    // 初始化 WiFi 硬件和事件处理
    if (wifi_manager_init() != ESP_OK) {
        ESP_LOGE(TAG, "WiFi初始化失败");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "WiFi硬件初始化完成");

    // 启动 WiFi 连接（智能双模式）
    // 启动结果可通过 wifi_manager_is_connected(), wifi_manager_is_ap_mode() 等查询
    if (wifi_manager_start() != ESP_OK) {
        ESP_LOGE(TAG, "WiFi启动失败");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "WiFi连接启动完成");

    ESP_LOGI(TAG, "网络服务初始化完成");
    return ESP_OK;
}
