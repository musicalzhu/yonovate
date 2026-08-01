// 标准库头文件
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// ESP-IDF 组件头文件
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 自定义组件头文件
#include "gpio_manager.h"
#include "gpio_monitor.h"
#include "oled.h"
#include "state_machine.h"
#include "sm_version.h"

#ifdef __cplusplus
extern "C" {
#endif

// 日志标签
static const char *TAG = "GPIO_MONITOR";

// 任务句柄
static TaskHandle_t s_gpio_monitor_task_handle = NULL;

// 系统状态实例
static system_status_t s_system_status = {
    .wifi_connected = false,
    .mqtt_connected = false,
    .device_online = true,
    .ip_addr = "0.0.0.0",
    .firmware_version = "1.0.0",
    .current_step = "IDLE"
};

// 上次显示的内容，用于检测变化
static char s_last_display[8][32] = {{0}};

// 系统状态更新函数
esp_err_t gpio_monitor_update_status(system_status_t *status)
{
    if (status == NULL) {
        ESP_LOGE(TAG, "Status parameter is null");
        return ESP_FAIL;
    }

    // 更新系统状态
    memcpy(&s_system_status, status, sizeof(s_system_status));

    ESP_LOGI(TAG, "System status updated: WiFi=%s, MQTT=%s, IP=%s",
             s_system_status.wifi_connected ? "Connected" : "Disconnected",
             s_system_status.mqtt_connected ? "Connected" : "Disconnected",
             s_system_status.ip_addr);

    return ESP_OK;
}

// IP地址更新函数
esp_err_t gpio_monitor_update_ip(const char *ip_addr)
{
    if (ip_addr == NULL) {
        ESP_LOGE(TAG, "IP address parameter is null");
        return ESP_FAIL;
    }

    // 更新IP地址
    strncpy(s_system_status.ip_addr, ip_addr, sizeof(s_system_status.ip_addr) - 1);
    s_system_status.ip_addr[sizeof(s_system_status.ip_addr) - 1] = '\0';

    ESP_LOGI(TAG, "IP address updated: %s", s_system_status.ip_addr);

    return ESP_OK;
}

// 更新组件状态
esp_err_t gpio_monitor_update_component_status(const char* component, bool status)
{
    if (component == NULL) {
        ESP_LOGE(TAG, "Component name parameter is null");
        return ESP_FAIL;
    }

    // 更新组件状态
    if (strcmp(component, "net") == 0) {
        s_system_status.wifi_connected = status;
        ESP_LOGI(TAG, "net status updated: %s", status ? "Connected" : "Disconnected");
    } else if (strcmp(component, "mqtt") == 0) {
        s_system_status.mqtt_connected = status;
        ESP_LOGI(TAG, "MQTT status updated: %s", status ? "Connected" : "Disconnected");
    } else if (strcmp(component, "device") == 0) {
        s_system_status.device_online = status;
        ESP_LOGI(TAG, "Device status updated: %s", status ? "Online" : "Offline");
    } else if (strcmp(component, "i2c") == 0) {
        // I2C组件状态更新
        ESP_LOGI(TAG, "I2C status updated: %s", status ? "OK" : "Error");
        // 这里可以添加I2C特定的状态处理逻辑
    } else if (strcmp(component, "oled") == 0) {
        // OLED组件状态更新
        ESP_LOGI(TAG, "OLED status updated: %s", status ? "OK" : "Error");
        // 这里可以添加OLED特定的状态处理逻辑
    } else if (strcmp(component, "gpio") == 0) {
        // GPIO管理器状态更新
        ESP_LOGI(TAG, "GPIO Manager status updated: %s", status ? "OK" : "Error");
    } else if (strcmp(component, "monitor") == 0) {
        // 监控任务状态更新
        ESP_LOGI(TAG, "Monitor task status updated: %s", status ? "OK" : "Error");
    } else if (strcmp(component, "state_machine") == 0) {
        // 状态机状态更新
        ESP_LOGI(TAG, "State Machine status updated: %s", status ? "OK" : "Error");
    } else {
        ESP_LOGE(TAG, "Unknown component: %s", component);
        return ESP_FAIL;
    }

    return ESP_OK;
}

// 更新当前步骤
esp_err_t gpio_monitor_update_step(const char* step)
{
    if (step == NULL) {
        ESP_LOGE(TAG, "Step parameter is null");
        return ESP_FAIL;
    }

    strncpy(s_system_status.current_step, step, sizeof(s_system_status.current_step) - 1);
    s_system_status.current_step[sizeof(s_system_status.current_step) - 1] = '\0';

    ESP_LOGI(TAG, "Current step updated: %s", s_system_status.current_step);

    return ESP_OK;
}

// 获取系统状态
esp_err_t gpio_monitor_get_status(system_status_t *status)
{
    if (status == NULL) {
        ESP_LOGE(TAG, "Status parameter is null");
        return ESP_FAIL;
    }

    memcpy(status, &s_system_status, sizeof(s_system_status));

    return ESP_OK;
}

// GPIO状态显示函数
esp_err_t gpio_display_states(void)
{
    // 定义显示缓冲区
    char line[64] = {0};
    bool updated = false;

    // 第0行：显示系统状态和步骤信息（合并显示）
    if (s_system_status.current_step[0] != '\0') {
        // 只显示最新的步骤信息
        snprintf(line, sizeof(line), "%s", s_system_status.current_step);
        if (strcmp(line, s_last_display[0]) != 0) {
            oled_display_string(0, line);
            strncpy(s_last_display[0], line, sizeof(s_last_display[0]) - 1);
            updated = true;
        }
    }

    // 第1行：显示IP地址或占位符
    if (s_system_status.ip_addr[0] != '\0') {
        snprintf(line, sizeof(line), "IP: %s", s_system_status.ip_addr);
    } else {
        snprintf(line, sizeof(line), "IP: ---.---.---.---");
    }
    if (strcmp(line, s_last_display[1]) != 0) {
        oled_display_string(1, line);
        strncpy(s_last_display[1], line, sizeof(s_last_display[1]) - 1);
        updated = true;
    }
    memset(line, 0, sizeof(line));
    // 第3-5行：显示12组IO数据，每行4个
    // 第3行：显示G1-G4
    for (int i = 0; i < 4; i++) {
        gpio_group_t group;
        if (gpio_manager_get_group_info(i, &group) == ESP_OK) {
            char groupName[4] = "";
            snprintf(groupName, sizeof(groupName), "P%d", i + 1);

            snprintf(line + strlen(line), sizeof(line) - strlen(line), "%s:%d%d ",
                     groupName,
                     group.states[0] > 0 ? 1 : 0,
                     group.states[1] > 0 ? 1 : 0);
        }
    }
    if (strcmp(line, s_last_display[2]) != 0) {
        oled_display_string(3, line);
        strncpy(s_last_display[2], line, sizeof(s_last_display[2]) - 1);
        updated = true;
    }
    memset(line, 0, sizeof(line));

    // 第4行：显示E1-E4 (组4-7)
    for (int i = 4; i < 8; i++) {
        gpio_group_t group;
        if (gpio_manager_get_group_info(i, &group) == ESP_OK) {
            snprintf(line + strlen(line), sizeof(line) - strlen(line), "E%d:%d%d ",
                     i - 3,
                     group.states[0] > 0 ? 1 : 0,
                     group.states[1] > 0 ? 1 : 0);
        }
    }
    if (strcmp(line, s_last_display[3]) != 0) {
        oled_display_string(4, line);
        strncpy(s_last_display[3], line, sizeof(s_last_display[3]) - 1);
        updated = true;
    }
    memset(line, 0, sizeof(line));

    // 第5行：显示E5-E8 (组8-11)
    for (int i = 8; i < 12; i++) {
        gpio_group_t group;
        if (gpio_manager_get_group_info(i, &group) == ESP_OK) {
            snprintf(line + strlen(line), sizeof(line) - strlen(line), "E%d:%d%d ",
                     i - 3,
                     group.states[0] > 0 ? 1 : 0,
                     group.states[1] > 0 ? 1 : 0);
        }
    }
    if (strcmp(line, s_last_display[4]) != 0) {
        oled_display_string(5, line);
        strncpy(s_last_display[4], line, sizeof(s_last_display[4]) - 1);
        updated = true;
    }
    memset(line, 0, sizeof(line));

    // 第6行：显示M1、M2电机组和A、B按钮
    motor_group_t motor1, motor2;
    int a_state = 0, b_state = 0;

    // 获取电机组状态
    if (gpio_manager_get_motor_info(0, &motor1) != ESP_OK) {
        motor1.states[0] = 0;
        motor1.states[1] = 0;
    }
    if (gpio_manager_get_motor_info(1, &motor2) != ESP_OK) {
        motor2.states[0] = 0;
        motor2.states[1] = 0;
    }

    // 获取按钮状态
    gpio_manager_get_button_state(0, &a_state);
    gpio_manager_get_button_state(1, &b_state);

    // 计算电机状态（0:停止, 1:正转, 2:反转）
    int m1_state = 0;
    int m2_state = 0;
    if (motor1.states[0] == 1 && motor1.states[1] == 0) m1_state = 1;
    if (motor1.states[0] == 0 && motor1.states[1] == 1) m1_state = 2;
    if (motor2.states[0] == 1 && motor2.states[1] == 0) m2_state = 1;
    if (motor2.states[0] == 0 && motor2.states[1] == 1) m2_state = 2;

    snprintf(line, sizeof(line), "M1:%d M2:%d A:%d B:%d", m1_state, m2_state, a_state, b_state);
    if (strcmp(line, s_last_display[6]) != 0) {
        oled_display_string(6, line);
        strncpy(s_last_display[6], line, sizeof(s_last_display[6]) - 1);
        updated = true;
    }
    memset(line, 0, sizeof(line));

    // 第7行：显示状态机的状态和电压
    uint32_t voltage = 0;
    if (gpio_manager_get_voltage(&voltage) != ESP_OK) {
        ESP_LOGD(TAG, "Failed to get voltage");
        voltage = -1;
    }
    
    // 获取状态机当前状态（自动根据版本 V2/V3 路由）
    char current_state[16] = "IDLE";
    if (sm_get_current_state(current_state, sizeof(current_state)) != ESP_OK) {
        ESP_LOGD(TAG, "Failed to get state machine status");
        strcpy(current_state, "---");
    }

    snprintf(line, sizeof(line), "S:%-14s V:%.2f", current_state, (float)voltage / 1000.0f);
    
    if (strcmp(line, s_last_display[7]) != 0) {
        oled_display_string(7, line);
        strncpy(s_last_display[7], line, sizeof(s_last_display[7]) - 1);
        updated = true;
    }

    // 如果有更新，则刷新OLED显示
    if (updated) {
        oled_refresh();
    }

    return ESP_OK;
}

// GPIO监视器任务函数
static void gpio_monitor_task(void *pvParameters)
{
    ESP_LOGI(TAG, "GPIO monitor task started");

    // 主循环
    while (1) {
        // 更新GPIO状态
        if (gpio_manager_update_states() != ESP_OK) {
            ESP_LOGE(TAG, "Failed to update GPIO states");
        }

        // 显示GPIO状态
        if (gpio_display_states() != ESP_OK) {
            ESP_LOGE(TAG, "Failed to display GPIO states");
        }

        // 延时100ms
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// 初始化GPIO监视器
esp_err_t gpio_monitor_init(void)
{
    // 检查是否已经初始化
    if (s_gpio_monitor_task_handle != NULL) {
        ESP_LOGW(TAG, "GPIO monitor already initialized");
        return ESP_OK;
    }

    // 创建GPIO监视器任务
    if (xTaskCreate(gpio_monitor_task, "GPIO_MONITOR", 4096, NULL, 5, &s_gpio_monitor_task_handle) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create GPIO monitor task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "GPIO monitor initialized successfully");

    return ESP_OK;
}

// 停止GPIO监视器
esp_err_t gpio_monitor_deinit(void)
{
    // 检查是否已经初始化
    if (s_gpio_monitor_task_handle == NULL) {
        ESP_LOGW(TAG, "GPIO monitor not initialized");
        return ESP_OK;
    }

    // 删除GPIO监视器任务
    vTaskDelete(s_gpio_monitor_task_handle);
    s_gpio_monitor_task_handle = NULL;

    // 清除OLED显示
    oled_clear();
    oled_refresh();

    ESP_LOGI(TAG, "GPIO monitor deinitialized successfully");

    return ESP_OK;
}

#ifdef __cplusplus
}
#endif
