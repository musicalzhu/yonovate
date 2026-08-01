/* 状态机模块实现文件 */
// 系统头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <ctype.h>

// 定义常量
#define TAG "state_machine"

// ESP-IDF 头文件
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

// 自定义组件头文件
#include "include/state_machine.h"
#include "../gpio_manager/include/gpio_manager.h"
#include "../motor_control/include/motor_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// 函数声明
static esp_err_t apply_state_effects(const char *state_name);
static void lock_state_machine_config(void);
static void unlock_state_machine_config(void);
static esp_err_t handle_output_effect(const state_effect_t *effect);
static void cleanup_all_timers(void);
void state_machine_monitor_callback(void *arg);
static void timer_callback(void *arg);

// 全局变量存储状态机配置
state_machine_config_t g_state_machine_config;

esp_timer_handle_t state_machine_timer = NULL;
esp_timer_handle_t state_machine_monitor_timer = NULL;

// 临界区保护，用于在任务和中断上下文中安全访问共享数据
portMUX_TYPE g_state_machine_spinlock = portMUX_INITIALIZER_UNLOCKED;

// 初始化状态机函数已经在文件开头定义，此处删除重复定义

// 锁定状态机配置 - 支持任务和中断上下文
static void lock_state_machine_config(void)
{
    if (xPortInIsrContext()) {
        // 在中断上下文中直接使用自旋锁
        portENTER_CRITICAL_ISR(&g_state_machine_spinlock);
    } else {
        // 在任务上下文中使用自旋锁
        portENTER_CRITICAL(&g_state_machine_spinlock);
    }
}

// 解锁状态机配置 - 支持任务和中断上下文
static void unlock_state_machine_config(void)
{
    if (xPortInIsrContext()) {
        // 在中断上下文中直接释放自旋锁
        portEXIT_CRITICAL_ISR(&g_state_machine_spinlock);
    } else {
        // 在任务上下文中释放自旋锁
        portEXIT_CRITICAL(&g_state_machine_spinlock);
    }
}

// 清理所有定时器
static void cleanup_all_timers(void)
{
    // 创建临时数组保存需要删除的定时器句柄
    esp_timer_handle_t timer_handles[MAX_STATE_EFFECTS] = {NULL};
    uint32_t valid_timer_count = 0;
    
    lock_state_machine_config();
    
    // 收集所有有效的定时器句柄
    for (uint32_t i = 0; i < g_state_machine_config.timer_manager.timer_count; i++) {
        state_machine_timer_t *timer = &g_state_machine_config.timer_manager.timers[i];
        
        if (timer->handle != NULL) {
            timer_handles[valid_timer_count] = timer->handle;
            timer->handle = NULL; // 标记为已处理
            valid_timer_count++;
        }
    }
    
    // 重置定时器计数
    g_state_machine_config.timer_manager.timer_count = 0;
    
    unlock_state_machine_config();
    
    // 在锁外安全地删除定时器，避免回调执行时的竞争条件
    for (uint32_t i = 0; i < valid_timer_count; i++) {
        if (timer_handles[i] != NULL) {
            esp_err_t err = esp_timer_delete(timer_handles[i]);
            if (err != ESP_OK) {
                // 删除失败，但继续处理其他定时器
            }
        }
    }
}

/**
 * 将物理GPIO电平转换为统一的逻辑电平
 * @param physical_state 物理电平状态 (0=低电平, 1=高电平)
 * @param module_type 模块类型
 * @return 逻辑电平 (0=未触发, 1=已触发)
 */
static inline int physical_to_logical_level(int physical_state, module_type_t module_type)
{
    switch (module_type) {
        case MODULE_TYPE_NORMAL_BUTTON:
            // 普通按键: 默认高电平(1)，按下时低电平(0)
            // 逻辑转换: 物理0→逻辑1, 物理1→逻辑0
            return (physical_state == 0) ? 1 : 0;
            
        case MODULE_TYPE_LED_BUTTON:
            // 带灯按键: 默认低电平(0)，按下时高电平(1)
            // 逻辑转换: 物理0→逻辑0, 物理1→逻辑1 (直接映射)
            return physical_state;
            
        default:
            // 默认按普通按键处理
            return (physical_state == 0) ? 1 : 0;
    }
}

