#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

#include "wifi_manager.h"
#include "web_server.h"
#include "i2c_manager.h"
#include "gpio_manager.h"
#include "gpio_monitor.h"
#include "logic_circuit.h"
#include "state_machine.h"
#include "oled.h"
#include "wifi_config_store.h"

// 声明测试函数
extern "C" void test_aw9523_i2c_communication(void);

extern "C" {
    // Forward declaration
    esp_err_t gpio_monitor_update_step(const char *step);
    esp_err_t gpio_monitor_update_ip(const char *ip_addr);
}

static const char *TAG = "main";

// WiFi配置使用Kconfig默认值（可通过menuconfig修改）
// CONFIG_WIFI_DEFAULT_SSID 和 CONFIG_WIFI_DEFAULT_PASSWORD 定义在 main/Kconfig.projbuild
#define WIFI_DEFAULT_SSID CONFIG_WIFI_DEFAULT_SSID
#define WIFI_DEFAULT_PASS CONFIG_WIFI_DEFAULT_PASSWORD

// ADC configuration
// ADC configuration handle will be declared when needed to avoid unused warnings

// HTTP server instance
static httpd_handle_t server = NULL;

// WiFi connection status callback
static void wifi_connection_handler(bool connected, const char* ip_address)
{
    if (connected) {
        ESP_LOGI(TAG, "WiFi已连接, IP地址: %s", ip_address);
        gpio_monitor_update_status("wifi", true);
        gpio_monitor_update_ip(ip_address);
        
        // Start web server
        if (!server) {
            server = web_server_start();
            if (server) {
                ESP_LOGI(TAG, "Web服务器启动成功");
                gpio_monitor_update_status("webserver", true);
            } else {
                ESP_LOGE(TAG, "Web服务器启动失败");
                gpio_monitor_update_status("webserver", false);
            }
        }
    } else {
        ESP_LOGI(TAG, "WiFi已断开连接");
        gpio_monitor_update_status("wifi", false);
        gpio_monitor_update_status("webserver", false);
        
        // Stop web server
        if (server) {
            web_server_stop(server);
            server = NULL;
        }
    }
}

