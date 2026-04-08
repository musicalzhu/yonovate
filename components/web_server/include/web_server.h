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
 * @brief 启动HTTP服务器
 * 
 * @return httpd_handle_t 服务器句柄，如果启动失败则返回NULL
 */
httpd_handle_t web_server_start(void);

/**
 * @brief 停止HTTP服务器
 * 
 * @param server 服务器句柄
 * @return esp_err_t 操作结果
 */
esp_err_t web_server_stop(httpd_handle_t server);

#ifdef __cplusplus
}
#endif

#endif /* WEB_SERVER_H */