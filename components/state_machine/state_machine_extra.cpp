#include "state_machine.h"
#include "esp_log.h"
#include "cJSON.h"
#include "include/state_machine.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "state_machine_extra";

extern state_machine_config_t g_state_machine_config;

/**
 * @brief 获取状态机当前状态
 * @param state_name 存储当前状态名称的缓冲区
 * @param max_len 缓冲区最大长度
 * @return ESP_OK 成功
 *         ESP_FAIL 失败
 */
esp_err_t state_machine_get_current_state(char *state_name, size_t max_len) {
    if (state_name == NULL || max_len == 0) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_FAIL;
    }
    
    strncpy(state_name, g_state_machine_config.current_state, max_len - 1);
    state_name[max_len - 1] = '\0';
    
    return ESP_OK;
}

/**
 * @brief 获取状态机配置（JSON格式）
 * @return cJSON* 状态机配置的JSON对象，使用完毕后需要调用cJSON_Delete释放
 */
cJSON *state_machine_get_config_json(void) {
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "Failed to create JSON root object");
        return NULL;
    }
    
    // 添加初始状态
    cJSON_AddStringToObject(root, "initialState", g_state_machine_config.initial_state);
    
    // 添加当前状态
    cJSON_AddStringToObject(root, "currentState", g_state_machine_config.current_state);
    
    // 添加运行状态
    cJSON_AddBoolToObject(root, "isRunning", g_state_machine_config.is_running);
    
    // 添加状态转移规则
    cJSON *transitions = cJSON_CreateArray();
    if (!transitions) {
        cJSON_Delete(root);
        ESP_LOGE(TAG, "Failed to create transitions array");
        return NULL;
    }
    cJSON_AddItemToObject(root, "transitions", transitions);
    
    for (uint32_t i = 0; i < g_state_machine_config.transition_count; i++) {
        cJSON *transition = cJSON_CreateObject();
        if (!transition) {
            cJSON_Delete(root);
            ESP_LOGE(TAG, "Failed to create transition object");
            return NULL;
        }
        
        cJSON_AddStringToObject(transition, "currentState", g_state_machine_config.transitions[i].current_state);
        cJSON_AddStringToObject(transition, "nextState", g_state_machine_config.transitions[i].next_state);
        cJSON_AddStringToObject(transition, "triggerSource", g_state_machine_config.transitions[i].trigger_source);
        
        cJSON_AddItemToArray(transitions, transition);
    }
    
    // 添加状态效果
    cJSON *state_effects = cJSON_CreateArray();
    if (!state_effects) {
        cJSON_Delete(root);
        ESP_LOGE(TAG, "Failed to create state_effects array");
        return NULL;
    }
    cJSON_AddItemToObject(root, "stateEffects", state_effects);
    
    for (uint32_t i = 0; i < g_state_machine_config.effect_count; i++) {
        cJSON *effect = cJSON_CreateObject();
        if (!effect) {
            cJSON_Delete(root);
            ESP_LOGE(TAG, "Failed to create effect object");
            return NULL;
        }
        
        cJSON_AddStringToObject(effect, "stateName", g_state_machine_config.state_effects[i].state_name);
        cJSON_AddNumberToObject(effect, "effectType", g_state_machine_config.state_effects[i].effect_type);
        cJSON_AddStringToObject(effect, "target", g_state_machine_config.state_effects[i].target);
        cJSON_AddStringToObject(effect, "effectValue", g_state_machine_config.state_effects[i].effect_value);
        cJSON_AddNumberToObject(effect, "delayMs", g_state_machine_config.state_effects[i].delay_ms);
        
        cJSON_AddItemToArray(state_effects, effect);
    }
    
    return root;
}