/*
 * GPIO监控模块头文件
 * 负责监控系统状态、GPIO状态并显示在OLED上
 */

#ifndef GPIO_MONITOR_H
#define GPIO_MONITOR_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// 系统状态结构体
typedef struct {
    bool wifi_connected;     // WiFi连接状态
    bool mqtt_connected;     // MQTT连接状态
    bool device_online;      // 设备在线状态
    char ip_addr[16];        // IP地址
    char firmware_version[16]; // 固件版本
    char current_step[32];   // 当前步骤文本
} system_status_t;



// 显示系统状态
esp_err_t gpio_display_states(void);

// 初始化GPIO监控
esp_err_t gpio_monitor_init(void);

// 反初始化GPIO监控
esp_err_t gpio_monitor_deinit(void);

// 启动GPIO监控
esp_err_t gpio_monitor_start(void);

// 更新组件状态
esp_err_t gpio_monitor_update_component_status(const char* component, bool status);

// 更新当前步骤
esp_err_t gpio_monitor_update_step(const char *step);

// 更新 IP 地址
esp_err_t gpio_monitor_update_ip(const char *ip_addr);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_MONITOR_H */
