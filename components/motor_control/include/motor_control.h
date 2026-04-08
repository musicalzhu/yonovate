#pragma once

#include <stdio.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明httpd_req_t类型
typedef struct httpd_req httpd_req_t;

/**
 * @brief 电机控制处理函数
 * 
 * @param req HTTP请求
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t motor_control_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif