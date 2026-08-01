/*
 * 状态机 V3.0 - 核心 API 接口
 */

#ifndef SM_CORE_H
#define SM_CORE_H

#include <string.h>
#include "sm_types.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ============================================
 * 核心 API
 * ============================================
 */

/**
 * @brief 初始化状态机（无参版本）
 * 
 * @return ESP_OK 成功
 */
esp_err_t sm_v3_init(void);

/**
 * @brief 启动状态机
 * 
 * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 如果未初始化
 */
esp_err_t sm_v3_start(void);

/**
 * @brief 启动状态机并自动创建驱动任务（100ms周期）
 * 
 * 内部自动创建任务，无需手动调用 xTaskCreate
 * 
 * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 如果未初始化
 */
esp_err_t sm_v3_start_auto(void);

/**
 * @brief 停止状态机
 */
void sm_v3_stop(void);

/**
 * @brief 处理状态机（在主循环中调用）
 */
void sm_v3_process(void);

/**
 * @brief 重置状态机到初始状态
 */
void sm_v3_reset(void);

/**
 * @brief 获取当前状态名到缓冲区
 * 
 * @param buf 状态名输出缓冲区
 * @param buf_size 缓冲区大小
 * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 参数无效，ESP_ERR_INVALID_STATE 未初始化
 */
esp_err_t sm_v3_get_current_state(char* buf, size_t buf_size);

/**
 * @brief 获取状态机配置
 * 
 * @return 指向当前配置的指针，未初始化时为 NULL
 */
sm_config_t* sm_v3_get_config(void);

/**
 * @brief 获取当前状态名字符串指针
 *
 * @return 当前状态名字符串（未初始化时为空字符串）
 */
const char* sm_v3_get_current_state_name(void);

/**
 * @brief 检查状态机是否正在运行
 *
 * @return true 正在运行，false 未运行或未初始化
 */
bool sm_v3_is_running(void);

/**
 * @brief 手动触发状态转换
 * 
 * @param target_state 目标状态名
 * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 如果参数无效
 */
esp_err_t sm_v3_manual_trigger(const char* target_state);

/**
 * @brief 监控回调（定时器每 100ms 调用）
 * 
 * @param arg 未使用的参数（用于定时器回调兼容性）
 */
void sm_v3_monitor_callback(void *arg);

/**
 * @brief 从字符串解析ID (P1->1, T0->0, M0->0)
 * 
 * @param str ID字符串
 * @param prefix 期望的前缀字符
 * @param max_count 最大有效值
 * @return 解析成功的ID，失败返回-1
 */
int sm_parse_id(const char *str, char prefix, int max_count);

#ifdef __cplusplus
}
#endif

#endif /* SM_CORE_H */
