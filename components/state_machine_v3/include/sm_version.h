/*
 * 状态机版本切换模块
 * 
 * 支持在 V2 和 V3 之间运行时切换
 */

#ifndef SM_VERSION_H
#define SM_VERSION_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_VERSION_KEY        "sm_version"
#define SM_VERSION_DEFAULT    3   /* 默认 V3 */
#define SM_VERSION_V2         2
#define SM_VERSION_V3         3

/**
 * @brief 从 NVS 读取当前激活的状态机版本
 * @return 版本号 (2 或 3)，读取失败返回老版本2
 */
int8_t sm_version_get(void);

/**
 * @brief 保存状态机版本到 NVS
 * @param version 版本号 (2 或 3)
 * @return ESP_OK 成功，其他错误码失败
 */
esp_err_t sm_version_set(int8_t version);

/**
 * @brief 验证版本号有效性
 * @param version 待验证的版本号
 * @return true 有效，false 无效
 */
bool sm_version_is_valid(int8_t version);

/**
 * @brief 获取版本字符串描述
 * @param version 版本号
 * @return 版本描述字符串
 */
const char* sm_version_to_string(int8_t version);

/**
 * @brief 统一初始化并启动状态机（自动根据 NVS 版本选择 V2/V3）
 * @return ESP_OK 成功
 * @note 内部自动处理版本判断、初始化、启动和 gpio_monitor 状态更新
 */
esp_err_t sm_init(void);

/**
 * @brief 获取当前状态机状态名（自动根据版本选择 V2/V3）
 * @param buf 状态名输出缓冲区
 * @param buf_size 缓冲区大小
 * @return ESP_OK 成功，其他错误码失败
 * @note 内部自动处理版本判断，调用对应版本的状态查询函数
 */
esp_err_t sm_get_current_state(char* buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* SM_VERSION_H */
