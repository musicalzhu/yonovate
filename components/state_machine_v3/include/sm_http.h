/*
 * 状态机 V3.0 - HTTP API 处理函数声明
 * 
 * 文件: components/state_machine_v3/include/sm_http.h
 * 描述: V3 状态机 HTTP API 端点处理函数
 * 
 * 版本: V3.0
 */

#ifndef SM_HTTP_H
#define SM_HTTP_H

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief V3 状态机 GET 请求处理器
 */
esp_err_t sm_v3_http_get_handler(httpd_req_t *req);

/**
 * @brief V3 状态机 POST 请求处理器
 */
esp_err_t sm_v3_http_post_handler(httpd_req_t *req);

/**
 * @brief V3 状态机 DELETE 请求处理器
 */
esp_err_t sm_v3_http_delete_handler(httpd_req_t *req);

/**
 * @brief V3 状态机 OPTIONS 请求处理器
 */
esp_err_t sm_v3_http_options_handler(httpd_req_t *req);

/**
 * @brief 注册 V3 状态机所有 HTTP 端点
 * @param server HTTP 服务器句柄
 * @return ESP_OK 成功，其他错误码失败
 */
esp_err_t sm_v3_register_endpoints(httpd_handle_t server);

#ifdef __cplusplus
}
#endif

#endif /* SM_HTTP_H */
