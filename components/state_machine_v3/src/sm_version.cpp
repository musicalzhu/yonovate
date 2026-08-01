/*
 * 状态机版本切换模块实现 (使用 nvs_manager)
 */

extern "C" {
#include <string.h>
#include "sm_version.h"
#include "nvs_manager.h"
#include "esp_log.h"
#include "sm_core.h"
#include "state_machine.h"
}

static const char *TAG = "SM_VERSION";
static int8_t s_cached_version = -1;  // 缓存变量，-1表示未初始化

int8_t sm_version_get(void)
{
    // 如果已缓存，直接返回（避免重复NVS读取和日志刷屏）
    if (s_cached_version >= 0) {
        return s_cached_version;
    }

    int8_t version = SM_VERSION_DEFAULT;
    esp_err_t err = nvs_mgr_get_i8("sm_config", SM_VERSION_KEY, &version);
    
    if (err != ESP_OK || !sm_version_is_valid(version)) {
        ESP_LOGW(TAG, "Using default version V%d", SM_VERSION_DEFAULT);
        version = SM_VERSION_DEFAULT;
    }
    
    ESP_LOGI(TAG, "Current version: V%d (%s)", version, sm_version_to_string(version));
    
    s_cached_version = version;  // 缓存结果
    return version;
}

esp_err_t sm_version_set(int8_t version)
{
    if (!sm_version_is_valid(version)) {
        ESP_LOGE(TAG, "Invalid version: %d", version);
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = nvs_mgr_set_i8("sm_config", SM_VERSION_KEY, version);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Version saved: V%d (%s)", version, sm_version_to_string(version));
        s_cached_version = version;  // 更新缓存
    }
    return err;
}

bool sm_version_is_valid(int8_t version)
{
    return (version == SM_VERSION_V2 || version == SM_VERSION_V3);
}

const char* sm_version_to_string(int8_t version)
{
    switch (version) {
        case SM_VERSION_V2:
            return "V2";
        case SM_VERSION_V3:
            return "V3";
        default:
            return "未知版本";
    }
}

esp_err_t sm_init(void)
{
    int8_t version = sm_version_get();
    esp_err_t err;

    ESP_LOGI(TAG, "初始化状态机 V%d...", version);

    // 初始化
    err = (version == SM_VERSION_V3) ? sm_v3_init() : state_machine_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "状态机 V%d 初始化失败", version);
        return err;
    }

    // 启动
    err = (version == SM_VERSION_V3) ?
          sm_v3_start_auto() : state_machine_start("idle");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "状态机 V%d 启动失败", version);
        return err;
    }

    ESP_LOGI(TAG, "状态机 V%d 初始化完成", version);
    return ESP_OK;
}

esp_err_t sm_get_current_state(char* buf, size_t buf_size)
{
    if (buf == NULL || buf_size == 0) {
        ESP_LOGW(TAG, "Invalid buffer for state query");
        return ESP_ERR_INVALID_ARG;
    }

    int8_t version = sm_version_get();
    ESP_LOGD(TAG, "Querying state for V%d", version);

    if (version == SM_VERSION_V3) {
        return sm_v3_get_current_state(buf, buf_size);
    } else if (version == SM_VERSION_V2) {
        return state_machine_get_current_state(buf, buf_size);
    } else {
        ESP_LOGW(TAG, "Invalid version %d, defaulting to IDLE", version);
        strncpy(buf, "IDLE", buf_size);
        buf[buf_size - 1] = '\0';
        return ESP_ERR_INVALID_STATE;
    }
}
