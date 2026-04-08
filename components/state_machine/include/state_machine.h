/*
 * 状态机模块头文件
 * 提供状态管理、状态转移和状态效果处理功能
 */
#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include "esp_err.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "../gpio_manager/include/gpio_manager.h"
#include "esp_timer.h"
#include "esp_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// 宏定义
#define MAX_STATE_NAME_LEN 32         // 最大状态名称长度
#define MAX_TRIGGER_SOURCE_LEN 32     // 最大触发源长度
#define MAX_TARGET_NAME_LEN 32        // 最大目标名称长度
#define MAX_VALUE_LEN 64              // 最大值长度
#define MAX_TRANSITION_RULES 200      // 最大转移规则数量 - 使用动态分配
#define MAX_STATE_EFFECTS 200         // 最大状态效果数量（包括定时器）- 使用动态分配

// 效果类型枚举
typedef enum {
    EFFECT_TYPE_NONE = 0,             // 无效果
    EFFECT_TYPE_OUTPUT,               // 输出效果
    EFFECT_TYPE_TIMER,                // 定时器效果
    EFFECT_TYPE_MOTOR,                // 电机效果
    EFFECT_TYPE_NOTIFICATION          // 通知效果
} effect_type_t;

// 触发类型枚举
typedef enum {
    TRIGGER_TYPE_NONE = 0,            // 无触发
    TRIGGER_TYPE_INPUT,               // 输入触发
    TRIGGER_TYPE_TIMER,               // 定时器触发
    TRIGGER_TYPE_MANUAL               // 手动触发
} trigger_type_t;

// 状态转移规则结构体
typedef struct {
    char current_state[MAX_STATE_NAME_LEN];      // 当前状态
    char next_state[MAX_STATE_NAME_LEN];         // 下一个状态
    char trigger_source[MAX_TRIGGER_SOURCE_LEN]; // 触发源
    trigger_type_t trigger_type;                 // 触发类型
} state_transition_t;

// 定时器结构体
typedef struct {
    char timer_id[MAX_TRIGGER_SOURCE_LEN];       // 定时器ID
    esp_timer_handle_t handle;                   // 定时器句柄
    char current_state[MAX_STATE_NAME_LEN];      // 关联的当前状态
    char next_state[MAX_STATE_NAME_LEN];         // 定时器到期后要转移到的状态
    uint32_t duration_ms;                        // 定时器持续时间（毫秒）
} state_machine_timer_t;

// 定时器管理器结构体
typedef struct {
    state_machine_timer_t *timers;               // 定时器数组（动态分配）
    uint32_t timer_count;                        // 活跃定时器数量
    uint32_t timer_capacity;                     // 定时器数组容量
} timer_manager_t;

// GPIO输出模块类型
typedef enum {
    OUTPUT_MODULE_TYPE_LED = 0,           // LED灯（0=关，1=开）
    OUTPUT_MODULE_TYPE_TRAFFIC_LIGHT = 1, // 交通灯（0=灭，1=红，2=黄，3=绿）
    OUTPUT_MODULE_TYPE_FAN = 2            // 风扇（0=关，1=开）
} output_module_type_t;

// 状态效果结构体
typedef struct {
    char state_name[MAX_STATE_NAME_LEN];         // 状态名称
    effect_type_t effect_type;                   // 效果类型
    output_module_type_t output_module_type;     // 输出模块类型（仅GPIO输出有效）
    char target[MAX_TARGET_NAME_LEN];            // 目标
    char effect_value[MAX_VALUE_LEN];            // 效果值
    uint32_t delay_ms;                           // 延迟时间（毫秒）
} state_effect_t;

// 模块类型枚举
typedef enum {
    MODULE_TYPE_NORMAL_BUTTON = 0,    // 普通按键：默认高电平，触发时低电平
    MODULE_TYPE_LED_BUTTON = 1        // 带灯按键：默认低电平，触发时高电平
} module_type_t;

// 状态机配置结构体
typedef struct {
    state_transition_t *transitions;                       // 状态转移规则（动态分配）
    uint32_t transition_count;                             // 转移规则数量
    uint32_t transition_capacity;                          // 转移规则数组容量
    state_effect_t *state_effects;                         // 状态效果（动态分配）
    uint32_t effect_count;                                 // 效果数量
    uint32_t effect_capacity;                              // 效果数组容量
    char initial_state[MAX_STATE_NAME_LEN];                // 初始状态
    char current_state[MAX_STATE_NAME_LEN];                // 当前状态
    bool is_running;                                       // 是否运行中
    int previous_gpio_states[GPIO_GROUP_COUNT];            // 存储GPIO组之前的状态
    int previous_physical_states[GPIO_GROUP_COUNT];        // 存储GPIO组之前的物理状态
    timer_manager_t timer_manager;                         // 定时器管理器
    bool group_modes[GPIO_GROUP_COUNT][GPIO_PER_GROUP];    // GPIO组模式配置 (0=输出, 1=输入)
    module_type_t group_modules[GPIO_GROUP_COUNT];         // GPIO组模块类型配置
} state_machine_config_t;

// 初始化状态机
esp_err_t state_machine_init(void);

// 启动状态机
esp_err_t state_machine_start(const char *initial_state);

// 停止状态机
esp_err_t state_machine_stop(void);

// 重置状态机
esp_err_t state_machine_reset(void);

// 获取状态机配置
esp_err_t state_machine_get_config(state_machine_config_t *config);

// 更新状态机配置
esp_err_t state_machine_update_config(const state_machine_config_t *config);

// 获取状态机当前状态
esp_err_t state_machine_get_current_state(char *state_name, size_t max_len);

/**
 * @brief 处理状态机相关的HTTP请求（HTTP服务器版本）
 * 
 * @param req HTTP请求
 * @return esp_err_t 处理结果
 */
esp_err_t state_machine_http_handler(httpd_req_t *req);

/**
 * @brief 获取状态机配置（JSON格式）
 * 
 * @return cJSON* 状态机配置的JSON对象，使用完毕后需要调用cJSON_Delete释放
 */
cJSON *state_machine_get_config_json(void);

#ifdef __cplusplus
}
#endif

#endif // STATE_MACHINE_H