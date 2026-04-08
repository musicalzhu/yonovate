#include "gpio_manager.h"
#include <cstdio>
#include <cstring>
#include "esp_log.h"
#include "oled.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "aw9523.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "i2c_manager.h"
#include "driver/ledc.h"



// 确保C++函数能被C代码正确链接
extern "C" {

// ADC 句柄
static adc_oneshot_unit_handle_t adc1_handle = NULL;

static const char *TAG = "gpio_manager";

// GPIO管理器互斥锁，用于保护共享资源访问
static SemaphoreHandle_t s_gpio_mutex = NULL;
#define GPIO_MUTEX_WAIT_TIME pdMS_TO_TICKS(100) // 锁等待时间100ms

// GPIO分组定义
// 每组2个GPIO，共4组，外加2组电机接口和2个按键
// 根据硬件设计：
// 第一组是GPIO 13, 12     (Group 0)
// 第二组是GPIO 15, 2      (Group 1)
// 第三组是GPIO 26, 25     (Group 2)
// 第四组是GPIO 27, 14     (Group 3)
// 电机1是GPIO 18(DIR), 19(PWM)      (Motor 0)
// 电机2是GPIO 23(DIR), 16(PWM)      (Motor 1)
// 按键A是GPIO 34         (Button A)
// 按键B是GPIO 35         (Button B)
// 电压检测是GPIO 36      (ADC)

static gpio_group_t s_gpio_groups[GPIO_GROUP_COUNT] = {
    {.pins = {GPIO_NUM_13, GPIO_NUM_12},
     .is_input = {false, false},
     .states = {0, 0}},        // Group 0 (原生)
    {.pins = {GPIO_NUM_15, GPIO_NUM_2},
     .is_input = {false, false},
     .states = {0, 0}},        // Group 1 (原生)
    {.pins = {GPIO_NUM_26, GPIO_NUM_25},
     .is_input = {false, false},
     .states = {0, 0}},        // Group 2 (原生)
    {.pins = {GPIO_NUM_27, GPIO_NUM_14},
     .is_input = {false, false},
     .states = {0, 0}},        // Group 3 (原生)
    {.pins = {GPIO_NUM_MAX, GPIO_NUM_MAX}, // 用于标识这是AW9523扩展端口
     .is_input = {false, false},
     .states = {0, 0}},        // Group 4 (AW9523 E1)
    {.pins = {GPIO_NUM_MAX, GPIO_NUM_MAX},
     .is_input = {false, false},
     .states = {0, 0}},        // Group 5 (AW9523 E2)
    {.pins = {GPIO_NUM_MAX, GPIO_NUM_MAX},
     .is_input = {false, false},
     .states = {0, 0}},        // Group 6 (AW9523 E3)
    {.pins = {GPIO_NUM_MAX, GPIO_NUM_MAX},
     .is_input = {false, false},
     .states = {0, 0}},        // Group 7 (AW9523 E4)
    {.pins = {GPIO_NUM_MAX, GPIO_NUM_MAX},
     .is_input = {false, false},
     .states = {0, 0}},        // Group 8 (AW9523 E5)
    {.pins = {GPIO_NUM_MAX, GPIO_NUM_MAX},
     .is_input = {false, false},
     .states = {0, 0}},        // Group 9 (AW9523 E6)
    {.pins = {GPIO_NUM_MAX, GPIO_NUM_MAX},
     .is_input = {false, false},
     .states = {0, 0}},        // Group 10 (AW9523 E7)
    {.pins = {GPIO_NUM_MAX, GPIO_NUM_MAX},
     .is_input = {false, false},
     .states = {0, 0}},        // Group 11 (AW9523 E8)
};

static motor_group_t s_motor_groups[MOTOR_GROUP_COUNT] = {
    {.pins = {GPIO_NUM_18, GPIO_NUM_19},
     .states = {0, 0}},        // Motor 0
    {.pins = {GPIO_NUM_23, GPIO_NUM_16},
     .states = {0, 0}},        // Motor 1
};

static button_t s_buttons[BUTTON_COUNT] = {
    {.pin = GPIO_NUM_34, .state = 0},    // Button A
    {.pin = GPIO_NUM_35, .state = 0},    // Button B
};


// AW9523设备句柄
extern "C" aw9523_handle_t *g_aw9523_handle;
aw9523_handle_t *g_aw9523_handle = NULL;

// PWM (LEDC) 配置用于电机速度控制
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT  // 8位分辨率 (0-255)
#define LEDC_FREQUENCY          (5000)            // 5kHz PWM频率

// LEDC通道分配
#define LEDC_CHANNEL_MOTOR0     LEDC_CHANNEL_0    // M1 PWM引脚
#define LEDC_CHANNEL_MOTOR1     LEDC_CHANNEL_1    // M2 PWM引脚

// 初始化标志
static bool s_initialized = false;

esp_err_t gpio_manager_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }
    
    // 创建GPIO管理器互斥锁
    s_gpio_mutex = xSemaphoreCreateMutex();
    if (s_gpio_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create GPIO mutex");
        return ESP_FAIL;
    }
    
    // 设置初始化标志为true，这样内部函数调用也能正常工作
    s_initialized = true;
    
    ESP_LOGI(TAG, "GPIO manager initialization started");
    
    // 初始化原生GPIO组 (0-3)
    for (int group_id = 0; group_id < AW9523_GROUP_START; group_id++) {
        for (int pin_index = 0; pin_index < GPIO_PER_GROUP; pin_index++) {
            gpio_num_t gpio_num = s_gpio_groups[group_id].pins[pin_index];
            
            // 配置为输出模式并初始化为低电平
            gpio_config_t io_conf = {};
            io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.mode = GPIO_MODE_OUTPUT;
            io_conf.pin_bit_mask = (1ULL << gpio_num);
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;  // 输出模式默认下拉
            
            esp_err_t ret = gpio_config(&io_conf);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to configure GPIO %d", gpio_num);
                return ret;
            }
            
            // 初始化为低电平
            gpio_set_level(gpio_num, 0);
            s_gpio_groups[group_id].states[pin_index] = 0;
        }
    }
    
    // 初始化AW9523
    if (g_aw9523_handle == NULL) {
        aw9523_config_t config = {
            .i2c_addr = AW9523_I2C_ADDR_DEFAULT
        };
        ESP_LOGI(TAG, "Initializing AW9523 module, I2C address: 0x%02X", config.i2c_addr);
        esp_err_t result = aw9523_init(&config, &g_aw9523_handle);
        if (result != ESP_OK || g_aw9523_handle == NULL) {
            ESP_LOGW(TAG, "AW9523 initialization failed: %s", result != ESP_OK ? esp_err_to_name(result) : "handle is NULL");
            g_aw9523_handle = NULL;
            // 即使AW9523初始化失败，也继续执行，以便原生GPIO能正常工作
        } else {
            ESP_LOGI(TAG, "AW9523 initialized successfully");
            // 初始化成功，设置所有扩展端口为输出模式并初始化为低电平
            
            // 关闭所有端口的LED模式和电流源，设置为GPIO模式
            ESP_LOGI(TAG, "Disabling all AW9523 port LED modes and current sources");
            esp_err_t disable_result = aw9523_disable_all_led_modes(g_aw9523_handle);
            if (disable_result != ESP_OK) {
                ESP_LOGE(TAG, "Failed to disable AW9523 LED modes: %s", esp_err_to_name(disable_result));
            }
            
            // 设置所有端口为输出模式并初始化为低电平
            for (int i = 0; i < 16; i++) {
                esp_err_t mode_result = aw9523_set_port_mode(g_aw9523_handle, i, AW9523_GPIO_MODE_OUTPUT);
                if (mode_result != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to set AW9523 port %d mode: %s", i, esp_err_to_name(mode_result));
                }
                
                esp_err_t level_result = aw9523_set_port_level(g_aw9523_handle, i, 0);
                if (level_result != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to set AW9523 port %d level: %s", i, esp_err_to_name(level_result));
                }
            }
            
        }
    }
    
    // 初始化电机接口
    // 配置LEDC定时器用于PWM
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    esp_err_t ledc_ret = ledc_timer_config(&ledc_timer);
    if (ledc_ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC定时器配置失败: %s", esp_err_to_name(ledc_ret));
        return ledc_ret;
    }
    
    for (int motor_id = 0; motor_id < MOTOR_GROUP_COUNT; motor_id++) {
        // DIR引脚 (pins[0]) - 普通GPIO
        gpio_num_t dir_pin = s_motor_groups[motor_id].pins[0];
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = (1ULL << dir_pin);
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        
        esp_err_t ret = gpio_config(&io_conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "电机DIR GPIO配置失败: motor %d, error %s", 
                     motor_id, esp_err_to_name(ret));
            return ret;
        }
        gpio_set_level(dir_pin, 0);
        s_motor_groups[motor_id].states[0] = 0;
        
        // PWM引脚 (pins[1]) - LEDC配置
        gpio_num_t pwm_pin = s_motor_groups[motor_id].pins[1];
        ledc_channel_t channel = (motor_id == 0) ? LEDC_CHANNEL_MOTOR0 : LEDC_CHANNEL_MOTOR1;
        
        ledc_channel_config_t ledc_channel = {
            .gpio_num       = (int)pwm_pin,
            .speed_mode     = LEDC_MODE,
            .channel        = channel,
            .intr_type      = LEDC_INTR_DISABLE,
            .timer_sel      = LEDC_TIMER,
            .duty           = 0,  // 初始占空比为0
            .hpoint         = 0,
            .flags          = {.output_invert = 0}
        };
        ret = ledc_channel_config(&ledc_channel);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "电机PWM LEDC配置失败: motor %d, error %s", 
                     motor_id, esp_err_to_name(ret));
            return ret;
        }
        s_motor_groups[motor_id].states[1] = 0;
    }
    
    // 初始化按键（输入模式默认上拉）
    for (int button_id = 0; button_id < BUTTON_COUNT; button_id++) {
        gpio_num_t gpio_num = s_buttons[button_id].pin;
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_ANYEDGE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = (1ULL << gpio_num);
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;   // 输入模式默认上拉
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        esp_err_t ret = gpio_config(&io_conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "按键GPIO配置失败: button %d, error %s", 
                     button_id, esp_err_to_name(ret));
            return ret;
        }
    }
    
    // 初始化ADC (电压检测)，避免重复初始化
    if (adc1_handle == NULL) {
        adc_oneshot_unit_init_cfg_t init_config;
        init_config.unit_id = ADC_UNIT_1;
        init_config.clk_src = ADC_RTC_CLK_SRC_DEFAULT;
        init_config.ulp_mode = ADC_ULP_MODE_DISABLE;
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

        adc_oneshot_chan_cfg_t config;
        config.atten = ADC_ATTEN_DB_12;  // 使用12dB衰减以支持更高的输入电压范围
        config.bitwidth = ADC_BITWIDTH_DEFAULT;
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));
    }
    
    // s_initialized = true;  // 已移到函数开头
    return ESP_OK;
}

