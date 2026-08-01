/*
 * 状态机 V3.0 - 类型定义
 */

#ifndef SM_TYPES_H
#define SM_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
#define SM_MAX_STATE_NAME_LEN       32
#define SM_MAX_INPUT_NAME_LEN       32
#define SM_MAX_TRIGGER_SOURCE_LEN   32
#define SM_MAX_EFFECT_TARGET_LEN    32
#define SM_MAX_TRANSITIONS          16
#define SM_MAX_EFFECTS              16
#define SM_MAX_STATES               16
#define SM_MAX_TIMERS               3
#define SM_MAX_MOTORS               2
#define SM_MONITOR_INTERVAL_MS      100
#define SM_MAX_GPIO_GROUPS          12

/* 输入模块类型 - 用于 GPIO 电平转换和传感器读取 */
typedef enum {
    SM_INPUT_MODULE_NORMAL_BUTTON = 0,      /* 普通按键：物理 0 → 逻辑 1（按下） */
    SM_INPUT_MODULE_LED_BUTTON = 1,         /* 带灯按键：物理 1 → 逻辑 1（按下） */
    SM_INPUT_MODULE_TEMP_SENSOR = 2,        /* 温度传感器：返回温度值（0.1°C精度） */
    SM_INPUT_MODULE_HUMIDITY_SENSOR = 3,    /* 湿度传感器（预留） */
    SM_INPUT_MODULE_VOLTAGE_MONITOR = 4     /* 电压监测（预留） */
} sm_input_module_type_t;

/* 
 * 边沿检测类型
 */
typedef enum {
    SM_EDGE_ANY = 0,           /* 任何边沿 (上升或下降) */
    SM_EDGE_RISING,            /* 上升沿 (0->1) */
    SM_EDGE_FALLING            /* 下降沿 (1->0) */
} sm_edge_type_t;

/* 输出模块类型 - 用于效果值解释 */
typedef enum {
    SM_OUTPUT_LED = 0,              /* LED灯：0=关，1=开 */
    SM_OUTPUT_TRAFFIC_LIGHT = 1,    /* 交通灯：0=灭，1=红，2=黄，3=绿 */
    SM_OUTPUT_FAN = 2               /* 风扇：0=关，非0=开 */
} sm_output_module_type_t;

/* 定时器状态 - 统一状态（效果启动，触发器检测）
 * 每个定时器ID (T0, T1, T2) 有统一的状态：
 * - SM_EFFECT_TIMER 启动定时器（设置 start_ms, running, duration_ms）
 * - SM_TRIGGER_TIMER 检测到期（比较 elapsed >= duration_ms）
 */
typedef struct {
    uint32_t start_ms;            /* 计时开始时间 */
    bool running;                 /* 是否正在计时 */
    uint32_t duration_ms;         /* 持续时间（由 SM_EFFECT_TIMER 设置） */
} sm_timer_state_t;

/* 定时器管理器
 * 管理所有定时器 (T0, T1, T2) 的状态
 */
typedef struct {
    sm_timer_state_t timers[SM_MAX_TIMERS];  /* T0, T1, T2 */
} sm_timer_manager_t;

/* 触发器类型 */
typedef enum {
    SM_TRIGGER_NONE = 0,       /* 无自动触发 */
    SM_TRIGGER_EDGE,           /* 边沿检测 (按钮按下) */
    SM_TRIGGER_LEVEL,          /* 电平条件检测 (温度/电量阈值) */
    SM_TRIGGER_TIMER,          /* 定时器到期 */
    SM_TRIGGER_MANUAL,         /* 手动/API 触发 */
} sm_trigger_type_t;

/* 
 * 触发器组合逻辑类型
 */
typedef enum {
    SM_LOGIC_AND = 0,          /* 默认: 所有触发器必须满足 (AND逻辑) */
    SM_LOGIC_OR  = 1          /* 任一触发器满足即可 (OR逻辑) */
} sm_logic_t;

/* 
 * 电平触发器比较运算符
 */
typedef enum {
    SM_OP_EQ = 0,              /* == 等于 (默认) */
    SM_OP_LT,                  /* <  小于 */
    SM_OP_GT,                  /* >  大于 */
    SM_OP_LE,                  /* <= 小于等于 */
    SM_OP_GE,                  /* >= 大于等于 */
    SM_OP_NE,                  /* != 不等于 */
    SM_OP_COUNT                /* 枚举计数，用于边界检查 */
} sm_operator_t;

/*
 * 触发器结构
 *
 * source 字段格式:
 *   - "P1"-"P12": GPIO 组 (EDGE/LEVEL 类型)
 *   - "T0"-"T2": 定时器索引 (TIMER 类型)
 *   - "MANUAL": 手动触发 (MANUAL 类型)
 */
typedef struct {
    sm_trigger_type_t type;                      /* EDGE/LEVEL/TIMER/MANUAL */
    char source[SM_MAX_TRIGGER_SOURCE_LEN];      /* P1-P12/T0-T2/MANUAL */
    sm_operator_t op;                          /* == < > <= >= != */
    int value;                                   /* 按钮电平/传感器值/定时器秒 */
} sm_trigger_t;

/* 
 * 状态转换结构
 */
typedef struct {
    char current_state[SM_MAX_STATE_NAME_LEN];
    char next_state[SM_MAX_STATE_NAME_LEN];
    sm_trigger_t triggers[3];  /* 触发器条件数组 */
    uint8_t trigger_count;     /* 1-3 */
    sm_logic_t trigger_logic;  /* AND/OR 逻辑 (默认: AND) */
} sm_transition_t;

/* 效果类型 */
typedef enum {
    SM_EFFECT_NONE = 0,      /* 无效果 */
    SM_EFFECT_OUTPUT,        /* GPIO 输出控制 */
    SM_EFFECT_TIMER,         /* 启动定时器 */
    SM_EFFECT_MOTOR,         /* 电机控制 */
    SM_EFFECT_NOTIFICATION   /* 日志通知 */
} sm_effect_type_t;

/* 
 * 状态效果
 */
typedef struct {
    char state_name[SM_MAX_STATE_NAME_LEN];
    sm_effect_type_t effect_type;
    char target[SM_MAX_EFFECT_TARGET_LEN];       /* GPIO组ID、定时器ID、电机ID */
    int value;                                   /* 输出值或定时器持续时间秒 */
} sm_state_effect_t;

/*
 * 状态机FSM配置结构
 *
 * 注意: 本结构仅包含配置数据，运行时状态由 sm_core.c 中的 g_sm 维护
 */
typedef struct {
    /* 状态 - 启动时的初始状态 */
    char initial_state[SM_MAX_STATE_NAME_LEN];

    /* 转换规则 */
    sm_transition_t transitions[SM_MAX_TRANSITIONS];
    uint32_t transition_count;

    /* 效果 */
    sm_state_effect_t effects[SM_MAX_EFFECTS];
    uint32_t effect_count;

    /* GPIO 组配置 - 完全集中式 */
    uint8_t group_modes[SM_MAX_GPIO_GROUPS];                      /* 0=输出, 1=输入 */
    sm_input_module_type_t input_module_types[SM_MAX_GPIO_GROUPS]; /* 输入模块类型 */
    sm_edge_type_t edge_types[SM_MAX_GPIO_GROUPS];                  /* 边沿检测类型 */
    sm_output_module_type_t output_module_types[SM_MAX_GPIO_GROUPS]; /* 输出模块类型 */
} sm_config_t;

#ifdef __cplusplus
}
#endif

#endif /* SM_TYPES_H */
