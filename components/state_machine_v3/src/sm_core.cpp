/*
 * 状态机 V3.0 - Core 引擎
 */

extern "C" {
#include <string.h>
#include <stdlib.h>
#include "sm_types.h"
#include "sm_core.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "gpio_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

static const char* TAG = "SM_CORE";

/* 单一全局实例 - 运行时状态 */
static struct {
    sm_config_t config;                          // 配置快照
    char current_state[SM_MAX_STATE_NAME_LEN]; // 当前状态
    bool is_running;                            // 运行标志
    sm_timer_manager_t timer_mgr;               // 定时器状态管理（按用途分离）
    uint8_t active_timer_count;
    uint8_t gpio_prev_state[SM_MAX_GPIO_GROUPS];      // GPIO 历史状态
    bool manual_trigger_pending;                // 手动触发待处理标志
} g_sm;

static bool g_initialized = false;

/* 内部函数声明 */
static bool process_transitions(void);
static bool execute_effects(const char* state_name);
static bool check_triggers(const sm_transition_t* trans);

/* GPIO 函数声明 */
int sm_gpio_read_logical(int group_id);
void sm_gpio_set_output(int group_id, int value);
void sm_gpio_set_motor(int motor_id, int8_t speed_direction);

/* 从字符串解析验证id (P1->1, T0->0, M0->0) */
int sm_parse_id(const char *str, char prefix, int max_count)
{
    if (str == NULL || str[0] == '\0') {
        ESP_LOGW(TAG, "无效的ID字符串: %s", str);
        return -1;
    }

    if (str[0] != prefix) {
        ESP_LOGW(TAG, "ID前缀不匹配: 期望%c, 实际%c", prefix, str[0]);
        return -1;
    }

    char *endptr;
    long id = strtol(str + 1, &endptr, 10);
    if (*endptr != '\0' || id < 0 || id >= max_count) {
        ESP_LOGW(TAG, "无效的ID数值: %s", str);
        return -1;
    }

    return (int)id;
}

/*
 * 检查单个触发器
 */
static bool check_single_trigger(const sm_trigger_t* trigger)
{
    switch (trigger->type) {
        case SM_TRIGGER_EDGE: {
            /* 解析 GPIO 组 ID */
            int group_id = sm_parse_id(trigger->source, 'P', SM_MAX_GPIO_GROUPS);
            if (group_id == -1) {
                ESP_LOGW(TAG, "无效的 GPIO 组: %s", trigger->source);
                return false;
            }

            /* 从集中配置获取边沿类型 */
            sm_edge_type_t edge_type = g_sm.config.edge_types[group_id];

            /* 读取当前状态 */
            int current_state = sm_gpio_read_logical(group_id);
            if (current_state < 0) {
                return false; /* 读取失败 */
            }

            uint8_t prev = g_sm.gpio_prev_state[group_id];
            uint8_t curr = (current_state != 0) ? 1 : 0;

            /* 检测边沿 */
            bool edge_detected = false;

            if (prev != curr) {
                /* 状态发生变化 */
                switch (edge_type) {
                    case SM_EDGE_ANY:
                        edge_detected = true; /* 任何边沿 */
                        break;
                    case SM_EDGE_RISING:
                        edge_detected = (prev == 0 && curr == 1); /* 上升沿 */
                        break;
                    case SM_EDGE_FALLING:
                        edge_detected = (prev == 1 && curr == 0); /* 下降沿 */
                        break;
                    default:
                        edge_detected = false;
                        break;
                }

                if (edge_detected) {
                    ESP_LOGD(TAG, "组 %d 检测到边沿: %s", group_id,
                            (edge_type == SM_EDGE_RISING) ? "上升" :
                            (edge_type == SM_EDGE_FALLING) ? "下降" : "任意");
                }
            }

            /* 更新 g_sm 中的历史状态 */
            g_sm.gpio_prev_state[group_id] = curr;

            return edge_detected;
        }

        case SM_TRIGGER_LEVEL: {
            /* 解析 GPIO 组 ID */
            int group_id = sm_parse_id(trigger->source, 'P', SM_MAX_GPIO_GROUPS);
            if (group_id == -1) {
                ESP_LOGW(TAG, "无效的 GPIO 组: %s", trigger->source);
                return false;
            }

            int input_value = sm_gpio_read_logical(group_id);
            if (input_value < 0) {
                return false;
            }

            /* op 字段仅用于 LEVEL 触发器的比较运算符 (< > == 等) */
            switch (trigger->op) {
                case SM_OP_EQ: return input_value == trigger->value;
                case SM_OP_LT: return input_value < trigger->value;
                case SM_OP_GT: return input_value > trigger->value;
                case SM_OP_LE: return input_value <= trigger->value;
                case SM_OP_GE: return input_value >= trigger->value;
                case SM_OP_NE: return input_value != trigger->value;
                default:       return false;
            }
        }

        case SM_TRIGGER_TIMER: {
            /* 解析 timer 组 ID */
            int idx = sm_parse_id(trigger->source, 'T', SM_MAX_TIMERS);
            if (idx == -1) {
                ESP_LOGW(TAG, "无效的 Timer ID: %s", trigger->source);
                return false;
            }

            sm_timer_state_t* timer = &g_sm.timer_mgr.timers[idx];

            /* 检查定时器是否由 SM_EFFECT_TIMER 启动 */
            if (!timer->running) {
                ESP_LOGD(TAG, "触发器: 定时器 T%d 未启动", idx);
                return false;
            }

            /* 检查是否到期（与 duration_ms 比较） */
            uint32_t now_ms = esp_timer_get_time() / 1000;
            uint32_t elapsed = now_ms - timer->start_ms;

            ESP_LOGI(TAG, "定时器 T%d 检查详情: now_ms=%lu, start_ms=%lu, elapsed=%lu, duration_ms=%lu, running=%s",
                     idx, now_ms, timer->start_ms, elapsed, timer->duration_ms, timer->running ? "true" : "false");

            if (elapsed >= timer->duration_ms) {
                timer->running = false;  /* 到期后停止 */
                g_sm.active_timer_count--;
                ESP_LOGI(TAG, "触发器: 定时器 T%d 到期 (%lu ms)", idx, elapsed);
                return true;
            }

            ESP_LOGD(TAG, "触发器: 定时器 T%d 运行中 (%lu/%lu ms)",
                     idx, elapsed, timer->duration_ms);
            return false;
        }

        case SM_TRIGGER_MANUAL:
            /* 检查是否有待处理的手动触发信号 */
            if (g_sm.manual_trigger_pending) {
                g_sm.manual_trigger_pending = false;  // 清除标志
                ESP_LOGD(TAG, "手动触发检查通过");
                return true;
            }
            ESP_LOGD(TAG, "手动触发检查: 无待处理信号");
            return false;

        default:
            return false;
    }
}

/*
 * 检查所有触发器 (支持 AND/OR 逻辑)
 */
static bool check_triggers(const sm_transition_t* trans)
{
    /* 边界检查: 无触发器视为无效 */
    if (trans->trigger_count == 0 || trans->trigger_count > 3) {
        return false;
    }

    /* OR 逻辑: 任一触发器满足即成功 (默认是 AND) */
    if (trans->trigger_logic == SM_LOGIC_OR) {
        for (uint8_t i = 0; i < trans->trigger_count; i++) {
            if (check_single_trigger(&trans->triggers[i])) {
                return true;  /* 一个满足就成功 */
            }
        }
        return false;  /* 全部不满足才失败 */
    }

    /* AND 逻辑 (默认): 所有触发器都必须满足 */
    for (uint8_t i = 0; i < trans->trigger_count; i++) {
        if (!check_single_trigger(&trans->triggers[i])) {
            return false;  /* 一个不满足就失败 */
        }
    }

    return true;  /* 全部满足 */
}

/*
 * 执行单个效果
 */
static void execute_single_effect(const sm_state_effect_t* effect)
{
    switch (effect->effect_type) {
        case SM_EFFECT_OUTPUT: {
            int group_id = sm_parse_id(effect->target, 'P', SM_MAX_GPIO_GROUPS);
            if (group_id == -1) {
                ESP_LOGW(TAG, "无效的 GPIO 组: %s", effect->target);
                return;
            }

            /* 从集中配置获取输出模块类型 */
            sm_output_module_type_t module_type = g_sm.config.output_module_types[group_id];
            int output_value = effect->value;

            /* 根据输出模块类型解释效果值 */
            switch (module_type) {
                case SM_OUTPUT_LED:
                    /* LED: 0=关，1=开 */
                    output_value = (effect->value != 0) ? 1 : 0;
                    break;

                case SM_OUTPUT_TRAFFIC_LIGHT:
                    /* 交通灯: 0=灭，1=红，2=黄，3=绿 */
                    if (effect->value < 0 || effect->value > 3) {
                        ESP_LOGW(TAG, "警告: 交通灯效果值 %d 非法，使用 0(灭)", effect->value);
                        output_value = 0;
                    } else {
                        output_value = effect->value;
                    }
                    break;

                case SM_OUTPUT_FAN:
                    /* 风扇: 0=关，非0=开 */
                    output_value = (effect->value != 0) ? 1 : 0;
                    break;

                default:
                    /* 未知类型，按 LED 处理 */
                    output_value = (effect->value != 0) ? 1 : 0;
                    break;
            }

            sm_gpio_set_output(group_id, output_value);
            ESP_LOGI(TAG, "效果: 设置 %s = %d (模块类型=%d, 原始值=%d)",
                     effect->target, output_value, module_type, effect->value);
            break;
        }

        case SM_EFFECT_MOTOR: {
            int motor_id = sm_parse_id(effect->target, 'M', SM_MAX_MOTORS);
            if (motor_id == -1) {
                ESP_LOGW(TAG, "无效的 MOTOR 组: %s", effect->target);
                return;
            }
            sm_gpio_set_motor(motor_id, (int8_t)effect->value);
            ESP_LOGI(TAG, "效果: 设置电机 %s = %d (档位)", effect->target, effect->value);
            break;
        }

        case SM_EFFECT_TIMER: {
            /* 解析 timer 组 ID */
            int idx = sm_parse_id(effect->target, 'T', SM_MAX_TIMERS);
            if (idx >= SM_MAX_TIMERS) idx = 0;

            sm_timer_state_t* timer = &g_sm.timer_mgr.timers[idx];
            uint32_t now_ms = esp_timer_get_time() / 1000;

            /* value 字段：持续时间（单位：毫秒， 默认1000毫秒） */
            uint32_t duration_ms = (effect->value > 0) ? (uint32_t)effect->value : 1000;

            timer->start_ms = now_ms;
            timer->running = true;
            timer->duration_ms = duration_ms;
            g_sm.active_timer_count++;

            ESP_LOGI(TAG, "效果: 启动定时器 T%d，持续时间 %lu 毫秒 (%lu ms)",
                     idx, duration_ms, timer->duration_ms);
            ESP_LOGI(TAG, "定时器 T%d 启动后状态: running=%s, start_ms=%lu, duration_ms=%lu, 活跃定时器数=%d",
                     idx, timer->running ? "true" : "false", timer->start_ms, timer->duration_ms, g_sm.active_timer_count);
            break;
        }

        case SM_EFFECT_NONE:
            /* 无效果，直接返回 */
            break;

        default:
            break;
    }
}

/*
 * 执行效果 (使用状态名称查找)
 */
static bool execute_effects(const char* state_name)
{
    /* 通过遍历查找匹配的索引 */
    for (size_t i = 0; i < g_sm.config.effect_count; i++) {
        const sm_state_effect_t* effect = &g_sm.config.effects[i];

        if (strcmp(effect->state_name, state_name) == 0) {
            execute_single_effect(effect);
        }
    }

    return true;
}

/*
 * 处理状态转换
 */
static bool process_transitions(void)
{
    if (!g_sm.is_running) {
        return true;
    }

    for (size_t i = 0; i < g_sm.config.transition_count; i++) {
        const sm_transition_t* trans = &g_sm.config.transitions[i];

        /* 检查当前状态匹配 */
        if (strcmp(trans->current_state, g_sm.current_state) != 0) {
            continue;
        }

        /* 检查触发器 */
        if (check_triggers(trans)) {
            /* 执行状态转换 */
            strncpy(g_sm.current_state, trans->next_state, SM_MAX_STATE_NAME_LEN - 1);
            g_sm.current_state[SM_MAX_STATE_NAME_LEN - 1] = '\0';

            ESP_LOGI(TAG, "状态转换: %s -> %s (%d个触发器满足)",
                     trans->current_state, trans->next_state, trans->trigger_count);

            execute_effects(g_sm.current_state);

            /* 每个周期只处理一个转换 */
            break;
        }
    }

    return true;
}



/*
 * 读取 GPIO 逻辑电平
 */
int sm_gpio_read_logical(int group_id)
{
    /* 通过 gpio_manager 获取 GPIO 组信息 */
    gpio_group_t group_info;
    esp_err_t err = gpio_manager_get_group_info((uint8_t)group_id, &group_info);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "获取 GPIO 组 %d 信息失败: %d", group_id, err);
        return -1;
    }

    // 读取物理电平 (组内第一个引脚作为代表)
    int physical_level = group_info.states[0];

    // 根据模块类型进行转换
    sm_input_module_type_t module_type = g_sm.config.input_module_types[group_id];
    int logical_level;

    switch (module_type) {
        case SM_INPUT_MODULE_NORMAL_BUTTON:
            /* 普通按键：默认高电平，触发时低电平 */
            logical_level = (physical_level == 0) ? 1 : 0;
            break;

        case SM_INPUT_MODULE_LED_BUTTON:
            /* 带灯按键：默认低电平，触发时高电平 */
            logical_level = (physical_level == 1) ? 1 : 0;
            break;

        case SM_INPUT_MODULE_TEMP_SENSOR: {
            /* 温度传感器：将 ADC 值转换为温度（0.1°C 精度）
                * 示例：使用 TMP36 传感器公式
                * ADC 范围：0-4095（12位）对应 0-3.3V
                * TMP36：Vout = 500mV + (T°C × 10mV)
                * 因此：T = (V - 0.5) × 100，单位 0.1°C
                */
            int adc_raw = physical_level;  /* ADC 原始值（0-4095） */

            /* 将 ADC 转换为电压（毫伏） */
            /* 3300mV / 4095 ≈ 0.8059 mV/LSB */
            int voltage_mv = (adc_raw * 3300) / 4095;

            /* TMP36：500mV 对应 0°C，每 10mV 对应 1°C */
            /* 温度（0.1°C）= (电压mV - 500) × 10 / 10 = (电压mV - 500) */
            logical_level = voltage_mv - 500;

            ESP_LOGD(TAG, "温度传感器: ADC=%d, 电压=%dmV, 温度=%d.%d°C",
                        adc_raw, voltage_mv, logical_level / 10, logical_level % 10);
            break;
        }

        case SM_INPUT_MODULE_HUMIDITY_SENSOR:
        case SM_INPUT_MODULE_VOLTAGE_MONITOR:
        default:
            /* 默认或未实现类型：直接返回物理电平 */
            logical_level = physical_level;
            break;
    }

    ESP_LOGD(TAG, "GPIO组%d 物理电平=%d 逻辑电平=%d (模块类型=%d)",
             group_id, physical_level, logical_level, module_type);

    return logical_level;
}