esp_err_t gpio_manager_get_group_info(uint8_t group_id, gpio_group_t* group_info)
{
    if (!s_initialized || group_id >= GPIO_GROUP_COUNT || group_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(group_info, &s_gpio_groups[group_id], sizeof(gpio_group_t));
    return ESP_OK;
}

esp_err_t gpio_manager_get_motor_info(uint8_t motor_id, motor_group_t* motor_info)
{
    if (!s_initialized || motor_id >= MOTOR_GROUP_COUNT || motor_info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(motor_info, &s_motor_groups[motor_id], sizeof(motor_group_t));
    return ESP_OK;
}

esp_err_t gpio_manager_set_motor(uint8_t motor_id, int8_t speed_direction)
{
    if (!s_initialized || motor_id >= MOTOR_GROUP_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }

    // 检查速度方向值是否有效 (-9到9)
    if (speed_direction < -9 || speed_direction > 9) {
        return ESP_ERR_INVALID_ARG;
    }

    // 获取LEDC通道
    ledc_channel_t channel = (motor_id == 0) ? LEDC_CHANNEL_MOTOR0 : LEDC_CHANNEL_MOTOR1;
    gpio_num_t dir_pin = s_motor_groups[motor_id].pins[0];
    
    // pins[0] = DIR (方向), pins[1] = PWM (速度)
    if (speed_direction == 0) {
        // 停止: DIR=0, PWM占空比=0
        gpio_set_level(dir_pin, 0);
        ledc_set_duty(LEDC_MODE, channel, 0);
        ledc_update_duty(LEDC_MODE, channel);
        s_motor_groups[motor_id].states[0] = 0;
        s_motor_groups[motor_id].states[1] = 0;
    } else if (speed_direction > 0) {
        // 正转 (1-9: 10%-90%)
        // DIR=0 (正转), PWM占空比根据速度设置
        gpio_set_level(dir_pin, 0);
        // 计算占空比: speed_direction=1->10%, speed_direction=9->90%
        // 8位PWM: 255 * (speed_direction * 10) / 100
        uint32_t duty = (255 * speed_direction * 10) / 100;
        ledc_set_duty(LEDC_MODE, channel, duty);
        ledc_update_duty(LEDC_MODE, channel);
        s_motor_groups[motor_id].states[0] = 0;
        s_motor_groups[motor_id].states[1] = 1;
    } else {
        // 反转 (-1到-9: 10%-90%)
        // DIR=1 (反转), PWM占空比根据速度设置
        gpio_set_level(dir_pin, 1);
        // 计算占空比: abs(speed_direction)=1->10%, abs(speed_direction)=9->90%
        int8_t abs_speed = -speed_direction;  // 转为正数
        uint32_t duty = (255 * abs_speed * 10) / 100;
        ledc_set_duty(LEDC_MODE, channel, duty);
        ledc_update_duty(LEDC_MODE, channel);
        s_motor_groups[motor_id].states[0] = 1;
        s_motor_groups[motor_id].states[1] = 1;
    }

    return ESP_OK;
}

esp_err_t gpio_manager_get_button_state(uint8_t button_id, int* state)
{
    if (!s_initialized || button_id >= BUTTON_COUNT || state == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *state = gpio_get_level(s_buttons[button_id].pin);
    s_buttons[button_id].state = *state;

    return ESP_OK;
}

esp_err_t gpio_manager_get_voltage(uint32_t* voltage)
{
    if (!s_initialized || voltage == NULL || adc1_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 读取ADC值
    int adc_value;
    esp_err_t ret = adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_value);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC读取失败: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 将ADC值转换为电压值 (mV)
    // 假设12位ADC，参考电压约3300mV，11dB衰减下最大输入约3.6V
    // 根据你之前提到的30K/30K分压，实际电压需要乘以2
    float voltage_mv = (float)adc_value * 3300.0f / 4095.0f * 2.0f;
    *voltage = (uint32_t)voltage_mv;

    return ESP_OK;
}

esp_err_t gpio_manager_set_pin_mode(uint8_t group_id, uint8_t pin_index, bool is_input)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (group_id >= GPIO_GROUP_COUNT || pin_index >= GPIO_PER_GROUP) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 更新内部状态跟踪
    s_gpio_groups[group_id].is_input[pin_index] = is_input;
    
    // 判断是否为AW9523扩展端口
    if (group_id >= AW9523_GROUP_START && group_id < AW9523_GROUP_START + AW9523_GROUP_COUNT) {
        // 检查AW9523是否初始化成功
        if (g_aw9523_handle == NULL) {
            // AW9523未连接或初始化失败，记录日志并返回成功
            ESP_LOGW(TAG, "AW9523 module not connected, skip port mode setting: group %d, pin %d", 
                      group_id, pin_index);
            return ESP_OK; // 不返回错误，让系统继续运行
        }
        
        // 计算AW9523的端口号
        uint8_t aw9523_port;
        // 根据用户确认的正确映射关系
        aw9523_port = (group_id - AW9523_GROUP_START) * 2 + (1 - pin_index);
        
        ESP_LOGI(TAG, "AW9523 port mapping: group=%d, pin=%d => port=%d, 模式=%s", 
                 group_id, pin_index, aw9523_port, is_input ? "输入" : "输出");
        
        
    
        
        // 设置AW9523端口模式
        uint8_t mode = is_input ? AW9523_GPIO_MODE_INPUT : AW9523_GPIO_MODE_OUTPUT;
        int result = aw9523_set_port_mode(g_aw9523_handle, aw9523_port, mode);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set AW9523 port mode: group %d, pin %d, error %s", 
                      group_id, pin_index, esp_err_to_name(result));
            return result;
        }
        
        // 如果是输出模式，初始化为低电平
        if (!is_input) {
            aw9523_set_port_level(g_aw9523_handle, aw9523_port, 0);
            s_gpio_groups[group_id].states[pin_index] = 0;
        }
        
        ESP_LOGI(TAG, "AW9523 port mode set: group %d, pin %d, mode %s", 
                    group_id, pin_index, is_input ? "input" : "output");
        return ESP_OK;
    }
    
    // ESP32原生GPIO引脚模式设置
    gpio_num_t gpio_num = s_gpio_groups[group_id].pins[pin_index];
    
    // 检查GPIO是否有效
    if (gpio_num == GPIO_NUM_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 重新配置GPIO
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = is_input ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << gpio_num);
    // 输入模式：默认上拉（拉高），输出模式：默认下拉（拉低）
    io_conf.pull_up_en = is_input ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = is_input ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE;
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO模式设置失败: group %d, pin %d, error %s", 
                    group_id, pin_index, esp_err_to_name(ret));
        return ret;
    }
    
    // 如果是输出模式，初始化为低电平
    if (!is_input) {
        gpio_set_level(gpio_num, 0);
        s_gpio_groups[group_id].states[pin_index] = 0;
    }
    
    //ESP_LOGI(TAG, "GPIO模式设置: group %d, pin %d, mode %s", 
    //            group_id, pin_index, is_input ? "input" : "output");
    return ESP_OK;
    
}

