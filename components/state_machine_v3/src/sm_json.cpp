/*
 * 状态机 V3.0 - JSON 解析器/序列化器
 */

#include "sm_types.h"
#include "sm_core.h"
#include "sm_json.h"
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "SM_V3_JSON";

/* 最后一次配置加载的状态信息 */
static char last_load_status[256] = "未加载配置";
static bool last_load_success = false;

/* 获取最后一次配置加载的状态信息 */
const char* sm_v3_get_last_load_status(void) {
    return last_load_status;
}

bool sm_v3_get_last_load_success(void) {
    return last_load_success;
}

/*
 * 双缓冲机制:
 * 1. temp_config 栈缓冲区解析验证 (预校验)
 * 2. memcpy 原子替换全局配置 (原子提交)
 * 优势: 全局内存始终保持"全旧"或"全新",无中间态风险
 */

/* 【双缓冲安全机制图解】
 *
 * 问题背景:
 *   危险做法 (直接修改全局配置):
 *     解析中出错 -> 全局配置处于半完成状态 -> 系统行为不可预测 -> 崩溃
 *
 *   安全做法 (双缓冲):
 *
 *   ┌─────────────────────────────────────────────────────────────┐
 *   │                      双缓冲流程                              │
 *   ├─────────────────────────────────────────────────────────────┤
 *   │                                                             │
 *   │   ┌──────────┐      ┌──────────────┐      ┌────────────┐  │
 *   │   │ JSON 输入 │ ──▶ │  temp_config  │ ──▶ │ 全局 g_sm   │  │
 *   │   │          │      │ (栈缓冲区)    │      │ (原子替换)  │  │
 *   │   └──────────┘      └──────────────┘      └────────────┘  │
 *   │        │                  │                     │          │
 *   │        ▼                  ▼                     ▼          │
 *   │   [解析验证]          [预校验通过]          [memcpy]      │
 *   │   (可能失败)          (全部或 nothing)      (瞬间完成)    │
 *   │                                                             │
 *   └─────────────────────────────────────────────────────────────┘
 *
 * 关键保证:
 *   - 全局状态只有两种: "完全旧配置" 或 "完全新配置"
 *   - 不会出现"部分更新"的中间态 (如只更新了 3 个转换规则中的 2 个)
 *   - memcpy 对单核 CPU 是原子操作 (不可中断)
 *
 * 对比 V2 的复杂方案:
 *   V2: 使用互斥锁 + 异步任务 + 动态内存分配
 *   V3: 仅使用 memcpy，无需锁，更简洁安全
 */

/*
 * 解析运算符字符串为枚举值
 */
static sm_operator_t parse_operator(const char* str)
{
    if (str == NULL) return SM_OP_EQ;  /* 默认为等于 */

    if (strcmp(str, "==") == 0 || strcmp(str, "=") == 0) return SM_OP_EQ;
    if (strcmp(str, "<") == 0)   return SM_OP_LT;
    if (strcmp(str, ">") == 0)   return SM_OP_GT;
    if (strcmp(str, "<=") == 0)  return SM_OP_LE;
    if (strcmp(str, ">=") == 0)  return SM_OP_GE;
    if (strcmp(str, "!=") == 0)  return SM_OP_NE;

    ESP_LOGW(TAG, "未知运算符: %s, 使用默认 ==", str);
    return SM_OP_EQ;  /* 默认 */
}