extern "C" void app_main()
{
    ESP_LOGI(TAG, "开始系统初始化...");
    
    // 更新步骤信息
    // gpio_monitor_update_step("系统启动初始化");

    // 初始化I2C总线
    if (i2c_manager_init() != ESP_OK) {
        ESP_LOGE(TAG, "I2C总线初始化失败");
        gpio_monitor_update_status("i2c", false);
        return;
    }
    // gpio_monitor_update_status("i2c", true);
    // gpio_monitor_update_step("I2C初始化完成");

    // 初始化OLED
    if (oled_init() != ESP_OK) {
        ESP_LOGE(TAG, "OLED初始化失败");
        gpio_monitor_update_status("oled", false);
        return;
    }
    gpio_monitor_update_status("oled", true);
    gpio_monitor_update_step("OLED初始化完成");

    // 初始化GPIO管理器
    if (gpio_manager_init() != ESP_OK) {
        ESP_LOGE(TAG, "GPIO管理器初始化失败");
        gpio_monitor_update_status("gpio", false);
        return;
    }
    gpio_monitor_update_status("gpio", true);
    gpio_monitor_update_step("GPIO管理器初始化完成");


    // 启动GPIO监控任务
    if (gpio_monitor_start() != ESP_OK) {
        ESP_LOGE(TAG, "GPIO监控任务启动失败");
        return;
    }
    gpio_monitor_update_step("GPIO监控已启动");

    // 初始化电机为停止状态
    gpio_manager_set_motor(0, 0);
    gpio_manager_set_motor(1, 0);
    gpio_monitor_update_step("电机已初始化为停止");

    // AW9523测试任务已移除
    // ESP_LOGI(TAG, "AW9523测试任务已启动");

    // 初始化状态机
    if (state_machine_init() != ESP_OK) {
        ESP_LOGE(TAG, "状态机初始化失败");
        gpio_monitor_update_status("state_machine", false);
        return;
    }
    gpio_monitor_update_status("state_machine", true);
    gpio_monitor_update_step("状态机初始化完成");

    // 启动状态机，设置初始状态为"idle"
    if (state_machine_start("idle") != ESP_OK) {
        ESP_LOGE(TAG, "状态机启动失败");
        gpio_monitor_update_status("state_machine", false);
    } else {
        ESP_LOGI(TAG, "State machine started successfully");
        gpio_monitor_update_step("State machine started");
    }

    // ========== WiFi Smart Dual Mode Startup ==========
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "WiFi Smart Dual Mode Initializing");
    ESP_LOGI(TAG, "========================================");
    
    // Initialize WiFi config storage
    ESP_LOGI(TAG, "[1/7] Initializing WiFi config storage (NVS)...");
    wifi_config_store_init();
    ESP_LOGI(TAG, "[1/7] OK NVS init done");

    // Try to load WiFi config from NVS
    ESP_LOGI(TAG, "[2/7] Loading WiFi config from NVS...");
    char saved_ssid[33] = {0};
    char saved_pass[65] = {0};
    bool has_wifi_config = false;

    if (wifi_config_store_load(saved_ssid, sizeof(saved_ssid), saved_pass, sizeof(saved_pass)) == ESP_OK) {
        ESP_LOGI(TAG, "[2/7] OK Loaded WiFi config from NVS");
        ESP_LOGI(TAG, "       SSID: %s", saved_ssid);
        ESP_LOGI(TAG, "       Pass len: %zu", strlen(saved_pass));
        has_wifi_config = true;
    } else {
        ESP_LOGI(TAG, "[2/7] WARN No WiFi config in NVS, using default");
        has_wifi_config = false;
    }

    // Prepare WiFi config
    const char* target_ssid = has_wifi_config ? saved_ssid : WIFI_DEFAULT_SSID;
    const char* target_pass = has_wifi_config ? saved_pass : WIFI_DEFAULT_PASS;
    
    ESP_LOGI(TAG, "[3/7] WiFi config params:");
    ESP_LOGI(TAG, "       Target SSID: %s", target_ssid);
    ESP_LOGI(TAG, "       Config src: %s", has_wifi_config ? "NVS saved" : "factory default");
    ESP_LOGI(TAG, "       AP SSID: Yonovate-Setup");
    ESP_LOGI(TAG, "       AP pass: 12345678");
    ESP_LOGI(TAG, "       AP IP: 192.168.4.1");
    
    wifi_manager_config_t wifi_config = {
        .ssid = target_ssid,
        .password = target_pass,
        .callback = wifi_connection_handler
    };
    
    ESP_LOGI(TAG, "[4/7] Starting smart dual mode WiFi...");
    ESP_LOGI(TAG, "       Flow: Start APSTA → Wait STA 30s → Close AP if OK, keep AP if fail");
    
    // Smart dual mode: start AP+STA, close AP if STA connects within 30s, keep AP if fails
    esp_err_t ret = wifi_manager_start_smart_dual_mode(&wifi_config, 
                                                        "Yonovate-Setup", 
                                                        "12345678",
                                                        30000);
    
    ESP_LOGI(TAG, "[5/7] Smart dual mode result: %s", esp_err_to_name(ret));
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "[6/7] Checking WiFi connection status...");
        
        // Check current mode
        if (wifi_manager_is_connected()) {
            // STA connected, now in pure STA mode
            ESP_LOGI(TAG, "[6/7] OK STA connected!");
            ESP_LOGI(TAG, "[7/7] OK Current mode: STA mode (connected to router)");
            gpio_monitor_update_status("wifi", true);
            gpio_monitor_update_status("ap", false);
        } else {
            // STA connection failed, stay in AP provisioning mode
            ESP_LOGW(TAG, "[6/7] FAIL STA connection timeout/failed");
            ESP_LOGI(TAG, "[7/7] WARN Current mode: AP provisioning (waiting for config)");
            ESP_LOGI(TAG, "       Connect to WiFi: Yonovate-Setup");
            ESP_LOGI(TAG, "       Visit http://192.168.4.1 to config");
            gpio_monitor_update_status("wifi", false);
            gpio_monitor_update_status("ap", true);
            
            // Start web server in AP mode
            ESP_LOGI(TAG, "       Starting web server (AP mode)...");
            if (!server) {
                server = web_server_start();
                if (server) {
                    ESP_LOGI(TAG, "       OK Web server started (AP mode)");
                    gpio_monitor_update_status("webserver", true);
                } else {
                    ESP_LOGE(TAG, "       FAIL Web server start failed");
                }
            }
        }
    } else {
        ESP_LOGE(TAG, "[6/7] FAIL WiFi smart dual mode start failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "[7/7] FAIL WiFi function unavailable");
        gpio_monitor_update_status("wifi", false);
    }
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "WiFi init flow complete");
    ESP_LOGI(TAG, "========================================");
}