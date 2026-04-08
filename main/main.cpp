#include <stdio.h>
#include <string.h>
#include <stdio.h>
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

// 声明测试函数
extern "C" void test_aw9523_i2c_communication(void);

extern "C" {
    // Forward declaration
    esp_err_t gpio_monitor_update_step(const char *step);
    esp_err_t gpio_monitor_update_ip(const char *ip_addr);
}

static const char *TAG = "main";

// WiFi configuration
// static const char* WIFI_SSID = "XINGHUI";
// static const char* WIFI_PASS = "88888888";

static const char* WIFI_SSID = "HUAWEI-GTJ-home";
static const char* WIFI_PASS = "Gt-123456";

// ADC configuration
// ADC configuration handle will be declared when needed to avoid unused warnings

// HTTP server instance
static httpd_handle_t server = NULL;

// WiFi connection status callback
static void wifi_connection_handler(bool connected, const char* ip_address)
{
    if (connected) {
        ESP_LOGI(TAG, "WiFi connected, IP address: %s", ip_address);
        gpio_monitor_update_status("wifi", true);
        gpio_monitor_update_ip(ip_address);
        
        // Start web server
        if (!server) {
            server = web_server_start();
            if (server) {
                ESP_LOGI(TAG, "Web server started successfully");
                gpio_monitor_update_status("webserver", true);
            } else {
                ESP_LOGE(TAG, "Failed to start web server");
                gpio_monitor_update_status("webserver", false);
            }
        }
    } else {
        ESP_LOGI(TAG, "WiFi disconnected");
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

    // 初始化WiFi
    wifi_manager_config_t wifi_config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
        .callback = wifi_connection_handler
    };
    if (wifi_manager_init(&wifi_config) != ESP_OK) {
        ESP_LOGE(TAG, "WiFi管理器初始化失败");
        gpio_monitor_update_status("wifi", false);
    } else {
        // 启动WiFi连接
        if (wifi_manager_start() != ESP_OK) {
            ESP_LOGE(TAG, "WiFi启动失败");
            gpio_monitor_update_status("wifi", false);
        }
    }
    

}