// 更新状态机配置
esp_err_t state_machine_update_config(const state_machine_config_t *config)
{
    if (config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 检查配置数据是否有效
    if (config->transition_count > config->transition_capacity || 
        config->effect_count > config->effect_capacity) {
        return ESP_ERR_INVALID_SIZE;
    }
    
    // 记录是否应该在更新后自动启动
    lock_state_machine_config();
    bool should_auto_start = g_state_machine_config.is_running;
    unlock_state_machine_config();
    
    // 停止状态机（内部会清理所有定时器）
    state_machine_stop();
    
    // 更新配置
    lock_state_machine_config();
    
    // 复制状态转移规则
    g_state_machine_config.transition_count = config->transition_count;
    memcpy(g_state_machine_config.transitions, config->transitions, 
           sizeof(state_transition_t) * config->transition_count);
    
    // 复制状态效果
    g_state_machine_config.effect_count = config->effect_count;
    memcpy(g_state_machine_config.state_effects, config->state_effects, 
           sizeof(state_effect_t) * config->effect_count);
            
    // 复制初始状态
    strncpy(g_state_machine_config.initial_state, config->initial_state, 
            sizeof(g_state_machine_config.initial_state) - 1);
    
    // 复制GPIO组模式配置
    memcpy(g_state_machine_config.group_modes, config->group_modes, 
           sizeof(g_state_machine_config.group_modes));
    
    // 复制GPIO组模块类型配置
    memcpy(g_state_machine_config.group_modules, config->group_modules,
           sizeof(g_state_machine_config.group_modules));
    
    unlock_state_machine_config();
    
    // 应用GPIO组模式配置 - 在锁外执行，避免长时间占用临界区
    // 使用非阻塞方式或适当的超时处理I2C操作
    for (int group_id = 0; group_id < GPIO_GROUP_COUNT; group_id++) {
        for (int pin_index = 0; pin_index < GPIO_PER_GROUP; pin_index++) {
            if (gpio_manager_set_pin_mode(group_id, pin_index, config->group_modes[group_id][pin_index]) != ESP_OK) {
                //ESP_LOGE(TAG, "设置GPIO组%d引脚%d模式失败", group_id, pin_index);
            } else {
                //ESP_LOGI(TAG, "设置GPIO组%d引脚%d为%s模式", group_id, pin_index, 
                //         config->group_modes[group_id][pin_index] ? "输入" : "输出");
            }
            // 短暂延时，避免I2C操作过于密集
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    
    // 所有GPIO模式设置完成后，等待GPIO状态完全稳定
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // 初始化所有输入GPIO的previous_gpio_states，避免误触发
    for (int group_id = 0; group_id < GPIO_GROUP_COUNT; group_id++) {
        // 如果低位IO是输入模式，读取当前状态并初始化previous_gpio_states
        if (config->group_modes[group_id][0]) {  // 低位IO是输入
            gpio_group_t group;
            if (gpio_manager_get_group_info(group_id, &group) == ESP_OK) {
                // 获取当前物理状态
                int current_physical_state = group.states[0];
                // 转换为逻辑状态
                module_type_t module_type = config->group_modules[group_id];
                int current_logical_state = physical_to_logical_level(current_physical_state, module_type);
                // 初始化previous状态，防止同步配置时误触发
                lock_state_machine_config();
                g_state_machine_config.previous_gpio_states[group_id] = current_logical_state;
                unlock_state_machine_config();
                
                ESP_LOGI(TAG, "初始化GPIO组%d: 物理=%d, 逻辑=%d, 模块类型=%d", 
                         group_id, current_physical_state, current_logical_state, module_type);
            }
        }
    }
    
    // 如果需要自动启动，则设置当前状态并启动
    if (should_auto_start) {
        lock_state_machine_config();
        strncpy(g_state_machine_config.current_state, config->initial_state, 
                sizeof(g_state_machine_config.current_state) - 1);
        unlock_state_machine_config();
        return state_machine_start(config->initial_state);
    }
    
    return ESP_OK;
}

// 初始化状态机
esp_err_t state_machine_init(void)
{
    ESP_LOGI(TAG, "状态机初始化");
    
    // 初始化配置
    lock_state_machine_config();
    
    // 释放旧的动态内存（如果存在）
    if (g_state_machine_config.transitions != NULL) {
        free(g_state_machine_config.transitions);
        g_state_machine_config.transitions = NULL;
    }
    if (g_state_machine_config.state_effects != NULL) {
        free(g_state_machine_config.state_effects);
        g_state_machine_config.state_effects = NULL;
    }
    if (g_state_machine_config.timer_manager.timers != NULL) {
        free(g_state_machine_config.timer_manager.timers);
        g_state_machine_config.timer_manager.timers = NULL;
    }
    
    memset(&g_state_machine_config, 0, sizeof(state_machine_config_t));
    
    // 分配动态内存
    g_state_machine_config.transitions = (state_transition_t*)malloc(MAX_TRANSITION_RULES * sizeof(state_transition_t));
    g_state_machine_config.state_effects = (state_effect_t*)malloc(MAX_STATE_EFFECTS * sizeof(state_effect_t));
    g_state_machine_config.timer_manager.timers = (state_machine_timer_t*)malloc(MAX_STATE_EFFECTS * sizeof(state_machine_timer_t));
    
    if (g_state_machine_config.transitions == NULL || 
        g_state_machine_config.state_effects == NULL ||
        g_state_machine_config.timer_manager.timers == NULL) {
        ESP_LOGE(TAG, "状态机内存分配失败");
        unlock_state_machine_config();
        return ESP_ERR_NO_MEM;
    }
    
    // 初始化内存
    memset(g_state_machine_config.transitions, 0, MAX_TRANSITION_RULES * sizeof(state_transition_t));
    memset(g_state_machine_config.state_effects, 0, MAX_STATE_EFFECTS * sizeof(state_effect_t));
    memset(g_state_machine_config.timer_manager.timers, 0, MAX_STATE_EFFECTS * sizeof(state_machine_timer_t));
    
    // 设置容量
    g_state_machine_config.transition_capacity = MAX_TRANSITION_RULES;
    g_state_machine_config.effect_capacity = MAX_STATE_EFFECTS;
    g_state_machine_config.timer_manager.timer_capacity = MAX_STATE_EFFECTS;
    
    // 设置初始状态
    strncpy(g_state_machine_config.current_state, "", sizeof(g_state_machine_config.current_state) - 1);
    strncpy(g_state_machine_config.initial_state, "", sizeof(g_state_machine_config.initial_state) - 1);
    
    g_state_machine_config.is_running = false;
    g_state_machine_config.transition_count = 0;
    g_state_machine_config.effect_count = 0;
    
    // 初始化GPIO组模式为默认值（输出模式）
    for (int group_id = 0; group_id < GPIO_GROUP_COUNT; group_id++) {
        for (int pin_index = 0; pin_index < GPIO_PER_GROUP; pin_index++) {
            g_state_machine_config.group_modes[group_id][pin_index] = false; // 默认输出模式
        }
        // 初始化模块类型为默认值（普通按键）
        g_state_machine_config.group_modules[group_id] = MODULE_TYPE_NORMAL_BUTTON;
        // 初始化previous_gpio_states为0（逻辑电平）
        g_state_machine_config.previous_gpio_states[group_id] = 0;
    }
    
    unlock_state_machine_config();
    
    // 清理可能存在的定时器
    cleanup_all_timers();
    
    ESP_LOGI(TAG, "状态机初始化完成");
    return ESP_OK;
}

// 启动状态机
esp_err_t state_machine_start(const char *initial_state)
{
    // 检查初始状态是否设置
    lock_state_machine_config();
    bool is_already_running = g_state_machine_config.is_running;
    bool has_initial_state = (g_state_machine_config.initial_state[0] != '\0') || (initial_state && initial_state[0] != '\0');
    
    // 如果提供了新的初始状态，则更新
    if (initial_state && initial_state[0] != '\0') {
        strncpy(g_state_machine_config.initial_state, initial_state, sizeof(g_state_machine_config.initial_state) - 1);
    }
    unlock_state_machine_config();
    
    if (is_already_running) {
        ESP_LOGW(TAG, "状态机已经在运行中");
        return ESP_OK;
    }
    
    if (!has_initial_state) {
        ESP_LOGE(TAG, "初始状态未设置，无法启动状态机");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 清理所有定时器
    cleanup_all_timers();
    
    // 停止并删除之前可能存在的监控定时器
    if (state_machine_monitor_timer != NULL) {
        esp_timer_delete(state_machine_monitor_timer);
        state_machine_monitor_timer = NULL;
    }
    
    // 设置运行标志并应用初始状态效果
    lock_state_machine_config();
    strncpy(g_state_machine_config.current_state, g_state_machine_config.initial_state, 
            sizeof(g_state_machine_config.current_state) - 1);
    g_state_machine_config.is_running = true;
    unlock_state_machine_config();
    
    ESP_LOGI(TAG, "状态机已启动，初始状态: %s", g_state_machine_config.initial_state);
    
    // 创建并启动监控定时器，每100ms调用一次状态机监视器回调
    esp_timer_create_args_t monitor_timer_args = {
        .callback = &state_machine_monitor_callback,
        .arg = NULL,
        .name = "state_machine_monitor"
    };
    
    esp_err_t err = esp_timer_create(&monitor_timer_args, &state_machine_monitor_timer);
    if (err == ESP_OK) {
        err = esp_timer_start_periodic(state_machine_monitor_timer, 100000); // 100ms = 100000微秒
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "状态机监控定时器已启动，周期100ms");
        } else {
            ESP_LOGE(TAG, "启动状态机监控定时器失败: %s", esp_err_to_name(err));
        }
    } else {
        ESP_LOGE(TAG, "创建状态机监控定时器失败: %s", esp_err_to_name(err));
    }
    
    // 创建任务来应用初始状态效果，避免在可能的阻塞上下文中执行I2C操作
    // 这进一步解决了AW9523的ESP_ERR_TIMEOUT错误
    char *initial_state_copy = (char*)malloc(MAX_STATE_NAME_LEN);
    if (initial_state_copy != NULL) {
        strncpy(initial_state_copy, g_state_machine_config.initial_state, MAX_STATE_NAME_LEN - 1);
        
        // 创建任务来应用状态效果
        xTaskCreate(
            [](void *arg) {
                char *state_name = (char*)arg;
                apply_state_effects(state_name);
                free(state_name);
                vTaskDelete(NULL);
            },
            "apply_state_effects_task",
            4096,  // 足够的栈空间
            initial_state_copy,
            5,     // 中等优先级
            NULL
        );
    }
    
    return err;
}

// 停止状态机
esp_err_t state_machine_stop(void)
{
    lock_state_machine_config();
    bool is_running = g_state_machine_config.is_running;
    unlock_state_machine_config();
    
    if (!is_running) {
        ESP_LOGW(TAG, "状态机已经停止");
        return ESP_OK;
    }
    
    // 停止状态机（不清理配置）
    lock_state_machine_config();
    g_state_machine_config.is_running = false;
    unlock_state_machine_config();
    
    // 清理所有定时器
    cleanup_all_timers();
    
    // 停止并删除监控定时器
    if (state_machine_monitor_timer != NULL) {
        esp_timer_delete(state_machine_monitor_timer);
        state_machine_monitor_timer = NULL;
        ESP_LOGI(TAG, "状态机监控定时器已停止");
    }
    
    ESP_LOGI(TAG, "状态机已停止");
    
    return ESP_OK;
}

// 重置状态机
esp_err_t state_machine_reset(void)
{
    // 停止状态机
    esp_err_t err = state_machine_stop();
    if (err != ESP_OK) {
        return err;
    }
    
    // 重新启动状态机，会应用初始状态效果
    return state_machine_start(g_state_machine_config.initial_state);
}

// 定时器回调函数 - 增强版，确保定时器能正确触发状态转移
static void timer_callback(void* arg)
{
    // 确保参数有效
    if (arg == NULL) {
        // 避免在定时器回调中使用ESP_LOGE
        return;
    }
    
    // 获取定时器ID（不要释放它，因为它在timer_manager中被引用）
    char *timer_id = (char*)arg;
    
    // 查找对应的状态转移规则和定时器信息
    lock_state_machine_config();
    
    // 检查状态机是否在运行
    if (!g_state_machine_config.is_running) {
        unlock_state_machine_config();
        // 注意：不释放timer_id，它会在清理定时器时统一释放
        return;
    }
    
    // 保存当前状态，避免在查找过程中被修改
    char current_state[MAX_STATE_NAME_LEN] = {0};
    strncpy(current_state, g_state_machine_config.current_state, sizeof(current_state) - 1);
    
    // 遍历所有转移规则，查找匹配的定时器ID和当前状态
    bool transition_found = false;
    char next_state[MAX_STATE_NAME_LEN] = {0};
    
    for (uint32_t i = 0; i < g_state_machine_config.transition_count; i++) {
        state_transition_t *transition = &g_state_machine_config.transitions[i];
        
        if (transition->trigger_type == TRIGGER_TYPE_TIMER && 
            strcmp(transition->trigger_source, timer_id) == 0 && 
            strcmp(transition->current_state, current_state) == 0) {
            
            // 找到匹配的转移规则
            transition_found = true;
            strncpy(next_state, transition->next_state, sizeof(next_state) - 1);
            
            // 执行状态转移
            strncpy(g_state_machine_config.current_state, transition->next_state, 
                    sizeof(g_state_machine_config.current_state) - 1);
            
            break;
        }
    }
    
    unlock_state_machine_config();
    
    // 如果找到匹配的转移规则，应用新状态的效果
    if (transition_found) {
        // 移除中断上下文中的日志调用，避免潜在的死锁问题
        
        // 创建新任务来应用状态效果，避免在中断上下文中执行I2C操作
        // 这解决了AW9523的ESP_ERR_TIMEOUT错误
        char *next_state_copy = (char*)malloc(MAX_STATE_NAME_LEN);
        if (next_state_copy != NULL) {
            strncpy(next_state_copy, next_state, MAX_STATE_NAME_LEN - 1);
            
            // 创建任务来应用状态效果
            xTaskCreate(
                [](void *arg) {
                    char *state_name = (char*)arg;
                    apply_state_effects(state_name);
                    free(state_name);
                    vTaskDelete(NULL);
                },
                "apply_state_effects_task",
                4096,  // 足够的栈空间
                next_state_copy,
                5,     // 中等优先级
                NULL
            );
        }
    } else {
        // 移除中断上下文中的日志调用，避免潜在的死锁问题
    }
    
    // 注意：不释放timer_id！它会在stop_state_machine或清理定时器时统一释放
    // 这样定时器可以持续循环触发
}

// 状态机监视器回调函数 - 增强版，确保能正确处理GPIO输入触发
void state_machine_monitor_callback(void *arg)
{
    bool should_break = false;
    
    // 检查状态机是否正在运行
    lock_state_machine_config();
    bool is_running = g_state_machine_config.is_running;
    uint32_t transition_count = g_state_machine_config.transition_count;
    state_transition_t *transitions = g_state_machine_config.transitions;
    unlock_state_machine_config();
    
    if (!is_running || transitions == NULL || transition_count == 0) {
        return;
    }
    
    // 遍历所有状态转移规则，检查是否有触发条件满足
    for (uint32_t i = 0; i < transition_count && !should_break; i++) {
        state_transition_t transition_copy;
        char current_state[32] = {0};
        int prev_gpio_state = -1;
        int group_id = -1;
        
        // 先获取必要的信息，然后立即释放锁
        lock_state_machine_config();
        if (g_state_machine_config.transitions == NULL) {
            unlock_state_machine_config();
            break;
        }
        transition_copy = g_state_machine_config.transitions[i];
        strncpy(current_state, g_state_machine_config.current_state, sizeof(current_state) - 1);
        
        // 仅在需要时获取GPIO组信息
        if (transition_copy.trigger_type == TRIGGER_TYPE_INPUT && transition_copy.trigger_source[0] != '\0') {
            char *endptr;
            group_id = strtol(transition_copy.trigger_source, &endptr, 10);
            if (*endptr == '\0' && group_id >= 0 && group_id < GPIO_GROUP_COUNT) {
                prev_gpio_state = g_state_machine_config.previous_gpio_states[group_id];
            }
        }
        unlock_state_machine_config();
        
        // 检查当前状态是否匹配
        if (strcmp(transition_copy.current_state, current_state) != 0) {
            continue;
        }
        
        // 检查trigger_source是否为空
        if (transition_copy.trigger_source[0] == '\0') {
            continue;
        }
        
        // 根据触发类型处理不同的触发源
        if (transition_copy.trigger_type == TRIGGER_TYPE_INPUT) {
            // 检查解析是否成功，且整个字符串都是数字
            if (group_id >= 0 && group_id < GPIO_GROUP_COUNT) {
                // 尝试获取GPIO组信息
                gpio_group_t group;
                if (gpio_manager_get_group_info(group_id, &group) == ESP_OK) {
                    // 只检查低位IO（pin_index=0）是否为输入模式
                    bool is_input_group = group.is_input[0];
                    
                    if (is_input_group) {
                        bool triggerConditionMet = false;
                        
                        // 获取当前物理GPIO状态值（只读取低位IO）
                        int current_physical_state = group.states[0];
                        
                        // 获取模块类型
                        module_type_t module_type;
                        lock_state_machine_config();
                        module_type = g_state_machine_config.group_modules[group_id];
                        unlock_state_machine_config();
                        
                        // 将物理电平转换为逻辑电平
                        int current_logical_state = physical_to_logical_level(current_physical_state, module_type);
                        
                        // 检查是否为上升沿触发（逻辑电平从0变为1）
                        // prev_gpio_state 现在存储的是逻辑状态
                        bool is_rising_edge = (prev_gpio_state == 0 && current_logical_state == 1);
                        
                        // 只在逻辑上升沿触发状态转移
                        if (is_rising_edge) {
                            triggerConditionMet = true;
                        }
                        
                        if (triggerConditionMet) {
                            // 执行状态转移
                            lock_state_machine_config();
                            strncpy(g_state_machine_config.current_state, transition_copy.next_state, sizeof(g_state_machine_config.current_state) - 1);
                            
                            unlock_state_machine_config();
                            
                            // 创建新任务来应用状态效果，避免在中断上下文中执行I2C操作
                            // 这解决了AW9523的ESP_ERR_TIMEOUT错误
                            char *next_state_copy = (char*)malloc(MAX_STATE_NAME_LEN);
                            if (next_state_copy != NULL) {
                                strncpy(next_state_copy, transition_copy.next_state, MAX_STATE_NAME_LEN - 1);
                                
                                // 创建任务来应用状态效果
                                xTaskCreate(
                                    [](void *arg) {
                                        char *state_name = (char*)arg;
                                        apply_state_effects(state_name);
                                        free(state_name);
                                        vTaskDelete(NULL);
                                    },
                                    "apply_state_effects_task",
                                    4096,  // 足够的栈空间
                                    next_state_copy,
                                    5,     // 中等优先级
                                    NULL
                                );
                            }
                            
                            // 设置标志位，准备退出循环
                            should_break = true;
                        }
                        // 更新previous_gpio_states为逻辑状态，用于下次上升沿检测
                        lock_state_machine_config();
                        g_state_machine_config.previous_gpio_states[group_id] = current_logical_state;
                        unlock_state_machine_config();
                    }
                }
            }
        } else if (transition_copy.trigger_type == TRIGGER_TYPE_MANUAL) {
            // 手动触发类型不在这里处理，应该由其他函数（如HTTP API）处理
            continue;
        } else if (transition_copy.trigger_type == TRIGGER_TYPE_TIMER) {
            // 定时器触发类型在定时器回调函数中处理，这里跳过
            continue;
        }
    }
}

// 应用状态效果 - 增强版，确保状态效果能正确触发定时器和GPIO输出
static esp_err_t apply_state_effects(const char *state_name)
{
    // 移除中断上下文中的日志调用，避免潜在的死锁问题
    
    if (state_name == NULL || state_name[0] == '\0') {
        // 移除中断上下文中的日志调用，避免潜在的死锁问题
        return ESP_ERR_INVALID_ARG;
    }
    
    // 检查state_effects指针
    lock_state_machine_config();
    if (g_state_machine_config.state_effects == NULL || g_state_machine_config.effect_count == 0) {
        unlock_state_machine_config();
        return ESP_OK;
    }
    uint32_t effect_count_total = g_state_machine_config.effect_count;
    unlock_state_machine_config();
    
    // 重要：在应用新状态效果之前，先清理所有旧的定时器
    // 这确保每个状态只有自己的定时器在运行，避免定时器槽位耗尽
    cleanup_all_timers();
    
    // 仍然需要group_used数组来跟踪哪些GPIO组被当前状态使用
    bool group_used[GPIO_GROUP_COUNT] = {false};
    // 添加motor_used数组来跟踪哪些电机被当前状态使用
    bool motor_used[2] = {false, false};  // 2个电机: M1和M2
    
    // 临时数组用于保存需要应用的效果
    state_effect_t *effects_to_apply = (state_effect_t *)malloc(effect_count_total * sizeof(state_effect_t));
    if (!effects_to_apply) {
        // 移除中断上下文中的日志调用，避免潜在的死锁问题
        return ESP_ERR_NO_MEM;
    }
    
    int effect_count = 0;
    
    // 先关闭所有AW9523扩展组的LED（在状态切换时）
    for (int group_id = AW9523_GROUP_START; group_id < GPIO_GROUP_COUNT; group_id++) {
        // 检查GPIO组是否至少有一个输出引脚
        bool has_output_pin = false;
        for (int pin_index = 0; pin_index < GPIO_PER_GROUP; pin_index++) {
            int is_input = 0;
            if (gpio_manager_get_pin_mode(group_id, pin_index, &is_input) == ESP_OK && !is_input) {
                has_output_pin = true;
                break;
            }
        }
        
        // 只有包含输出引脚的组才进行重置
        if (has_output_pin) {
            for (int pin_index = 0; pin_index < GPIO_PER_GROUP; pin_index++) {
                int is_input = 0;
                if (gpio_manager_get_pin_mode(group_id, pin_index, &is_input) == ESP_OK && !is_input) {
                    // 只重置输出模式的引脚
                    gpio_manager_set_level(group_id, pin_index, 0);
                }
            }
        }
    }
    
    lock_state_machine_config();
    
    // 检查指针有效性
    if (g_state_machine_config.state_effects == NULL) {
        unlock_state_machine_config();
        free(effects_to_apply);
        return ESP_OK;
    }
    
    // 收集当前状态的所有效果
    for (uint32_t i = 0; i < g_state_machine_config.effect_count; i++) {
        if (strcmp(g_state_machine_config.state_effects[i].state_name, state_name) == 0) {
            effects_to_apply[effect_count] = g_state_machine_config.state_effects[i];
            effect_count++;
            
            // 如果效果类型是OUTPUT并且target是数字，标记该GPIO组为已使用
            if (g_state_machine_config.state_effects[i].effect_type == EFFECT_TYPE_OUTPUT) {
                char *endptr;
                int group_id = strtol(g_state_machine_config.state_effects[i].target, &endptr, 10);
                if (*endptr == '\0' && group_id >= 0 && group_id < GPIO_GROUP_COUNT) {
                    group_used[group_id] = true;
                }
            }
            // 如果效果类型是MOTOR，标记该电机为已使用
            else if (g_state_machine_config.state_effects[i].effect_type == EFFECT_TYPE_MOTOR) {
                char *endptr;
                int motor_id = strtol(g_state_machine_config.state_effects[i].target, &endptr, 10);
                if (*endptr == '\0' && motor_id >= 0 && motor_id <= 1) {
                    motor_used[motor_id] = true;
                }
            }
        }
    }
    
    unlock_state_machine_config();
    
    // 移除中断上下文中的日志调用，避免潜在的死锁问题
    
    // 应用所有收集到的效果
    bool all_effects_applied = true;
    for (int i = 0; i < effect_count; i++) {
        if (handle_output_effect(&effects_to_apply[i]) != ESP_OK) {
            // 移除日志调用，避免递归锁问题
            all_effects_applied = false;
        }
    }
    
    // 将未在当前状态中使用的GPIO组重置为00状态
    // 注意：只重置原生GPIO组(0-3)，不重置AW9523扩展组和输入组
    for (int group_id = 0; group_id < AW9523_GROUP_START; group_id++) {
        if (!group_used[group_id]) {
            // 检查GPIO组是否至少有一个输出引脚
            bool has_output_pin = false;
            for (int pin_index = 0; pin_index < GPIO_PER_GROUP; pin_index++) {
                int is_input = 0;
                if (gpio_manager_get_pin_mode(group_id, pin_index, &is_input) == ESP_OK && !is_input) {
                    has_output_pin = true;
                    break;
                }
            }
            
            // 只有包含输出引脚的组才进行重置
            if (has_output_pin) {
                for (int pin_index = 0; pin_index < GPIO_PER_GROUP; pin_index++) {
                    int is_input = 0;
                    if (gpio_manager_get_pin_mode(group_id, pin_index, &is_input) == ESP_OK && !is_input) {
                        // 只重置输出模式的引脚
                        gpio_manager_set_level(group_id, pin_index, 0);
                    }
                }
            }
        }
    }
    
    // 将未在当前状态中使用的电机重置为停止状态
    for (int motor_id = 0; motor_id < 2; motor_id++) {
        if (!motor_used[motor_id]) {
            gpio_manager_set_motor(motor_id, 0);  // 0 = 停止
        }
    }
    
    // 释放临时数组
    free(effects_to_apply);
    
    return all_effects_applied ? ESP_OK : ESP_FAIL;
}

// 状态机完整实现方案
// 1. 更新handle_output_effect函数，增强定时器效果处理
// 2. 优化timer_callback函数，确保定时器能正确触发状态转移
// 3. 改进apply_state_effects函数，确保状态效果能正确触发定时器和GPIO输出

// 处理输出效果
static esp_err_t handle_output_effect(const state_effect_t *effect)
{
    if (effect == NULL) {
        // 避免在定时器回调中使用ESP_LOGE
        return ESP_ERR_INVALID_ARG;
    }
    
    // 根据效果类型执行不同的操作
    if (effect->effect_type == EFFECT_TYPE_OUTPUT) {
        // 处理输出效果 - GPIO输出
        char *endptr;
        int group_id = strtol(effect->target, &endptr, 10);
        
        // 检查target是否为有效的GPIO组ID
        if (*endptr == '\0' && group_id >= 0 && group_id < GPIO_GROUP_COUNT) {
            int value = strtol(effect->effect_value, NULL, 10);
            
            // 根据模块类型控制GPIO
            if (effect->output_module_type == OUTPUT_MODULE_TYPE_TRAFFIC_LIGHT) {
                // 交通灯: 0=灭, 1=红, 2=黄, 3=绿
                // 使用两个IO引脚的高低位组合来表示
                // 0: 低位=0, 高位=0 (灭)
                // 1: 低位=1, 高位=0 (红)
                // 2: 低位=0, 高位=1 (黄)
                // 3: 低位=1, 高位=1 (绿)
                int is_input_low = 0, is_input_high = 0;
                if (gpio_manager_get_pin_mode(group_id, 0, &is_input_low) == ESP_OK && !is_input_low &&
                    gpio_manager_get_pin_mode(group_id, 1, &is_input_high) == ESP_OK && !is_input_high) {
                    // 计算低位和高位的值
                    int low_bit = value & 0x01;  // 取最低位
                    int high_bit = (value >> 1) & 0x01;  // 取第二位
                    
                    // 设置两个IO引脚
                    if (gpio_manager_set_level(group_id, 0, low_bit) != ESP_OK ||
                        gpio_manager_set_level(group_id, 1, high_bit) != ESP_OK) {
                        return ESP_ERR_INVALID_ARG;
                    }
                }
            } else {
                // LED灯和风扇: 0=关, 1=开
                // 控制低位IO
                int is_input = 0;
                if (gpio_manager_get_pin_mode(group_id, 0, &is_input) == ESP_OK && !is_input) {
                    if (gpio_manager_set_level(group_id, 0, value) != ESP_OK) {
                        return ESP_ERR_INVALID_ARG;
                    }
                }
            }
            
            return ESP_OK;
        } else {
            // 避免在定时器回调中使用ESP_LOGE
            return ESP_ERR_INVALID_ARG;
        }
    } else if (effect->effect_type == EFFECT_TYPE_MOTOR) {
        // 处理电机效果
        char *endptr;
        int motor_id = strtol(effect->target, &endptr, 10);
        
        // 检查motor_id是否有效 (0=M1, 1=M2)
        if (*endptr == '\0' && motor_id >= 0 && motor_id <= 1) {
            // effect_value: 0=停止, 1-9=正转(10%-90%), -1到-9=反转(10%-90%)
            int8_t speed_direction = (int8_t)strtol(effect->effect_value, NULL, 10);
            
            // 调用gpio_manager的电机控制函数
            if (gpio_manager_set_motor(motor_id, speed_direction) != ESP_OK) {
                return ESP_ERR_INVALID_ARG;
            }
            
            return ESP_OK;
        } else {
            return ESP_ERR_INVALID_ARG;
        }
    } else if (effect->effect_type == EFFECT_TYPE_TIMER) {
        // 处理定时器效果 - 激活定时器，在定时器结束后会激活对应的转移策略
        lock_state_machine_config();
        
        // 获取当前状态
        char current_state[MAX_STATE_NAME_LEN] = {0};
        strncpy(current_state, g_state_machine_config.current_state, sizeof(current_state) - 1);
        
        // 查找匹配的转移策略
        state_transition_t *matched_transition = NULL;
        int delay_seconds = -1; // 修改：使用秒作为单位
        
        for (uint32_t i = 0; i < g_state_machine_config.transition_count; i++) {
            state_transition_t *transition = &g_state_machine_config.transitions[i];
            
            // 查找匹配当前状态和定时器ID的转移策略
            if (transition->trigger_type == TRIGGER_TYPE_TIMER && 
                strcmp(transition->trigger_source, effect->target) == 0 &&
                strcmp(transition->current_state, current_state) == 0) {
                
                matched_transition = transition;
                
                // 获取延迟时间 - 优先使用effect_value中的值（单位：秒）
                if (effect->effect_value[0] != '\0') {
                    delay_seconds = strtol(effect->effect_value, NULL, 10);
                } else if (effect->delay_ms > 0) {
                    // 注意：这里假设delay_ms实际上是秒
                    delay_seconds = effect->delay_ms;
                } else {
                    delay_seconds = 1; // 默认1秒
                }
                break;
            }
        }
        
        // 如果没有找到匹配的转移策略，尝试将effect->target作为定时器ID直接使用
        if (matched_transition == NULL) {
            // 尝试从effect_value获取延迟时间（单位：秒）
            if (effect->effect_value[0] != '\0') {
                delay_seconds = strtol(effect->effect_value, NULL, 10);
            } else if (effect->delay_ms > 0) {
                // 注意：这里假设delay_ms实际上是秒
                delay_seconds = effect->delay_ms;
            } else {
                delay_seconds = 1; // 默认1秒
            }
        }
        
        unlock_state_machine_config();
        
        // 检查延迟时间是否有效
        if (delay_seconds <= 0) {
            // 避免在定时器回调中使用ESP_LOGE
            return ESP_ERR_INVALID_ARG;
        }
        
        // 先在timer_manager中分配一个槽位
        lock_state_machine_config();
        if (g_state_machine_config.timer_manager.timer_count >= g_state_machine_config.timer_manager.timer_capacity) {
            unlock_state_machine_config();
            return ESP_ERR_NO_MEM;
        }
        
        // 获取当前timer槽位的索引
        uint32_t timer_index = g_state_machine_config.timer_manager.timer_count;
        state_machine_timer_t *timer_info = &g_state_machine_config.timer_manager.timers[timer_index];
        
        // 保存定时器ID到timer_manager结构中
        strncpy(timer_info->timer_id, effect->target, sizeof(timer_info->timer_id) - 1);
        timer_info->timer_id[sizeof(timer_info->timer_id) - 1] = '\0';
        
        // 配置定时器 - 使用timer_manager中的timer_id作为参数
        const esp_timer_create_args_t timer_args = {
            .callback = &timer_callback,
            .arg = timer_info->timer_id,  // 使用timer_manager中的字符串，不需要额外malloc
            .name = "state_machine_timer"
        };
        
        // 创建定时器
        esp_timer_handle_t timer_handle = NULL;
        esp_err_t err = esp_timer_create(&timer_args, &timer_handle);
        if (err != ESP_OK) {
            unlock_state_machine_config();
            return err;
        }
        
        // 启动定时器 - 将秒转换为微秒（秒 * 1000 * 1000）
        err = esp_timer_start_once(timer_handle, (uint64_t)delay_seconds * 1000000);
        if (err != ESP_OK) {
            esp_timer_delete(timer_handle);
            unlock_state_machine_config();
            return err;
        }
        
        // 保存定时器信息到定时器管理器
        timer_info->handle = timer_handle;
        timer_info->duration_ms = delay_seconds;
        
        // 如果找到了匹配的转移策略，保存完整的状态信息
        if (matched_transition != NULL) {
            strncpy(timer_info->current_state, matched_transition->current_state, 
                    sizeof(timer_info->current_state) - 1);
            strncpy(timer_info->next_state, matched_transition->next_state, 
                    sizeof(timer_info->next_state) - 1);
        } else {
            // 使用当前状态
            strncpy(timer_info->current_state, current_state, sizeof(timer_info->current_state) - 1);
            memset(timer_info->next_state, 0, sizeof(timer_info->next_state));
        }
        
        // 增加定时器计数
        g_state_machine_config.timer_manager.timer_count++;
        
        unlock_state_machine_config();
        
        // 移除中断上下文中的日志调用，避免潜在的死锁问题
        
        return ESP_OK;
    }
    
    // 移除中断上下文中的日志调用，避免潜在的死锁问题
    return ESP_ERR_NOT_SUPPORTED;
}

// HTTP请求处理函数
/**
 * @brief 处理状态机相关的HTTP请求（HTTP服务器版本）
 * 
 * @param req HTTP请求
 * @return esp_err_t 处理结果
 */
esp_err_t state_machine_http_handler(httpd_req_t *req)
{
    if (req == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 对于OPTIONS请求，设置CORS头并返回成功
    if (req->method == HTTP_OPTIONS) {
        // 设置CORS头信息
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization");
        httpd_resp_set_hdr(req, "Access-Control-Max-Age", "86400");
        
        // 返回204 No Content响应
        httpd_resp_set_status(req, "204 No Content");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    // 对于GET请求，可以返回当前状态机配置
    if (req->method == HTTP_GET) {
        // 设置CORS头
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_type(req, "application/json");
        
        // 这里可以实现返回当前状态机配置的逻辑
        // 为简化，先返回空配置
        httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"GET method is allowed\"}", -1);
        return ESP_OK;
    }
    
    // 仅处理POST请求
    if (req->method != HTTP_POST) { 
        httpd_resp_set_status(req, "405 Method Not Allowed");
        httpd_resp_send(req, "Method not allowed", strlen("Method not allowed"));
        return ESP_FAIL;
    }
    
    // 设置CORS头
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    
    // 动态分配缓冲区以接收大型 JSON 数据
    char *content = (char *)malloc(req->content_len + 1);
    if (content == NULL) {
        ESP_LOGE(TAG, "Failed to allocate buffer for content_len: %d", req->content_len);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_send(req, "Out of memory", strlen("Out of memory"));
        return ESP_FAIL;
    }
    
    // 读取请求体内容
    int ret = httpd_req_recv(req, content, req->content_len);
    
    if (ret <= 0) {
        free(content);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_set_status(req, "408 Request Timeout");
            httpd_resp_send(req, "Request timed out", strlen("Request timed out"));
        }
        return ESP_FAIL;
    }
    
    content[ret] = '\0';
    
    // 解析JSON请求
    cJSON *root = cJSON_Parse(content);
    free(content);  // 解析后立即释放缓冲区
    
    if (root == NULL) {
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_send(req, "Invalid JSON format", strlen("Invalid JSON format"));
        return ESP_FAIL;
    }
    
    // 检查是否是控制命令
    cJSON *command = cJSON_GetObjectItem(root, "command");
    if (command != NULL && cJSON_IsString(command)) {
        esp_err_t err = ESP_OK;
        
        // 简单命令可以直接处理，因为它们不会执行耗时的I2C操作
        if (strcmp(command->valuestring, "start") == 0) {
            err = state_machine_start(g_state_machine_config.initial_state);
        } else if (strcmp(command->valuestring, "stop") == 0) {
            err = state_machine_stop();
        } else if (strcmp(command->valuestring, "reset") == 0) {
            err = state_machine_reset();
        } else if (strcmp(command->valuestring, "init") == 0) {
            err = state_machine_init();
        } else {
            httpd_resp_set_status(req, "400 Bad Request");
            httpd_resp_send(req, "Unknown command", strlen("Unknown command"));
            cJSON_Delete(root);
            return ESP_FAIL;
        }
        
        if (err == ESP_OK) {
            httpd_resp_set_status(req, "200 OK");
            httpd_resp_send(req, "Command executed successfully", strlen("Command executed successfully"));
        } else {
            httpd_resp_set_status(req, "500 Internal Server Error");
            char error_msg[100] = {0};
            snprintf(error_msg, sizeof(error_msg), "Command execution failed: %s", esp_err_to_name(err));
            httpd_resp_send(req, error_msg, strlen(error_msg));
        }
        
        cJSON_Delete(root);
        return ESP_OK;
    }
    
    // 如果不是控制命令，则尝试更新配置
    // 注意：这里我们创建一个新的配置结构体并复制数据，但不立即执行更新
    state_machine_config_t *new_config = (state_machine_config_t *)malloc(sizeof(state_machine_config_t));
    if (new_config == NULL) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_send(req, "Memory allocation failed", strlen("Memory allocation failed"));
        cJSON_Delete(root);
        return ESP_FAIL;
    }
    memset(new_config, 0, sizeof(state_machine_config_t));
    
    // 分配动态数组
    new_config->transitions = (state_transition_t*)malloc(MAX_TRANSITION_RULES * sizeof(state_transition_t));
    new_config->state_effects = (state_effect_t*)malloc(MAX_STATE_EFFECTS * sizeof(state_effect_t));
    new_config->timer_manager.timers = (state_machine_timer_t*)malloc(MAX_STATE_EFFECTS * sizeof(state_machine_timer_t));
    
    if (new_config->transitions == NULL || 
        new_config->state_effects == NULL ||
        new_config->timer_manager.timers == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for new config arrays");
        if (new_config->transitions) free(new_config->transitions);
        if (new_config->state_effects) free(new_config->state_effects);
        if (new_config->timer_manager.timers) free(new_config->timer_manager.timers);
        free(new_config);
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_send(req, "Memory allocation failed", strlen("Memory allocation failed"));
        cJSON_Delete(root);
        return ESP_FAIL;
    }
    
    // 初始化内存
    memset(new_config->transitions, 0, MAX_TRANSITION_RULES * sizeof(state_transition_t));
    memset(new_config->state_effects, 0, MAX_STATE_EFFECTS * sizeof(state_effect_t));
    memset(new_config->timer_manager.timers, 0, MAX_STATE_EFFECTS * sizeof(state_machine_timer_t));
    
    // 设置容量
    new_config->transition_capacity = MAX_TRANSITION_RULES;
    new_config->effect_capacity = MAX_STATE_EFFECTS;
    new_config->timer_manager.timer_capacity = MAX_STATE_EFFECTS;
    
    ESP_LOGI(TAG, "配置解析开始，分配容量：转换规则=%u，效果=%u", 
             (unsigned int)new_config->transition_capacity, (unsigned int)new_config->effect_capacity);
    
    // 解析初始状态
    cJSON *initialState = cJSON_GetObjectItem(root, "initialState");
    if (initialState != NULL && cJSON_IsString(initialState)) {
        strncpy(new_config->initial_state, initialState->valuestring, sizeof(new_config->initial_state) - 1);
    } else {
        // 即使字段不存在，也设置一个默认值，防止显示为空
        strncpy(new_config->initial_state, "", sizeof(new_config->initial_state) - 1);
    }
    
    // 解析转移规则
    cJSON *transitions = cJSON_GetObjectItem(root, "transitions");
    if (transitions != NULL && cJSON_IsArray(transitions)) {
        int transition_count = cJSON_GetArraySize(transitions);
        new_config->transition_count = transition_count > new_config->transition_capacity ? new_config->transition_capacity : transition_count;
        
        for (int i = 0; i < new_config->transition_count; i++) {
            cJSON *transition = cJSON_GetArrayItem(transitions, i);
            
            cJSON *currentState = cJSON_GetObjectItem(transition, "currentState");
            cJSON *nextState = cJSON_GetObjectItem(transition, "nextState");
            cJSON *triggerType = cJSON_GetObjectItem(transition, "triggerType");
            cJSON *triggerSource = cJSON_GetObjectItem(transition, "triggerSource");
            
            if (currentState != NULL && cJSON_IsString(currentState) &&
                nextState != NULL && cJSON_IsString(nextState) &&
                triggerType != NULL && cJSON_IsNumber(triggerType) &&
                triggerSource != NULL && cJSON_IsString(triggerSource)) {
                
                strncpy(new_config->transitions[i].current_state, currentState->valuestring, sizeof(new_config->transitions[i].current_state) - 1);
                strncpy(new_config->transitions[i].next_state, nextState->valuestring, sizeof(new_config->transitions[i].next_state) - 1);
                new_config->transitions[i].trigger_type = (trigger_type_t)triggerType->valueint;
                strncpy(new_config->transitions[i].trigger_source, triggerSource->valuestring, sizeof(new_config->transitions[i].trigger_source) - 1);
            }
        }
    }
    
    // 解析状态效果
    cJSON *stateEffects = cJSON_GetObjectItem(root, "stateEffects");
    if (stateEffects != NULL && cJSON_IsArray(stateEffects)) {
        int effect_count = cJSON_GetArraySize(stateEffects);
        new_config->effect_count = effect_count > new_config->effect_capacity ? new_config->effect_capacity : effect_count;
        
        // 先重置有效效果计数
        int valid_effect_count = 0;
        
        for (int i = 0; i < new_config->effect_count; i++) {
            cJSON *effect = cJSON_GetArrayItem(stateEffects, i);
            
            cJSON *stateName = cJSON_GetObjectItem(effect, "stateName");
            cJSON *effectType = cJSON_GetObjectItem(effect, "effectType");
            cJSON *target = cJSON_GetObjectItem(effect, "target");
            cJSON *value = cJSON_GetObjectItem(effect, "value");
            
            if (stateName != NULL && cJSON_IsString(stateName) &&
                effectType != NULL && cJSON_IsNumber(effectType) &&
                target != NULL && cJSON_IsString(target) &&
                value != NULL && cJSON_IsString(value)) {
                
                strncpy(new_config->state_effects[i].state_name, stateName->valuestring, sizeof(new_config->state_effects[i].state_name) - 1);
                new_config->state_effects[i].effect_type = (effect_type_t)effectType->valueint;
                
                // 输出模块类型从groupModules获取，这里先设置默认值
                new_config->state_effects[i].output_module_type = OUTPUT_MODULE_TYPE_LED;
                
                strncpy(new_config->state_effects[i].target, target->valuestring, sizeof(new_config->state_effects[i].target) - 1);
                strncpy(new_config->state_effects[i].effect_value, value->valuestring, sizeof(new_config->state_effects[i].effect_value) - 1);
                
                valid_effect_count++;
            }
        }
        
        // 更新为实际有效效果数量
        new_config->effect_count = valid_effect_count;
    }
    
    // 解析groupModes
    cJSON *groupModes = cJSON_GetObjectItem(root, "groupModes");
    if (groupModes != NULL && cJSON_IsObject(groupModes)) {
        // 遍历所有可能的GPIO组
        for (int group_id = 0; group_id < GPIO_GROUP_COUNT; group_id++) {
            char group_key[16];
            snprintf(group_key, sizeof(group_key), "%d", group_id);
            cJSON *group_mode = cJSON_GetObjectItem(groupModes, group_key);
            
            if (group_mode != NULL && cJSON_IsNumber(group_mode)) {
                int mode_value = group_mode->valueint;
                // 根据用户要求：1表示两个IO都是输入，0表示两个IO都是输出
                bool is_input = (mode_value == 1);
                
                for (int pin_index = 0; pin_index < GPIO_PER_GROUP; pin_index++) {
                    new_config->group_modes[group_id][pin_index] = is_input;
                }
            } else {
                // 默认设置为输出模式
                for (int pin_index = 0; pin_index < GPIO_PER_GROUP; pin_index++) {
                    new_config->group_modes[group_id][pin_index] = false;
                }
            }
        }
    } else {
        // 默认设置所有GPIO组为输出模式
        for (int group_id = 0; group_id < GPIO_GROUP_COUNT; group_id++) {
            for (int pin_index = 0; pin_index < GPIO_PER_GROUP; pin_index++) {
                new_config->group_modes[group_id][pin_index] = false;
            }
        }
    }
    
    // 解析groupModules（模块类型）
    cJSON *groupModules = cJSON_GetObjectItem(root, "groupModules");
    if (groupModules != NULL && cJSON_IsObject(groupModules)) {
        // 遍历所有可能的GPIO组
        for (int group_id = 0; group_id < GPIO_GROUP_COUNT; group_id++) {
            char group_key[16];
            snprintf(group_key, sizeof(group_key), "%d", group_id);
            cJSON *module_type = cJSON_GetObjectItem(groupModules, group_key);
            
            if (module_type != NULL && cJSON_IsString(module_type)) {
                const char *type_str = module_type->valuestring;
                // 输入模块类型:
                // "button": MODULE_TYPE_NORMAL_BUTTON (默认高，触发低)
                // "led_button": MODULE_TYPE_LED_BUTTON (默认低，触发高)
                if (strcmp(type_str, "button") == 0) {
                    new_config->group_modules[group_id] = MODULE_TYPE_NORMAL_BUTTON;
                } else if (strcmp(type_str, "led_button") == 0) {
                    new_config->group_modules[group_id] = MODULE_TYPE_LED_BUTTON;
                // 输出模块类型 (仅用于标记，实际控制在apply_state_effects时处理):
                // "led": LED灯
                // "traffic_light": 交通灯
                // "fan": 风扇
                } else if (strcmp(type_str, "led") == 0 || 
                          strcmp(type_str, "traffic_light") == 0 || 
                          strcmp(type_str, "fan") == 0) {
                    // 输出模块类型暂时也存在group_modules中（复用字段）
                    // 实际使用时需要区分输入和输出
                    new_config->group_modules[group_id] = MODULE_TYPE_NORMAL_BUTTON; // 占位
                } else {
                    // 默认使用普通按键类型
                    new_config->group_modules[group_id] = MODULE_TYPE_NORMAL_BUTTON;
                }
            } else {
                // 默认使用普通按键类型
                new_config->group_modules[group_id] = MODULE_TYPE_NORMAL_BUTTON;
            }
            
            // 初始化物理状态记录为-1（表示未初始化）
            new_config->previous_physical_states[group_id] = -1;
        }
    } else {
        // 默认设置所有GPIO组为普通按键类型
        for (int group_id = 0; group_id < GPIO_GROUP_COUNT; group_id++) {
            new_config->group_modules[group_id] = MODULE_TYPE_NORMAL_BUTTON;
            new_config->previous_physical_states[group_id] = -1;
        }
    }
    
    // 解析完groupModules后，更新state_effects中的输出模块类型
    cJSON *groupModules2 = cJSON_GetObjectItem(root, "groupModules");
    if (groupModules2 != NULL && cJSON_IsObject(groupModules2)) {
        for (uint32_t i = 0; i < new_config->effect_count; i++) {
            // 仅处理GPIO输出类型
            if (new_config->state_effects[i].effect_type == EFFECT_TYPE_OUTPUT) {
                // 获取target（GPIO组ID）
                char *endptr;
                int group_id = strtol(new_config->state_effects[i].target, &endptr, 10);
                
                if (*endptr == '\0' && group_id >= 0 && group_id < GPIO_GROUP_COUNT) {
                    // 从groupModules查找该组的模块类型
                    char group_key[16];
                    snprintf(group_key, sizeof(group_key), "%d", group_id);
                    cJSON *module_type = cJSON_GetObjectItem(groupModules2, group_key);
                    
                    if (module_type != NULL && cJSON_IsString(module_type)) {
                        const char *type_str = module_type->valuestring;
                        
                        if (strcmp(type_str, "led") == 0) {
                            new_config->state_effects[i].output_module_type = OUTPUT_MODULE_TYPE_LED;
                        } else if (strcmp(type_str, "traffic_light") == 0) {
                            new_config->state_effects[i].output_module_type = OUTPUT_MODULE_TYPE_TRAFFIC_LIGHT;
                        } else if (strcmp(type_str, "fan") == 0) {
                            new_config->state_effects[i].output_module_type = OUTPUT_MODULE_TYPE_FAN;
                        }
                        // 其他类型（button, led_button）保持默认LED
                    }
                }
            }
        }
    }
    
    // 释放JSON对象
    cJSON_Delete(root);
    
    // 在HTTP服务器任务中，我们不应该执行可能阻塞的操作
    // 因此我们只返回一个接受请求的响应，然后在后台处理配置更新
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"success\",\"message\":\"Configuration update request accepted and will be processed in background\"}", strlen("{\"status\":\"success\",\"message\":\"Configuration update request accepted and will be processed in background\"}"));
    
    // 现在在后台执行配置更新
    // 注意：这里我们创建一个短期任务来处理配置更新
    // 这比在HTTP任务中直接执行更安全，因为它不会阻塞HTTP响应
    TaskHandle_t update_task_handle = NULL;
    xTaskCreate(
        [](void *pvParameters) {
            state_machine_config_t *config = (state_machine_config_t *)pvParameters;
            
            // 执行配置更新
            (void)state_machine_update_config(config); // 避免未使用变量警告
            
            // 释放动态数组
            if (config->transitions) free(config->transitions);
            if (config->state_effects) free(config->state_effects);
            if (config->timer_manager.timers) free(config->timer_manager.timers);
            
            // 释放配置结构体
            free(config);
            
            // 删除自己
            vTaskDelete(NULL);
        },
        "state_machine_config_update",
        4096,  // 足够的栈空间
        new_config,
        5,     // 中等优先级，高于HTTP任务但低于关键任务
        &update_task_handle
    );
    
    if (update_task_handle == NULL) {
        // 如果任务创建失败，确保释放资源
        free(new_config);
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif