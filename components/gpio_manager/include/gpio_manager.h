#pragma once

#include "esp_err.h"
#include "driver/gpio.h"
#include "aw9523.h"

#ifdef __cplusplus
extern "C" {
#endif

// GPIO组定义
#define GPIO_GROUP_COUNT 12  // 4个原生组 + 8个AW9523扩展组
#define GPIO_PER_GROUP 2
#define MOTOR_GROUP_COUNT 2
#define BUTTON_COUNT 2

// AW9523扩展端口组定义
#define AW9523_GROUP_START 4  // AW9523扩展组起始ID
#define AW9523_GROUP_COUNT 8  // AW9523扩展组数量

// GPIO组结构
typedef struct {
    gpio_num_t pins[GPIO_PER_GROUP];  // 每组2个GPIO引脚
    bool is_input[GPIO_PER_GROUP];    // 每个引脚是否为输入模式
    int states[GPIO_PER_GROUP];       // 每个引脚的状态
} gpio_group_t;

// 电机组结构
typedef struct {
    gpio_num_t pins[GPIO_PER_GROUP];  // 每个电机2个控制引脚
    int states[GPIO_PER_GROUP];       // 每个引脚的状态
} motor_group_t;

// 按键结构
typedef struct {
    gpio_num_t pin;    // 按键引脚
    int state;         // 按键状态
} button_t;

/**
 * @brief 初始化GPIO管理器
 * 
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_manager_init(void);

/**
 * @brief 设置GPIO组中某个引脚的模式
 * 
 * @param group_id GPIO组ID (0-7)
 * @param pin_index 组内引脚索引 (0-3)
 * @param is_input true为输入模式，false为输出模式
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_manager_set_pin_mode(uint8_t group_id, uint8_t pin_index, bool is_input);

/**
 * @brief 获取GPIO组中某个引脚的模式
 * 
 * @param group_id GPIO组ID (0-7)
 * @param pin_index 组内引脚索引 (0-3)
 * @param is_input 用于存储模式的指针 (true为输入模式，false为输出模式)
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_manager_get_pin_mode(uint8_t group_id, uint8_t pin_index, int* is_input);

/**
 * @brief 设置整个GPIO组的模式
 * 
 * @param group_id GPIO组ID (0-7)
 * @param is_input true为输入模式，false为输出模式
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_manager_set_group_mode(uint8_t group_id, bool is_input);

/**
 * @brief 设置GPIO输出电平
 * 
 * @param group_id GPIO组ID (0-7)
 * @param pin_index 组内引脚索引 (0-3)
 * @param level 电平值 (0或1)
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_manager_set_level(uint8_t group_id, uint8_t pin_index, uint32_t level);

/**
 * @brief 读取GPIO输入电平
 * 
 * @param group_id GPIO组ID (0-7)
 * @param pin_index 组内引脚索引 (0-3)
 * @param level 用于存储电平值的指针
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_manager_get_level(uint8_t group_id, uint8_t pin_index, int* level);

/**
 * @brief 更新所有GPIO组的状态
 * 
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_manager_update_states(void);

/**
 * @brief 获取GPIO组信息
 * 
 * @param group_id GPIO组ID (0-3)
 * @param group_info 用于存储组信息的指针
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_manager_get_group_info(uint8_t group_id, gpio_group_t* group_info);

/**
 * @brief 获取电机组信息
 * 
 * @param motor_id 电机ID (0-1)
 * @param motor_info 用于存储电机信息的指针
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_manager_get_motor_info(uint8_t motor_id, motor_group_t* motor_info);

/**
 * @brief 设置电机状态和转速
 * 
 * @param motor_id 电机ID (0-1)
 * @param speed_direction 速度和方向
 *                        0: 停止
 *                        1-9: 正转，转速 10%-90%
 *                        -1到-9: 反转，转速 10%-90%
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_manager_set_motor(uint8_t motor_id, int8_t speed_direction);

/**
 * @brief 读取按键状态
 * 
 * @param button_id 按键ID (0:A, 1:B)
 * @param state 用于存储按键状态的指针
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_manager_get_button_state(uint8_t button_id, int* state);

/**
 * @brief 读取系统电压
 * 
 * @param voltage 用于存储电压值的指针 (mV)
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_manager_get_voltage(uint32_t* voltage);

/**
 * @brief 获取GPIO组的状态信息
 * 
 * @param group_id GPIO组ID (0-7)
 * @param group 用于存储组信息的指针
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t gpio_manager_get_group_info(uint8_t group_id, gpio_group_t* group);



#ifdef __cplusplus
}
#endif