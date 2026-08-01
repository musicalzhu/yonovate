/*
 * 状态机 V3.0 - JSON 接口
 * 
 * 文件: components/state_machine_v3/include/sm_json.h
 * 描述: JSON 配置解析接口
 * 
 * 合规: PRD-001, ARCH-001
 */

#ifndef SM_JSON_H
#define SM_JSON_H

#include "sm_types.h"
#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 将 JSON 配置字符串解析为配置结构体
 * 
 * 解析 JSON 配置并填充提供的配置结构体。
 * 此函数在内部由 sm_api.cpp 用于加载配置。
 * 
 * @param json_str 包含配置的 JSON 字符串
 * @param config 指向要填充的 sm_config_t 的指针
 * 
 * @return 成功返回 ESP_OK，JSON 无效返回 ESP_ERR_INVALID_ARG
 */
esp_err_t sm_json_parse_config(const char* json_str, sm_config_t* config);

/**
 * @brief 将配置序列化为 JSON
 * 
 * 将当前配置序列化为 JSON 字符串。
 * 用于 HTTP GET /api/v3/sm/state 端点。
 * 
 * @param buf 存储 JSON 字符串的缓冲区
 * @param buf_size 缓冲区大小
 * 
 * @return 成功返回 ESP_OK，缓冲区太小返回 ESP_ERR_NO_MEM
 */
esp_err_t sm_v3_get_config_json(char *buf, size_t buf_size);

/**
 * @brief 从 JSON 字符串加载配置
 * 
 * 解析 JSON 并直接加载到状态机核心。
 * sm_json_parse_config 的便捷包装函数。
 * 
 * @param json_str 包含配置的 JSON 字符串
 * 
 * @return 成功返回 ESP_OK
 */
esp_err_t sm_v3_load_config(const char *json_str);

const char* sm_v3_get_last_load_status(void);
bool sm_v3_get_last_load_success(void);

#ifdef __cplusplus
}
#endif

#endif /* SM_JSON_H */
