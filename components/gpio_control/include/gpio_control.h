#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief GPIO控制处理函数
 * 
 * @param req HTTP请求
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_control_handler(httpd_req_t *req);

/**
 * @brief GPIO状态获取处理函数
 * 
 * @param req HTTP请求
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_state_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif