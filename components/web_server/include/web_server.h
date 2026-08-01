/*
 * Web服务器模块头文件
 * 提供HTTP服务器功能和API接口
 */

/*
 * Web服务器模块头文件
 * 提供HTTP服务器功能和API接口
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdio.h>
#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化 Web Server，自动注册 WiFi 状态监听
 * 
 * 调用后 Web Server 将在 WiFi 就绪时自动启动，
 * WiFi 断开时自动停止。无需手动调用 start/stop。
 */
void web_server_init(void);

/**
 * @brief 启动HTTP服务器（内部使用，通常无需手动调用）
 * 
 * 在 web_server_init() 后，服务器会根据 WiFi 状态自动启动。
 * 如需手动控制（如暂停服务但保持 WiFi），可调用此函数。
 * 
 * @return esp_err_t ESP_OK 成功，ESP_FAIL 失败
 */
esp_err_t web_server_start(void);

/**
 * @brief 停止HTTP服务器（内部使用，通常无需手动调用）
 * 
 * @return esp_err_t 操作结果
 */
esp_err_t web_server_stop(void);

/**
 * @brief 检查 Web Server 是否正在运行
 * 
 * @return true 运行中，false 已停止
 */
bool web_server_is_running(void);

/**
 * @brief 获取 Web Server HTTP 句柄
 * 
 * @return httpd_handle_t 句柄，未启动时返回 NULL
 */
httpd_handle_t web_server_get_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* WEB_SERVER_H */