esp_err_t gpio_manager_set_group_mode(uint8_t group_id, bool is_input)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (group_id >= GPIO_GROUP_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 为组内所有引脚设置相同的模式
    esp_err_t ret = ESP_OK;
    for (int i = 0; i < GPIO_PER_GROUP; i++) {
        esp_err_t pin_ret = gpio_manager_set_pin_mode(group_id, i, is_input);
        if (pin_ret != ESP_OK) {
            ret = pin_ret; // 记录最后一个错误
        }
    }
    
    return ret;
}

// 获取GPIO管理器锁
static esp_err_t gpio_manager_lock(void)
{
    if (!s_initialized || s_gpio_mutex == NULL) {
        return ESP_FAIL;
    }
    
    if (xSemaphoreTake(s_gpio_mutex, GPIO_MUTEX_WAIT_TIME) != pdTRUE) {
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

// 释放GPIO管理器锁
static esp_err_t gpio_manager_unlock(void)
{
    if (!s_initialized || s_gpio_mutex == NULL) {
        return ESP_FAIL;
    }
    
    if (xSemaphoreGive(s_gpio_mutex) != pdTRUE) {
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t gpio_manager_set_level(uint8_t group_id, uint8_t pin_index, uint32_t level)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (group_id >= GPIO_GROUP_COUNT || pin_index >= GPIO_PER_GROUP) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 获取锁保护状态更新
    esp_err_t ret = gpio_manager_lock();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 检查是否为输出模式
    if (s_gpio_groups[group_id].is_input[pin_index]) {
        ESP_LOGW(TAG, "Attempt to set level on input pin: group %d, pin %d", group_id, pin_index);
        gpio_manager_unlock();
        return ESP_ERR_INVALID_ARG;
    }
    
    // 判断是否为AW9523扩展端口
    if (group_id >= AW9523_GROUP_START && group_id < AW9523_GROUP_START + AW9523_GROUP_COUNT) {
        // 检查AW9523是否初始化成功
        if (g_aw9523_handle == NULL) {
            ESP_LOGW(TAG, "AW9523 module not connected, skip port level setting: group %d, pin %d", group_id, pin_index);
            // 更新内部状态，以便系统可以继续运行
            s_gpio_groups[group_id].states[pin_index] = level;
            // 释放锁
            gpio_manager_unlock();
            return ESP_OK; // 不返回错误，让系统继续运行
        }
        
        // 计算AW9523的端口号
        uint8_t aw9523_port;
        // 使用所有端口(0-15)，组4-7映射到端口0-7，组8-11映射到端口8-15
        aw9523_port = (group_id - AW9523_GROUP_START) * 2 + (1 - pin_index);
        ESP_LOGD(TAG, "AW9523 port mapping (FULL mode): group=%d, pin=%d => port=%d", 
                 group_id, pin_index, aw9523_port);
        
        
              
        // 设置AW9523端口电平
        int result = aw9523_set_port_level(g_aw9523_handle, aw9523_port, level);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set AW9523 port level: group %d, pin %d, error %s", 
                        group_id, pin_index, esp_err_to_name(result));
            gpio_manager_unlock();
            return result;
        }
        
        // 更新状态
        s_gpio_groups[group_id].states[pin_index] = level;
        
        // 释放锁
        gpio_manager_unlock();
        return ESP_OK;
    }
    
    // ESP32原生GPIO引脚电平设置
    gpio_num_t gpio_num = s_gpio_groups[group_id].pins[pin_index];
    
    // 检查GPIO是否有效
    if (gpio_num == GPIO_NUM_MAX) {
        gpio_manager_unlock();
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t gpio_ret = gpio_set_level(gpio_num, level);
    if (gpio_ret != ESP_OK) {
        gpio_manager_unlock();
        return gpio_ret;
    }
    
    // 更新状态
    s_gpio_groups[group_id].states[pin_index] = level;
    
    // 释放锁
    gpio_manager_unlock();
    return ESP_OK;
}

esp_err_t gpio_manager_get_level(uint8_t group_id, uint8_t pin_index, int* level)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (group_id >= GPIO_GROUP_COUNT || pin_index >= GPIO_PER_GROUP || level == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 获取锁保护状态读取
    esp_err_t ret = gpio_manager_lock();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 判断是否为AW9523扩展端口
    if (group_id >= AW9523_GROUP_START && group_id < AW9523_GROUP_START + AW9523_GROUP_COUNT) {
        // 检查AW9523是否初始化成功
        if (g_aw9523_handle == NULL) {
            ESP_LOGW(TAG, "AW9523 module not connected, return default level: group %d, pin %d", group_id, pin_index);
            *level = 0; // 返回默认值
            // 更新状态
            s_gpio_groups[group_id].states[pin_index] = *level;
            // 释放锁
            gpio_manager_unlock();
            return ESP_OK; // 不返回错误，让系统继续运行
        }
        

        uint8_t aw9523_port = (group_id - AW9523_GROUP_START) * 2 + (1 - pin_index);
        
        ESP_LOGD(TAG, "读取AW9523端口电平: group=%d, pin=%d => port=%d, 输入模式=%d", 
                 group_id, pin_index, aw9523_port, s_gpio_groups[group_id].is_input[pin_index]);
        
        if (s_gpio_groups[group_id].is_input[pin_index]) {
            // 输入模式，读取实际电平
            uint8_t port_level = 0;
            int result = aw9523_get_port_level(g_aw9523_handle, aw9523_port, &port_level);
            if (result != ESP_OK) {
                *level = 0;
                gpio_manager_unlock();
                // 向I2C管理器报告错误
                i2c_manager_report_error(result);
                return ESP_FAIL;
            }
            *level = port_level;
        } else {
            // 输出模式，返回保存的状态
            *level = s_gpio_groups[group_id].states[pin_index];
        }
        
        // 更新状态
        s_gpio_groups[group_id].states[pin_index] = *level;
        
        // 释放锁
        gpio_manager_unlock();
        return ESP_OK;
    }
    
    // ESP32原生GPIO引脚电平读取
    gpio_num_t gpio_num = s_gpio_groups[group_id].pins[pin_index];
    
    // 检查GPIO是否有效
    if (gpio_num == GPIO_NUM_MAX) {
        *level = 0;
        gpio_manager_unlock();
        return ESP_OK;
    }
    
    if (s_gpio_groups[group_id].is_input[pin_index]) {
        // 输入模式，读取实际电平
        *level = gpio_get_level(gpio_num);
    } else {
        // 输出模式，返回保存的状态
        *level = s_gpio_groups[group_id].states[pin_index];
    }
    
    // 更新状态
    s_gpio_groups[group_id].states[pin_index] = *level;
    
    // 释放锁
    gpio_manager_unlock();
    return ESP_OK;
}

esp_err_t gpio_manager_update_states(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // 获取锁保护状态更新
    esp_err_t ret = gpio_manager_lock();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 无需跟踪错误，函数总是返回ESP_OK
    
    // 更新所有GPIO组状态
    for (int group_id = 0; group_id < GPIO_GROUP_COUNT; group_id++) {
        for (int pin_index = 0; pin_index < GPIO_PER_GROUP; pin_index++) {
            // 判断是否为AW9523扩展端口
            if (group_id >= AW9523_GROUP_START && group_id < AW9523_GROUP_START + AW9523_GROUP_COUNT) {
                // 检查AW9523是否初始化成功
                if (g_aw9523_handle == NULL) {
                    // AW9523未连接，跳过更新，保持内部状态不变
                    ESP_LOGV(TAG, "AW9523 module not connected, skip port state update: group %d, pin %d", group_id, pin_index);
                } else if (s_gpio_groups[group_id].is_input[pin_index]) {
                    // 计算AW9523的端口号
                    uint8_t aw9523_port;
                    aw9523_port = (group_id - AW9523_GROUP_START) * 2 + (1 - pin_index);
                   
                    
                    // 读取AW9523端口电平
                    uint8_t port_level = 0;
                    int result = aw9523_get_port_level(g_aw9523_handle, aw9523_port, &port_level);
                    if (result == ESP_OK) {
                        s_gpio_groups[group_id].states[pin_index] = port_level;
                    } else {
                        // 向I2C管理器报告错误
                        i2c_manager_report_error(result);
                        ESP_LOGE(TAG, "Failed to read AW9523 port level: group %d, pin %d", group_id, pin_index);
                        // 继续更新其他端口
                    }
                }
                continue;
            }
            
            // ESP32原生GPIO引脚状态更新
            gpio_num_t gpio_num = s_gpio_groups[group_id].pins[pin_index];
            
            // 检查GPIO是否有效
            if (gpio_num == GPIO_NUM_MAX) {
                continue;
            }
            
            if (s_gpio_groups[group_id].is_input[pin_index]) {
                s_gpio_groups[group_id].states[pin_index] = gpio_get_level(gpio_num);
            }
            // 输出模式的状态已经在设置时更新，无需再次读取
        }
    }
    
    // 更新按键状态
    for (int button_id = 0; button_id < BUTTON_COUNT; button_id++) {
        s_buttons[button_id].state = gpio_get_level(s_buttons[button_id].pin);
    }
    
    // 释放锁
    gpio_manager_unlock();
    
    // 即使有个别端口读取失败，仍返回成功，确保系统继续运行
    return ESP_OK;
}

esp_err_t gpio_manager_get_pin_mode(uint8_t group_id, uint8_t pin_index, int* is_input)
{
    if (!s_initialized || is_input == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (group_id >= GPIO_GROUP_COUNT || pin_index >= GPIO_PER_GROUP) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 获取锁保护状态读取
    esp_err_t ret = gpio_manager_lock();
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 从内部状态获取引脚模式
    *is_input = s_gpio_groups[group_id].is_input[pin_index] ? 1 : 0;
    
    // 释放锁
    gpio_manager_unlock();
    return ESP_OK;
}
} // extern "C"