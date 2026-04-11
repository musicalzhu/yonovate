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
        ESP_LOGI(TAG, "状态机启动成功");
        gpio_monitor_update_step("状态机已启动");
    }

    // 初始化WiFi配置存储
    wifi_config_store_init();

    // 尝试从NVS加载WiFi配置
    char saved_ssid[33] = {0};
    char saved_pass[65] = {0};
    bool has_wifi_config = false;

    if (wifi_config_store_load(saved_ssid, sizeof(saved_ssid), saved_pass, sizeof(saved_pass)) == ESP_OK) {
        ESP_LOGI(TAG, "从NVS加载WiFi配置: SSID=%s", saved_ssid);
        has_wifi_config = true;
    } else {
        ESP_LOGI(TAG, "NVS中无WiFi配置，将启动AP配网模式");
        has_wifi_config = false;
    }

    // 初始化WiFi管理器（使用默认配置或空配置）
    wifi_manager_config_t wifi_config = {
        .ssid = has_wifi_config ? saved_ssid : WIFI_DEFAULT_SSID,
        .password = has_wifi_config ? saved_pass : WIFI_DEFAULT_PASS,
        .callback = wifi_connection_handler
    };
    if (wifi_manager_init(&wifi_config) != ESP_OK) {
        ESP_LOGE(TAG, "WiFi管理器初始化失败");
        gpio_monitor_update_status("wifi", false);
    } else {
        if (has_wifi_config) {
            // 有配置：启动STA模式连接
            if (wifi_manager_start() != ESP_OK) {
                ESP_LOGE(TAG, "WiFi启动失败");
                gpio_monitor_update_status("wifi", false);
            }
        } else {
            // 无配置：启动AP配网模式
            ESP_LOGI(TAG, "启动AP配网模式，等待用户配置...");
            if (wifi_manager_start_ap_provisioning("Yonovate-Setup", "12345678") == ESP_OK) {
                ESP_LOGI(TAG, "AP配网模式已启动，请连接WiFi: Yonovate-Setup，密码: 12345678");
                ESP_LOGI(TAG, "然后访问 http://192.168.4.1/api/wifi/config 进行配置");
                // AP模式下也需要启动Web服务器，否则用户无法访问配置API
                if (!server) {
                    server = web_server_start();
                    if (server) {
                        ESP_LOGI(TAG, "Web服务器启动成功（AP模式）");
                        gpio_monitor_update_status("webserver", true);
                    } else {
                        ESP_LOGE(TAG, "Web服务器启动失败");
                        gpio_monitor_update_status("webserver", false);
                    }
                }
            } else {
                ESP_LOGE(TAG, "AP配网模式启动失败");
                gpio_monitor_update_status("wifi", false);
            }
        }
    }
    

}