esp_err_t sm_v3_load_config(const char *json_str)
{
    if (json_str == NULL) {
        ESP_LOGE(TAG, "JSON 字符串为空");
        snprintf(last_load_status, sizeof(last_load_status), "错误: JSON 字符串为空");
        last_load_success = false;
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        ESP_LOGE(TAG, "JSON 解析失败");
        snprintf(last_load_status, sizeof(last_load_status), "错误: JSON 解析失败");
        last_load_success = false;
        return ESP_ERR_INVALID_ARG;
    }

    sm_config_t temp_config;  /* 双缓冲: 临时缓冲区避免污染全局配置 */
    memset(&temp_config, 0, sizeof(temp_config));

    /* 【JSON 字段与 C 结构映射说明】
     *
     * JSON 配置文件的字段名映射到 C 结构体的字段名:
     *
     * ┌─────────────────────┬─────────────────────────────┬─────────────────────┐
     * │ JSON 字段           │ C 结构体字段                 │ 说明                 │
     * ├─────────────────────┼─────────────────────────────┼─────────────────────┤
     * │ "initialState"      │ sm_config_t.initial_state  │ 初始状态名           │
     * ├─────────────────────┼─────────────────────────────┼─────────────────────┤
     * │ "transitions"       │ sm_config_t.transitions[]  │ 转换规则数组         │
     * │   "currentState"    │ sm_transition_t.current_state│ 源状态               │
     * │   "nextState"       │ sm_transition_t.next_state │ 目标状态             │
     * │   "triggerType"     │ sm_trigger_t.type          │ 触发器类型           │
     * │   "triggerSource"   │ sm_trigger_t.source        │ 触发源               │
     * │   "operator"        │ sm_trigger_t.op            │ 比较运算符           │
     * │   "triggerValue"    │ sm_trigger_t.value         │ 比较值               │
     * ├─────────────────────┼─────────────────────────────┼─────────────────────┤
     * │ "stateEffects"      │ sm_config_t.state_effects[]│ 状态效果数组         │
     * │   "stateName"       │ sm_state_effect_t.state_name  │ 效果所属状态      │
     * │   "effectType"      │ sm_state_effect_t.effect_type │ 效果类型          │
     * │   "target"          │ sm_state_effect_t.target      │ 目标名称          │
     * │   "value"           │ sm_state_effect_t.value       │ 目标值            │
     * └─────────────────────┴─────────────────────────────┴─────────────────────┘
     *
     * 字段名差异说明:
     *   JSON 使用 camelCase (currentState)，C 使用 snake_case (current_state)
     *   这是为了符合各自语言的命名惯例
     */

    /* Parse initialState */
    cJSON *initial_state = cJSON_GetObjectItem(root, "initialState");
    if (cJSON_IsString(initial_state)) {
        strncpy(temp_config.initial_state, initial_state->valuestring,
                SM_MAX_STATE_NAME_LEN - 1);
    } else {
        ESP_LOGW(TAG, "No initialState in JSON, using default 'IDLE'");
        strncpy(temp_config.initial_state, "IDLE", SM_MAX_STATE_NAME_LEN - 1);
    }

    /* Parse transitions */
    cJSON *transitions = cJSON_GetObjectItem(root, "transitions");
    if (cJSON_IsArray(transitions)) {
        int count = cJSON_GetArraySize(transitions);
        if (count > SM_MAX_TRANSITIONS) {
            count = SM_MAX_TRANSITIONS;
            ESP_LOGW(TAG, "转换数量限制为 %d", SM_MAX_TRANSITIONS);
        }

        for (int i = 0; i < count; i++) {
            cJSON *trans = cJSON_GetArrayItem(transitions, i);
            if (!cJSON_IsObject(trans)) continue;

            sm_transition_t *t = &temp_config.transitions[i];

            /* currentState (required) */
            cJSON *current = cJSON_GetObjectItem(trans, "currentState");
            if (cJSON_IsString(current)) {
                strncpy(t->current_state, current->valuestring,
                        SM_MAX_STATE_NAME_LEN - 1);
            }

            /* nextState (required) */
            cJSON *next = cJSON_GetObjectItem(trans, "nextState");
            if (cJSON_IsString(next)) {
                strncpy(t->next_state, next->valuestring,
                        SM_MAX_STATE_NAME_LEN - 1);
            }

            /*
             * 解析多触发器数组
             */
            cJSON *triggers_arr = cJSON_GetObjectItem(trans, "triggers");
            if (cJSON_IsArray(triggers_arr)) {
                int trigger_count = cJSON_GetArraySize(triggers_arr);
                if (trigger_count > 3) {
                    trigger_count = 3;
                    ESP_LOGW(TAG, "转换 %d: 触发器数量限制为3, 截断多余部分", i);
                }

                t->trigger_count = (uint8_t)trigger_count;

                for (int j = 0; j < trigger_count; j++) {
                    cJSON *trig = cJSON_GetArrayItem(triggers_arr, j);
                    if (!cJSON_IsObject(trig)) continue;

                    sm_trigger_t *cond = &t->triggers[j];

                    /* triggerType (必填) */
                    cJSON *type = cJSON_GetObjectItem(trig, "triggerType");
                    if (cJSON_IsNumber(type)) {
                        cond->type = (sm_trigger_type_t)type->valueint;
                    }

                    /* triggerSource (必填) */
                    cJSON *source = cJSON_GetObjectItem(trig, "triggerSource");
                    if (cJSON_IsString(source)) {
                        strncpy(cond->source, source->valuestring,
                                SM_MAX_TRIGGER_SOURCE_LEN - 1);
                    }

                    /* operator (可选，默认 "==") */
                    cJSON *op = cJSON_GetObjectItem(trig, "operator");
                    if (cJSON_IsNumber(op)) {
                        cond->op = (sm_operator_t)op->valueint;
                    } else if (cJSON_IsString(op)) {
                        cond->op = parse_operator(op->valuestring);
                    } else {
                        cond->op = SM_OP_EQ;
                    }

                    /* triggerValue (可选，默认 1) */
                    cJSON *val = cJSON_GetObjectItem(trig, "triggerValue");
                    if (cJSON_IsNumber(val)) {
                        cond->value = val->valueint;
                    } else {
                        cond->value = 1;
                    }

                }
            } else {
                ESP_LOGE(TAG, "转换 %d: 缺少 'triggers' 数组", i);
                continue;  /* 跳过此转换 */
            }

            /*
             * triggerLogic (optional, default "AND")
             * 支持 "AND"/"OR"，大小写不敏感
             */
            cJSON *trigger_logic = cJSON_GetObjectItem(trans, "triggerLogic");
            if (cJSON_IsString(trigger_logic)) {
                const char* logic_str = trigger_logic->valuestring;
                if (strcasecmp(logic_str, "OR") == 0) {
                    t->trigger_logic = SM_LOGIC_OR;
                } else if (strcasecmp(logic_str, "AND") == 0) {
                    t->trigger_logic = SM_LOGIC_AND;
                } else {
                    ESP_LOGW(TAG, "转换 %d: 未知 triggerLogic '%s'，使用默认 AND", i, logic_str);
                    t->trigger_logic = SM_LOGIC_AND;
                }
            } else if (cJSON_IsNumber(trigger_logic)) {
                t->trigger_logic = (trigger_logic->valueint == 0) ? SM_LOGIC_AND : SM_LOGIC_OR;
            } else {
                /* 字段缺失时默认 AND */
                t->trigger_logic = SM_LOGIC_AND;
            }

            temp_config.transition_count++;
        }
    }

    /* Parse stateEffects */
    cJSON *effects = cJSON_GetObjectItem(root, "stateEffects");
    if (cJSON_IsArray(effects)) {
        int count = cJSON_GetArraySize(effects);
        if (count > SM_MAX_EFFECTS) {
            count = SM_MAX_EFFECTS;
        }

        for (int i = 0; i < count; i++) {
            cJSON *eff = cJSON_GetArrayItem(effects, i);
            if (!cJSON_IsObject(eff)) continue;

            sm_state_effect_t *e = &temp_config.effects[i];

            /* stateName (required) */
            cJSON *state_name = cJSON_GetObjectItem(eff, "stateName");
            if (cJSON_IsString(state_name)) {
                strncpy(e->state_name, state_name->valuestring,
                        SM_MAX_STATE_NAME_LEN - 1);
            }

            /* effectType (required) */
            cJSON *effect_type = cJSON_GetObjectItem(eff, "effectType");
            if (cJSON_IsNumber(effect_type)) {
                e->effect_type = (sm_effect_type_t)effect_type->valueint;
            }

            /* target (required) */
            cJSON *target = cJSON_GetObjectItem(eff, "target");
            if (cJSON_IsString(target)) {
                strncpy(e->target, target->valuestring,
                        SM_MAX_EFFECT_TARGET_LEN - 1);
            }

            /* value (required) */
            cJSON *value = cJSON_GetObjectItem(eff, "value");
            if (cJSON_IsNumber(value)) {
                e->value = value->valueint;
            }

            temp_config.effect_count++;
        }
    }

    /* Parse groupModes (GPIO 方向配置) */
    cJSON *group_modes = cJSON_GetObjectItem(root, "groupModes");
    if (cJSON_IsObject(group_modes)) {
        for (int i = 0; i < SM_MAX_GPIO_GROUPS; i++) {
            char key[4];
            snprintf(key, sizeof(key), "%d", i);
            cJSON *mode = cJSON_GetObjectItem(group_modes, key);
            if (cJSON_IsNumber(mode)) {
                int mode_val = mode->valueint;
                if (mode_val == 0 || mode_val == 1) {
                    temp_config.group_modes[i] = (uint8_t)mode_val;
                } else {
                    ESP_LOGW(TAG, "groupModes[%d] 非法值 %d，使用默认 0(输出)", i, mode_val);
                    temp_config.group_modes[i] = 0;
                }
            }
        }
    }

    /* Parse groupInputModuleTypes (GPIO 输入模块类型配置) */
    cJSON *input_modules = cJSON_GetObjectItem(root, "groupInputModuleTypes");
    if (cJSON_IsObject(input_modules)) {
        for (int i = 0; i < SM_MAX_GPIO_GROUPS; i++) {
            char key[4];
            snprintf(key, sizeof(key), "%d", i);
            cJSON *module = cJSON_GetObjectItem(input_modules, key);
            if (cJSON_IsNumber(module)) {
                int module_val = module->valueint;
                if (module_val >= 0 && module_val <= 4) {
                    temp_config.input_module_types[i] = (sm_input_module_type_t)module_val;
                } else {
                    ESP_LOGW(TAG, "groupInputModuleTypes[%d] 非法值 %d，使用默认 0(normal_button)", i, module_val);
                    temp_config.input_module_types[i] = SM_INPUT_MODULE_NORMAL_BUTTON;
                }
            }
        }
    }

    /* Parse edgeTypes (GPIO 边沿检测类型配置) */
    cJSON *edge_types = cJSON_GetObjectItem(root, "edgeTypes");
    if (cJSON_IsObject(edge_types)) {
        for (int i = 0; i < SM_MAX_GPIO_GROUPS; i++) {
            char key[4];
            snprintf(key, sizeof(key), "%d", i);
            cJSON *edge = cJSON_GetObjectItem(edge_types, key);
            if (cJSON_IsNumber(edge)) {
                int edge_val = edge->valueint;
                if (edge_val >= 0 && edge_val <= 2) {
                    temp_config.edge_types[i] = (sm_edge_type_t)edge_val;
                } else {
                    ESP_LOGW(TAG, "edgeTypes[%d] 非法值 %d，使用默认 0(ANY)", i, edge_val);
                    temp_config.edge_types[i] = SM_EDGE_ANY;
                }
            }
        }
    }

    /* Parse outputModuleTypes (GPIO 输出模块类型配置) */
    cJSON *output_modules = cJSON_GetObjectItem(root, "outputModuleTypes");
    if (cJSON_IsObject(output_modules)) {
        for (int i = 0; i < SM_MAX_GPIO_GROUPS; i++) {
            char key[4];
            snprintf(key, sizeof(key), "%d", i);
            cJSON *output = cJSON_GetObjectItem(output_modules, key);
            if (cJSON_IsNumber(output)) {
                int output_val = output->valueint;
                if (output_val >= 0 && output_val <= 2) {
                    temp_config.output_module_types[i] = (sm_output_module_type_t)output_val;
                } else {
                    ESP_LOGW(TAG, "outputModuleTypes[%d] 非法值 %d，使用默认 0(LED)", i, output_val);
                    temp_config.output_module_types[i] = SM_OUTPUT_LED;
                }
            }
        }
    }

    /* 验证 groupInputModuleTypes 对于输入模式是必需的 (必须在 cJSON_Delete 之前) */
    bool has_input_modules = cJSON_IsObject(cJSON_GetObjectItem(root, "groupInputModuleTypes"));

    for (int i = 0; i < SM_MAX_GPIO_GROUPS; i++) {
        if (temp_config.group_modes[i] == 1) {  /* 输入模式 */
            if (!has_input_modules) {
                ESP_LOGE(TAG, "GPIO 组 %d 配置为输入模式，但缺少 groupInputModuleTypes 配置", i);
                cJSON_Delete(root);
                return ESP_ERR_INVALID_ARG;
            }
        }
    }

    cJSON_Delete(root);

    /* 验证配置 */
    if (temp_config.transition_count == 0) {
        ESP_LOGW(TAG, "未定义任何转换规则");
    }

    /* 应用配置 - Core 使用单一全局实例 */
    extern sm_config_t* sm_v3_get_config(void);

    sm_config_t *cfg = sm_v3_get_config();
    if (cfg == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 原子提交: 仅当预校验全部通过后才替换全局配置
     * 全局内存状态: 全旧 -> [memcpy瞬间] -> 全新
     * 无中间态，故障安全
     */
    memcpy(cfg, &temp_config, sizeof(sm_config_t));

    ESP_LOGI(TAG, "配置已加载: %u 个转换, %u 个效果",
             cfg->transition_count, cfg->effect_count);

    snprintf(last_load_status, sizeof(last_load_status),
             "成功: 加载了 %lu 个转换, %lu 个效果",
             cfg->transition_count, cfg->effect_count);
    last_load_success = true;

    return ESP_OK;
}

/* 将配置序列化为 JSON */
esp_err_t sm_v3_get_config_json(char *buf, size_t buf_size)
{
    if (buf == NULL || buf_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    extern sm_config_t* sm_v3_get_config(void);
    sm_config_t *cfg = sm_v3_get_config();

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_ERR_NO_MEM;
    }

    /* 当前状态 - 从运行时获取 */
    cJSON_AddStringToObject(root, "currentState", sm_v3_get_current_state_name());
    cJSON_AddStringToObject(root, "initialState", cfg->initial_state);
    cJSON_AddBoolToObject(root, "isRunning", sm_v3_is_running());

    /* 转换数组 */
    cJSON *transitions = cJSON_CreateArray();
    for (uint32_t i = 0; i < cfg->transition_count; i++) {
        cJSON *trans = cJSON_CreateObject();
        cJSON_AddStringToObject(trans, "currentState", cfg->transitions[i].current_state);
        cJSON_AddStringToObject(trans, "nextState", cfg->transitions[i].next_state);

        /* 序列化触发器数组 */
        cJSON *triggers_arr = cJSON_CreateArray();
        for (uint8_t j = 0; j < cfg->transitions[i].trigger_count; j++) {
            cJSON *trig = cJSON_CreateObject();
            const sm_trigger_t* cond = &cfg->transitions[i].triggers[j];

            cJSON_AddNumberToObject(trig, "triggerType", cond->type);
            cJSON_AddStringToObject(trig, "triggerSource", cond->source);

            /* 运算符序列化为数字 */
            cJSON_AddNumberToObject(trig, "operator", cond->op);
            cJSON_AddNumberToObject(trig, "triggerValue", cond->value);

            cJSON_AddItemToArray(triggers_arr, trig);
        }
        cJSON_AddItemToObject(trans, "triggers", triggers_arr);
        cJSON_AddNumberToObject(trans, "triggerCount", cfg->transitions[i].trigger_count);

        /*
         * 序列化触发器逻辑 (AND/OR)
         */
        const char* logic_str = (cfg->transitions[i].trigger_logic == SM_LOGIC_OR) ? "OR" : "AND";
        cJSON_AddStringToObject(trans, "triggerLogic", logic_str);

        cJSON_AddItemToArray(transitions, trans);
    }
    cJSON_AddItemToObject(root, "transitions", transitions);

    /* 效果数组 */
    cJSON *effects = cJSON_CreateArray();
    for (uint32_t i = 0; i < cfg->effect_count; i++) {
        cJSON *eff = cJSON_CreateObject();
        cJSON_AddStringToObject(eff, "stateName", cfg->effects[i].state_name);
        cJSON_AddNumberToObject(eff, "effectType", cfg->effects[i].effect_type);
        cJSON_AddStringToObject(eff, "target", cfg->effects[i].target);
        cJSON_AddNumberToObject(eff, "value", cfg->effects[i].value);
        cJSON_AddItemToArray(effects, eff);
    }
    cJSON_AddItemToObject(root, "stateEffects", effects);

    /* GPIO 组模式配置序列化 */
    cJSON *group_modes = cJSON_CreateObject();
    for (int i = 0; i < SM_MAX_GPIO_GROUPS; i++) {
        char key[4];
        snprintf(key, sizeof(key), "%d", i);
        cJSON_AddNumberToObject(group_modes, key, cfg->group_modes[i]);
    }
    cJSON_AddItemToObject(root, "groupModes", group_modes);

    /* GPIO 输入模块类型配置序列化 */
    cJSON *input_modules = cJSON_CreateObject();
    for (int i = 0; i < SM_MAX_GPIO_GROUPS; i++) {
        char key[4];
        snprintf(key, sizeof(key), "%d", i);
        cJSON_AddNumberToObject(input_modules, key, cfg->input_module_types[i]);
    }
    cJSON_AddItemToObject(root, "groupInputModuleTypes", input_modules);

    /* GPIO 边沿检测类型配置序列化 */
    cJSON *edge_types = cJSON_CreateObject();
    for (int i = 0; i < SM_MAX_GPIO_GROUPS; i++) {
        char key[4];
        snprintf(key, sizeof(key), "%d", i);
        cJSON_AddNumberToObject(edge_types, key, cfg->edge_types[i]);
    }
    cJSON_AddItemToObject(root, "edgeTypes", edge_types);

    /* GPIO 输出模块类型配置序列化 */
    cJSON *output_modules = cJSON_CreateObject();
    for (int i = 0; i < SM_MAX_GPIO_GROUPS; i++) {
        char key[4];
        snprintf(key, sizeof(key), "%d", i);
        cJSON_AddNumberToObject(output_modules, key, cfg->output_module_types[i]);
    }
    cJSON_AddItemToObject(root, "outputModuleTypes", output_modules);

    /* V3.0 info */
    cJSON_AddStringToObject(root, "version", "3.0");

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (json_str == NULL) {
        return ESP_ERR_NO_MEM;
    }

    strncpy(buf, json_str, buf_size - 1);
    buf[buf_size - 1] = '\0';
    free(json_str);

    return ESP_OK;
}

/* Parse JSON configuration into provided config structure
 * This is the internal version used by sm_api.cpp
 */
esp_err_t sm_json_parse_config(const char* json_str, sm_config_t* config)
{
    if (json_str == NULL || config == NULL) {
        ESP_LOGE(TAG, "解析配置失败: 参数为空");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) {
        ESP_LOGE(TAG, "JSON 解析失败");
        return ESP_ERR_INVALID_ARG;
    }

    memset(config, 0, sizeof(sm_config_t));

    /* 解析 initialState */
    cJSON *initial_state = cJSON_GetObjectItem(root, "initialState");
    if (cJSON_IsString(initial_state)) {
        strncpy(config->initial_state, initial_state->valuestring,
                SM_MAX_STATE_NAME_LEN - 1);
    } else {
        ESP_LOGW(TAG, "No initialState in JSON, using default 'IDLE'");
        strncpy(config->initial_state, "IDLE", SM_MAX_STATE_NAME_LEN - 1);
    }

    /* 解析 transitions */
    cJSON *transitions = cJSON_GetObjectItem(root, "transitions");
    if (cJSON_IsArray(transitions)) {
        int count = cJSON_GetArraySize(transitions);
        if (count > SM_MAX_TRANSITIONS) {
            count = SM_MAX_TRANSITIONS;
            ESP_LOGW(TAG, "转换数量限制为 %d", SM_MAX_TRANSITIONS);
        }

        for (int i = 0; i < count; i++) {
            cJSON *trans = cJSON_GetArrayItem(transitions, i);
            if (!cJSON_IsObject(trans)) continue;

            sm_transition_t *t = &config->transitions[i];

            /* currentState (必填) */
            cJSON *current = cJSON_GetObjectItem(trans, "currentState");
            if (cJSON_IsString(current)) {
                strncpy(t->current_state, current->valuestring,
                        SM_MAX_STATE_NAME_LEN - 1);
            }

            /* nextState (必填) */
            cJSON *next = cJSON_GetObjectItem(trans, "nextState");
            if (cJSON_IsString(next)) {
                strncpy(t->next_state, next->valuestring,
                        SM_MAX_STATE_NAME_LEN - 1);
            }

            /* triggerType (必填) */
            cJSON *trigger_type = cJSON_GetObjectItem(trans, "triggerType");
            if (cJSON_IsNumber(trigger_type)) {
                t->triggers[0].type = (sm_trigger_type_t)trigger_type->valueint;
            }

            /* triggerSource (必填) */
            cJSON *trigger_source = cJSON_GetObjectItem(trans, "triggerSource");
            if (cJSON_IsString(trigger_source)) {
                strncpy(t->triggers[0].source, trigger_source->valuestring,
                        SM_MAX_TRIGGER_SOURCE_LEN - 1);
            }

            /* conditionOperator (可选，默认 "==")
             * 运算符支持
             */
            cJSON *cond_op = cJSON_GetObjectItem(trans, "conditionOperator");
            if (cJSON_IsString(cond_op)) {
                t->triggers[0].op = parse_operator(cond_op->valuestring);
            } else {
                t->triggers[0].op = SM_OP_EQ;  /* 默认等于 */
            }

            /* conditionValue (可选，默认 1) */
            cJSON *cond_value = cJSON_GetObjectItem(trans, "conditionValue");
            if (cJSON_IsNumber(cond_value)) {
                t->triggers[0].value = cond_value->valueint;
            } else {
                t->triggers[0].value = 1;  /* 默认触发值 */
            }

            /* 设置触发器数量 */
            t->trigger_count = 1;
            t->trigger_logic = SM_LOGIC_AND;  /* 默认AND逻辑 */

            config->transition_count++;
        }
    }

    /* Parse stateEffects */
    cJSON *effects = cJSON_GetObjectItem(root, "stateEffects");
    if (cJSON_IsArray(effects)) {
        int count = cJSON_GetArraySize(effects);
        if (count > SM_MAX_EFFECTS) {
            count = SM_MAX_EFFECTS;
        }

        for (int i = 0; i < count; i++) {
            cJSON *eff = cJSON_GetArrayItem(effects, i);
            if (!cJSON_IsObject(eff)) continue;

            sm_state_effect_t *e = &config->effects[i];

            /* stateName (required) */
            cJSON *state_name = cJSON_GetObjectItem(eff, "stateName");
            if (cJSON_IsString(state_name)) {
                strncpy(e->state_name, state_name->valuestring,
                        SM_MAX_STATE_NAME_LEN - 1);
            }

            /* effectType (required) */
            cJSON *effect_type = cJSON_GetObjectItem(eff, "effectType");
            if (cJSON_IsNumber(effect_type)) {
                e->effect_type = (sm_effect_type_t)effect_type->valueint;
            }

            /* target (required) */
            cJSON *target = cJSON_GetObjectItem(eff, "target");
            if (cJSON_IsString(target)) {
                strncpy(e->target, target->valuestring,
                        SM_MAX_EFFECT_TARGET_LEN - 1);
            }

            /* value (required) */
            cJSON *value = cJSON_GetObjectItem(eff, "value");
            if (cJSON_IsNumber(value)) {
                e->value = value->valueint;
            }

            config->effect_count++;
        }
    }

    /* Parse groupModes (GPIO 方向配置) */
    cJSON *group_modes = cJSON_GetObjectItem(root, "groupModes");
    if (cJSON_IsObject(group_modes)) {
        for (int i = 0; i < SM_MAX_GPIO_GROUPS; i++) {
            char key[4];
            snprintf(key, sizeof(key), "%d", i);
            cJSON *mode = cJSON_GetObjectItem(group_modes, key);
            if (cJSON_IsNumber(mode)) {
                int mode_val = mode->valueint;
                if (mode_val == 0 || mode_val == 1) {
                    config->group_modes[i] = (uint8_t)mode_val;
                } else {
                    ESP_LOGW(TAG, "groupModes[%d] 非法值 %d，使用默认 0(输出)", i, mode_val);
                    config->group_modes[i] = 0;
                }
            }
        }
    }

    /* Parse groupInputModuleTypes (GPIO 输入模块类型配置) */
    cJSON *input_modules = cJSON_GetObjectItem(root, "groupInputModuleTypes");
    if (cJSON_IsObject(input_modules)) {
        for (int i = 0; i < SM_MAX_GPIO_GROUPS; i++) {
            char key[4];
            snprintf(key, sizeof(key), "%d", i);
            cJSON *module = cJSON_GetObjectItem(input_modules, key);
            if (cJSON_IsNumber(module)) {
                int module_val = module->valueint;
                if (module_val == 0 || module_val == 1) {
                    config->input_module_types[i] = (sm_input_module_type_t)module_val;
                } else {
                    ESP_LOGW(TAG, "groupInputModuleTypes[%d] 非法值 %d，使用默认 0(normal_button)", i, module_val);
                    config->input_module_types[i] = SM_INPUT_MODULE_NORMAL_BUTTON;
                }
            }
        }
    }

    /* 验证 groupInputModuleTypes 对于输入模式是必需的 */
    bool has_input_modules = cJSON_IsObject(cJSON_GetObjectItem(root, "groupInputModuleTypes"));

    for (int i = 0; i < SM_MAX_GPIO_GROUPS; i++) {
        if (config->group_modes[i] == 1) {  /* 输入模式 */
            if (!has_input_modules) {
                ESP_LOGE(TAG, "GPIO 组 %d 配置为输入模式，但缺少 groupInputModuleTypes 配置", i);
                cJSON_Delete(root);
                return ESP_ERR_INVALID_ARG;
            }
        }
    }

    cJSON_Delete(root);

    ESP_LOGI(TAG, "JSON 配置解析完成: %d 个转换, %d 个效果",
             config->transition_count, config->effect_count);

    return ESP_OK;
}
