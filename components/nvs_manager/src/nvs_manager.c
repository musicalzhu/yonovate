/**
 * @file nvs_manager.c
 * @brief NVS Manager 实现
 * 
 * 统一的 NVS 封装层实现，提供懒加载初始化、类型安全 API 和错误恢复。
 */

#include "nvs_manager.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "nvs_mgr";

/* ========== 内部状态 ========== */

static bool g_initialized = false;
static nvs_handle_t g_trans_handle = 0;  /**< 事务句柄 */
static char g_trans_ns[16] = {0};          /**< 事务命名空间 */
static bool g_in_transaction = false;      /**< 事务状态 */

/* ========== 内部函数 ========== */

/* ========== 初始化 API ========== */

/**
 * @brief 初始化 NVS（显式调用，也用于懒加载）
 * @return ESP_OK 成功
 */
esp_err_t nvs_mgr_init(void)
{
    if (g_initialized) {
        return ESP_OK;
    }
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS 需要擦除，正在恢复...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    if (ret == ESP_OK) {
        g_initialized = true;
        ESP_LOGI(TAG, "NVS Manager 初始化完成");
    } else {
        ESP_LOGE(TAG, "NVS 初始化失败: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/**
 * @brief 统一错误处理
 */
static esp_err_t handle_error(esp_err_t err, const char* context)
{
    switch (err) {
        case ESP_OK:
            return ESP_OK;
            
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGD(TAG, "%s: 键不存在", context);
            return ESP_ERR_NVS_NOT_FOUND;
            
        case ESP_ERR_NVS_INVALID_LENGTH:
            ESP_LOGW(TAG, "%s: 缓冲区不足", context);
            return ESP_ERR_NVS_INVALID_LENGTH;
            
        default:
            if (err != ESP_OK) {
                ESP_LOGW(TAG, "%s: 错误 %d (%s)", context, err, esp_err_to_name(err));
            }
            return err;
    }
}

/* ========== 初始化 API ========== */

bool nvs_mgr_is_initialized(void)
{
    return g_initialized;
}

/* ========== int8 API ========== */

esp_err_t nvs_mgr_get_i8(const char* ns_name, const char* key, int8_t* out)
{
    if (!ns_name || !key || !out) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = nvs_mgr_init();
    if (err != ESP_OK) return err;

    // 事务模式下使用事务句柄
    if (g_in_transaction && strcmp(g_trans_ns, ns_name) == 0) {
        err = nvs_get_i8(g_trans_handle, key, out);
        return handle_error(err, "get_i8");
    }

    nvs_handle_t handle;
    err = nvs_open(ns_name, NVS_READONLY, &handle);
    if (err != ESP_OK) return handle_error(err, "nvs_open");

    err = nvs_get_i8(handle, key, out);
    nvs_close(handle);

    return handle_error(err, "get_i8");
}

esp_err_t nvs_mgr_set_i8(const char* ns_name, const char* key, int8_t value)
{
    if (!ns_name || !key) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = nvs_mgr_init();
    if (err != ESP_OK) return err;
    
    // 事务模式下使用事务句柄
    if (g_in_transaction && strcmp(g_trans_ns, ns_name) == 0) {
        err = nvs_set_i8(g_trans_handle, key, value);
        return handle_error(err, "set_i8");
    }
    
    nvs_handle_t handle;
    err = nvs_open(ns_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) return handle_error(err, "nvs_open");
    
    err = nvs_set_i8(handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    
    return handle_error(err, "set_i8");
}

/* ========== int32 API ========== */

esp_err_t nvs_mgr_get_i32(const char* ns_name, const char* key, int32_t* out)
{
    if (!ns_name || !key || !out) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = nvs_mgr_init();
    if (err != ESP_OK) return err;
    
    if (g_in_transaction && strcmp(g_trans_ns, ns_name) == 0) {
        err = nvs_get_i32(g_trans_handle, key, out);
        return handle_error(err, "get_i32");
    }
    
    nvs_handle_t handle;
    err = nvs_open(ns_name, NVS_READONLY, &handle);
    if (err != ESP_OK) return handle_error(err, "nvs_open");
    
    err = nvs_get_i32(handle, key, out);
    nvs_close(handle);
    
    return handle_error(err, "get_i32");
}

esp_err_t nvs_mgr_set_i32(const char* ns_name, const char* key, int32_t value)
{
    if (!ns_name || !key) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = nvs_mgr_init();
    if (err != ESP_OK) return err;
    
    if (g_in_transaction && strcmp(g_trans_ns, ns_name) == 0) {
        err = nvs_set_i32(g_trans_handle, key, value);
        return handle_error(err, "set_i32");
    }
    
    nvs_handle_t handle;
    err = nvs_open(ns_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) return handle_error(err, "nvs_open");
    
    err = nvs_set_i32(handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    
    return handle_error(err, "set_i32");
}

/* ========== string API ========== */

esp_err_t nvs_mgr_get_string(const char* ns_name, const char* key, 
                              char* buf, size_t buf_size)
{
    if (!ns_name || !key || !buf || buf_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = nvs_mgr_init();
    if (err != ESP_OK) return err;
    
    if (g_in_transaction && strcmp(g_trans_ns, ns_name) == 0) {
        err = nvs_get_str(g_trans_handle, key, buf, &buf_size);
        return handle_error(err, "get_string");
    }
    
    nvs_handle_t handle;
    err = nvs_open(ns_name, NVS_READONLY, &handle);
    if (err != ESP_OK) return handle_error(err, "nvs_open");
    
    err = nvs_get_str(handle, key, buf, &buf_size);
    nvs_close(handle);
    
    return handle_error(err, "get_string");
}

esp_err_t nvs_mgr_set_string(const char* ns_name, const char* key, 
                              const char* value)
{
    if (!ns_name || !key || !value) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = nvs_mgr_init();
    if (err != ESP_OK) return err;
    
    if (g_in_transaction && strcmp(g_trans_ns, ns_name) == 0) {
        err = nvs_set_str(g_trans_handle, key, value);
        return handle_error(err, "set_string");
    }
    
    nvs_handle_t handle;
    err = nvs_open(ns_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) return handle_error(err, "nvs_open");
    
    err = nvs_set_str(handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    
    return handle_error(err, "set_string");
}

/* ========== bool API ========== */

esp_err_t nvs_mgr_get_bool(const char* ns_name, const char* key, bool* out)
{
    if (!ns_name || !key || !out) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t val = 0;
    esp_err_t err = nvs_mgr_get_i8(ns_name, key, (int8_t*)&val);
    *out = (val != 0);
    return err;
}

esp_err_t nvs_mgr_set_bool(const char* ns_name, const char* key, bool value)
{
    if (!ns_name || !key) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t val = value ? 1 : 0;
    return nvs_mgr_set_i8(ns_name, key, (int8_t)val);
}

/* ========== blob API ========== */

esp_err_t nvs_mgr_get_blob(const char* ns_name, const char* key, 
                            void* buf, size_t* buf_size)
{
    if (!ns_name || !key || !buf || !buf_size || *buf_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = nvs_mgr_init();
    if (err != ESP_OK) return err;
    
    if (g_in_transaction && strcmp(g_trans_ns, ns_name) == 0) {
        err = nvs_get_blob(g_trans_handle, key, buf, buf_size);
        return handle_error(err, "get_blob");
    }
    
    nvs_handle_t handle;
    err = nvs_open(ns_name, NVS_READONLY, &handle);
    if (err != ESP_OK) return handle_error(err, "nvs_open");
    
    err = nvs_get_blob(handle, key, buf, buf_size);
    nvs_close(handle);
    
    return handle_error(err, "get_blob");
}

esp_err_t nvs_mgr_set_blob(const char* ns_name, const char* key, 
                            const void* data, size_t data_size)
{
    if (!ns_name || !key || !data || data_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = nvs_mgr_init();
    if (err != ESP_OK) return err;
    
    if (g_in_transaction && strcmp(g_trans_ns, ns_name) == 0) {
        err = nvs_set_blob(g_trans_handle, key, data, data_size);
        return handle_error(err, "set_blob");
    }
    
    nvs_handle_t handle;
    err = nvs_open(ns_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) return handle_error(err, "nvs_open");
    
    err = nvs_set_blob(handle, key, data, data_size);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    
    return handle_error(err, "set_blob");
}

/* ========== 事务 API ========== */

esp_err_t nvs_mgr_begin_transaction(const char* ns_name)
{
    if (!ns_name) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (g_in_transaction) {
        ESP_LOGW(TAG, "已有进行中的事务，正在中止...");
        nvs_mgr_abort_transaction();
    }
    
    esp_err_t err = nvs_mgr_init();
    if (err != ESP_OK) return err;
    
    err = nvs_open(ns_name, NVS_READWRITE, &g_trans_handle);
    if (err != ESP_OK) {
        return handle_error(err, "begin_transaction");
    }
    
    strncpy(g_trans_ns, ns_name, sizeof(g_trans_ns) - 1);
    g_trans_ns[sizeof(g_trans_ns) - 1] = '\0';
    g_in_transaction = true;
    
    ESP_LOGD(TAG, "事务开始: namespace=%s", ns_name);
    return ESP_OK;
}

esp_err_t nvs_mgr_commit_transaction(void)
{
    if (!g_in_transaction) {
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t err = nvs_commit(g_trans_handle);
    nvs_close(g_trans_handle);
    
    g_in_transaction = false;
    g_trans_handle = 0;
    memset(g_trans_ns, 0, sizeof(g_trans_ns));
    
    ESP_LOGD(TAG, "事务提交: %s", err == ESP_OK ? "成功" : "失败");
    return handle_error(err, "commit_transaction");
}

esp_err_t nvs_mgr_abort_transaction(void)
{
    if (!g_in_transaction) {
        return ESP_ERR_INVALID_STATE;
    }
    
    nvs_close(g_trans_handle);
    
    g_in_transaction = false;
    g_trans_handle = 0;
    memset(g_trans_ns, 0, sizeof(g_trans_ns));
    
    ESP_LOGD(TAG, "事务中止");
    return ESP_OK;
}

/* ========== 管理 API ========== */

esp_err_t nvs_mgr_erase_namespace(const char* ns_name)
{
    if (!ns_name) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = nvs_mgr_init();
    if (err != ESP_OK) return err;
    
    // 如果正在事务中且同名，先中止
    if (g_in_transaction && strcmp(g_trans_ns, ns_name) == 0) {
        nvs_mgr_abort_transaction();
    }
    
    nvs_handle_t handle;
    err = nvs_open(ns_name, NVS_READWRITE, &handle);
    if (err != ESP_OK) return handle_error(err, "erase_namespace");
    
    err = nvs_erase_all(handle);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    
    ESP_LOGI(TAG, "命名空间已擦除: %s", ns_name);
    return handle_error(err, "erase_namespace");
}

esp_err_t nvs_mgr_erase_all(void)
{
    esp_err_t err = nvs_mgr_init();
    if (err != ESP_OK) return err;
    
    // 中止任何进行中的事务
    if (g_in_transaction) {
        nvs_mgr_abort_transaction();
    }
    
    err = nvs_flash_erase();
    if (err == ESP_OK) {
        g_initialized = false;  // 强制重新初始化
        ESP_LOGI(TAG, "NVS 全部分区已擦除");
    } else {
        ESP_LOGE(TAG, "擦除失败: %s", esp_err_to_name(err));
    }
    
    return err;
}

esp_err_t nvs_mgr_get_stats(char* buf, size_t buf_size)
{
    if (!buf || buf_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = nvs_mgr_init();
    if (err != ESP_OK) {
        snprintf(buf, buf_size, "NVS 未初始化");
        return err;
    }
    
    nvs_stats_t stats;
    err = nvs_get_stats(NULL, &stats);
    if (err != ESP_OK) {
        snprintf(buf, buf_size, "统计获取失败: %s", esp_err_to_name(err));
        return err;
    }
    
    snprintf(buf, buf_size, 
             "已用: %zu 条目, 空闲: %zu 条目, 命名空间: %zu 个",
             stats.used_entries, stats.free_entries, stats.namespace_count);
    
    return ESP_OK;
}
