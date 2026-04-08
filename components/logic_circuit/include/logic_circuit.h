#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 启动逻辑电路监控
 * @note 会初始化所有必要的数据结构并启动监控任务
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t logic_circuit_start(void);

/**
 * @brief 停止逻辑电路监控
 * @note 会停止监控任务并清空所有状态
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t logic_circuit_stop(void);

/**
 * @brief 停止逻辑电路监控任务
 */
void logic_circuit_monitor_stop(void);

/**
 * @brief 重置逻辑电路配置
 */
void logic_circuit_reset_config(void);

/**
 * @brief 更新逻辑电路状态
 */
void logic_circuit_update_states(void);

/**
 * @brief 逻辑电路配置处理函数
 * 
 * @param req HTTP请求
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t logic_circuit_handler(httpd_req_t *req);

#ifdef __cplusplus
}
#endif