/*
 * 设置 GPIO 输出
 */
void sm_gpio_set_output(int group_id, int value)
{
    // 根据模块类型进行转换
    sm_output_module_type_t module_type = g_sm.config.output_module_types[group_id];

    switch (module_type) {
        case SM_OUTPUT_TRAFFIC_LIGHT: {
            /* 交通灯: 使用两个IO引脚的高低位组合来表示 0=灭, 1=红, 2=黄, 3=绿
             * 0: 低位=0, 高位=0 (灭)
             * 1: 低位=1, 高位=0 (红)
             * 2: 低位=0, 高位=1 (黄)
             * 3: 低位=1, 高位=1 (绿)
             */
            int low_bit = value & 0x01;           /* 取最低位 */
            int high_bit = (value >> 1) & 0x01;   /* 取第二位 */

            esp_err_t err1 = gpio_manager_set_level(group_id, 0, low_bit);
            esp_err_t err2 = gpio_manager_set_level(group_id, 1, high_bit);

            if (err1 == ESP_OK && err2 == ESP_OK) {
                ESP_LOGI(TAG, "交通灯组 %d 设置为 %d (低位=%d, 高位=%d)",
                         group_id, value, low_bit, high_bit);
            } else {
                ESP_LOGW(TAG, "交通灯组 %d 写入失败: %d, %d", group_id, err1, err2);
            }
            break;
        }
        case SM_OUTPUT_LED:
        case SM_OUTPUT_FAN: {
            /* LED灯和风扇: 0=关, 1=开，控制低位IO */
            uint32_t level = (value != 0) ? 1 : 0;
            esp_err_t err = gpio_manager_set_level((uint8_t)group_id, 0, level);

            if (err == ESP_OK) {
                ESP_LOGI(TAG, "输出组 %d (%s) 设置为 %d",
                         group_id,
                         module_type == SM_OUTPUT_LED ? "LED" : "FAN",
                         value);
            } else {
                ESP_LOGW(TAG, "输出组 %d 写入失败: %d", group_id, err);
            }
            break;
        }

        default: {
            /* 默认处理：直接设置低位 */
            uint32_t level = (value != 0) ? 1 : 0;
            esp_err_t err = gpio_manager_set_level((uint8_t)group_id, 0, level);

            if (err == ESP_OK) {
                ESP_LOGI(TAG, "输出组 %d (类型=%d) 设置为 %d", group_id, module_type, value);
            } else {
                ESP_LOGW(TAG, "输出组 %d 写入失败: %d", group_id, err);
            }
            break;
        }
    }
}

