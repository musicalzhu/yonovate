/**
 * @file nvs_manager.h
 * @brief NVS Manager - 统一的非易失性存储封装组件
 * 
 * 简化 ESP-IDF NVS API 的使用，提供类型安全的键值存储接口。
 * 支持懒加载初始化，自动错误恢复，统一日志输出。
 * 
 * @version 1.0.0
 * @date 2026-04-29
 */

#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 常量定义 ========== */

#define NVS_MGR_NS_MAX_LEN   16  /**< 命名空间最大长度 (含终止符) */
#define NVS_MGR_KEY_MAX_LEN  16  /**< 键名最大长度 (含终止符) */

/* ========== 核心 API: int8 ========== */

/**
 * @brief 获取 int8 值
 * @param ns_name NVS 命名空间（最大15字符）
 * @param key 键名（最大15字符）
 * @param out 输出值指针
 * @return ESP_OK 成功，ESP_ERR_NVS_NOT_FOUND 键不存在，其他错误码失败
 */
esp_err_t nvs_mgr_get_i8(const char* ns_name, const char* key, int8_t* out);

/**
 * @brief 设置 int8 值
 * @param ns_name NVS 命名空间
 * @param key 键名
 * @param value 值
 * @return ESP_OK 成功
 */
esp_err_t nvs_mgr_set_i8(const char* ns_name, const char* key, int8_t value);

/* ========== 核心 API: int32 ========== */

/**
 * @brief 获取 int32 值
 */
esp_err_t nvs_mgr_get_i32(const char* ns_name, const char* key, int32_t* out);

/**
 * @brief 设置 int32 值
 */
esp_err_t nvs_mgr_set_i32(const char* ns_name, const char* key, int32_t value);

/* ========== 核心 API: 字符串 ========== */

/**
 * @brief 获取字符串
 * @param ns_name NVS 命名空间
 * @param key 键名
 * @param buf 输出缓冲区
 * @param buf_size 缓冲区大小（包含终止符）
 * @return ESP_OK 成功，ESP_ERR_NVS_INVALID_LENGTH 缓冲区不足
 */
esp_err_t nvs_mgr_get_string(const char* ns_name, const char* key, 
                              char* buf, size_t buf_size);

/**
 * @brief 设置字符串
 */
esp_err_t nvs_mgr_set_string(const char* ns_name, const char* key, 
                              const char* value);

/* ========== 核心 API: 布尔值 ========== */

/**
 * @brief 获取布尔值（以 uint8_t 存储）
 */
esp_err_t nvs_mgr_get_bool(const char* ns_name, const char* key, bool* out);

/**
 * @brief 设置布尔值
 */
esp_err_t nvs_mgr_set_bool(const char* ns_name, const char* key, bool value);

/* ========== 核心 API: 二进制数据 ========== */

/**
 * @brief 获取二进制数据
 * @param buf_size 输入为缓冲区大小，输出为实际数据大小
 */
esp_err_t nvs_mgr_get_blob(const char* ns_name, const char* key, 
                            void* buf, size_t* buf_size);

/**
 * @brief 设置二进制数据
 */
esp_err_t nvs_mgr_set_blob(const char* ns_name, const char* key, 
                            const void* data, size_t data_size);

/* ========== 初始化 API ========== */

/**
 * @brief 初始化 NVS（显式调用，也用于懒加载）
 * @return ESP_OK 成功
 */
esp_err_t nvs_mgr_init(void);

/**
 * @brief 检查 NVS 是否已初始化
 */
bool nvs_mgr_is_initialized(void);

/* ========== 事务 API ========== */

/**
 * @brief 开始事务（优化批量写操作）
 * @param ns_name 目标命名空间
 * @return ESP_OK 成功
 * @note 事务期间，同一命名空间的写操作被缓存，提交时统一写入
 * @note 当前实现为简化版，直接打开命名空间保持，不实现复杂缓存
 */
esp_err_t nvs_mgr_begin_transaction(const char* ns_name);

/**
 * @brief 提交事务
 */
esp_err_t nvs_mgr_commit_transaction(void);

/**
 * @brief 中止事务（丢弃所有未提交更改）
 */
esp_err_t nvs_mgr_abort_transaction(void);

/* ========== 管理 API ========== */

/**
 * @brief 擦除命名空间所有数据
 */
esp_err_t nvs_mgr_erase_namespace(const char* ns_name);

/**
 * @brief 擦除整个 NVS 分区（慎用）
 */
esp_err_t nvs_mgr_erase_all(void);

/**
 * @brief 获取 NVS 统计信息（简化实现）
 * @param buf 输出缓冲区
 * @param buf_size 缓冲区大小
 * @return ESP_OK 成功
 */
esp_err_t nvs_mgr_get_stats(char* buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* NVS_MANAGER_H */