/* 设置电机速度 - 复用 gpio_manager
 * 参数: speed_direction (-9~9 档位, 0=停止, 1-9=正转, -1~-9=反转)
 */
void sm_gpio_set_motor(int motor_id, int8_t speed_direction)
{
    /* 验证档位范围 */
    if (speed_direction < -9 || speed_direction > 9) {
        ESP_LOGW(TAG, "电机档位超出范围: %d (有效范围: -9~9)", speed_direction);
        return;
    }

    /* 复用 gpio_manager 设置电机 */
    esp_err_t err = gpio_manager_set_motor((uint8_t)motor_id, speed_direction);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "电机 %d 设置失败: %d", motor_id, err);
        return;
    }

    ESP_LOGI(TAG, "电机 %d 档位设置为 %d", motor_id, speed_direction);
}


/*
 * 公共 API 实现
 */

extern "C" {

esp_err_t sm_v3_init(void)
{
    sm_config_t config;
    memset(&config, 0, sizeof(config));

    memset(&g_sm, 0, sizeof(g_sm));
    memcpy(&g_sm.config, &config, sizeof(sm_config_t));

    g_initialized = true;
    return ESP_OK;
}

esp_err_t sm_v3_start(void)
{
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    g_sm.is_running = true;

    // 检查 initial_state 是否为空，使用默认值
    if (g_sm.config.initial_state[0] == '\0') {
        ESP_LOGW(TAG, "initial_state empty, using default 'IDLE'");
        strncpy(g_sm.current_state, "IDLE", SM_MAX_STATE_NAME_LEN - 1);
    } else {
        strncpy(g_sm.current_state, g_sm.config.initial_state, SM_MAX_STATE_NAME_LEN - 1);
    }
    g_sm.current_state[SM_MAX_STATE_NAME_LEN - 1] = '\0';

    execute_effects(g_sm.current_state);

    return ESP_OK;
}

/* 状态机驱动任务 - 100ms周期 */
static void sm_v3_loop(void* arg)
{
    (void)arg;
    const TickType_t delay = pdMS_TO_TICKS(100);  // 100ms 周期
    while (1) {
        sm_v3_process();  // 执行一次状态机处理
        vTaskDelay(delay);
    }
}

esp_err_t sm_v3_start_auto(void)
{
    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // 先启动状态机
    esp_err_t err = sm_v3_start();
    if (err != ESP_OK) {
        return err;
    }

    // 创建驱动任务
    BaseType_t task_created = xTaskCreate(sm_v3_loop, "sm_v3", 2048, NULL, 5, NULL);
    if (task_created != pdPASS) {
        sm_v3_stop();  // 启动失败，回滚
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "状态机驱动任务已启动 (100ms周期)");
    return ESP_OK;
}

void sm_v3_stop(void)
{
    if (!g_initialized) {
        return;
    }

    g_sm.is_running = false;
    memset(&g_sm.timer_mgr, 0, sizeof(g_sm.timer_mgr));
    g_sm.active_timer_count = 0;
}


void sm_v3_reset(void)
{
    if (!g_initialized) {
        return;
    }

    sm_v3_stop();

    memset(g_sm.gpio_prev_state, 0, sizeof(g_sm.gpio_prev_state));
    g_sm.manual_trigger_pending = false;  // 清除手动触发标志
    strncpy(g_sm.current_state, g_sm.config.initial_state, SM_MAX_STATE_NAME_LEN - 1);
    g_sm.current_state[SM_MAX_STATE_NAME_LEN - 1] = '\0';
}

void sm_v3_process(void)
{
    if (!g_initialized || !g_sm.is_running) {
        ESP_LOGD(TAG, "sm_v3_process() 跳过: initialized=%s, running=%s",
                 g_initialized ? "true" : "false", g_sm.is_running ? "true" : "false");
        return;
    }

    ESP_LOGD(TAG, "sm_v3_process() 调用: 当前状态=%s, 活跃定时器数=%d",
             g_sm.current_state, g_sm.active_timer_count);

    process_transitions();
}

esp_err_t sm_v3_get_current_state(char* buf, size_t buf_size)
{
    if (buf == NULL || buf_size == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!g_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    strncpy(buf, g_sm.current_state, buf_size);
    buf[buf_size - 1] = '\0';
    return ESP_OK;
}

const char* sm_v3_get_current_state_name(void)
{
    if (!g_initialized) {
        return "";
    }
    return g_sm.current_state;
}

esp_err_t sm_v3_manual_trigger(const char* target_state)
{
    if (!g_initialized || target_state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* 设置手动触发标志，让触发器检查机制处理状态转换 */
    g_sm.manual_trigger_pending = true;

    ESP_LOGI(TAG, "手动触发信号已设置，目标状态: %s", target_state);

    /* 立即处理一次状态转换检查 */
    process_transitions();

    return ESP_OK;
}


void sm_v3_monitor_callback(void* arg)
{
    (void)arg;
    sm_v3_process();
}

/* 对外暴露的辅助函数 */
sm_config_t* sm_v3_get_config(void)
{
    if (!g_initialized) {
        return NULL;
    }
    return &g_sm.config;
}

bool sm_v3_is_running(void)
{
    if (!g_initialized) {
        return false;
    }
    return g_sm.is_running;
}

} /* extern "C" */
