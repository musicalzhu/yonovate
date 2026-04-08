#ifndef AW9523_H
#define AW9523_H

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

// AW9523 I2C地址
#define AW9523_I2C_ADDR_DEFAULT    0x58  // 默认I2C地址

// AW9523寄存器定义
#define AW9523_REG_INPUT0         0x00  // 输入端口寄存器0
#define AW9523_REG_INPUT1         0x01  // 输入端口寄存器1
#define AW9523_REG_OUTPUT0        0x02  // 输出端口寄存器0
#define AW9523_REG_OUTPUT1        0x03  // 输出端口寄存器1
#define AW9523_REG_CONFIG0        0x04  // 配置寄存器0（方向）
#define AW9523_REG_CONFIG1        0x05  // 配置寄存器1（方向）
#define AW9523_REG_INT0      0x06  // 中断使能寄存器0
#define AW9523_REG_INT1      0x07  // 中断使能寄存器1
#define AW9523_REG_ID             0x10  // 设备ID寄存器
#define AW9523_REG_CTL            0x11  // 控制寄存器
#define AW9523_REG_LED_MODE0      0x12  // LED模式寄存器0
#define AW9523_REG_LED_MODE1      0x13  // LED模式寄存器1
#define AW9523_REG_RST            0x7F
#define AW9523_RST_RST            0x01  // 软复位位

// AW9523常量定义
#define AW9523_NUM_PORTS          16    // 总共有16个GPIO端口
#define AW9523_NUM_EXT_PORTS      8     // 扩展的8个GPIO端口(E1-E8)
#define AW9523_EXT_PORT_E1        8     // E1对应的端口号
#define AW9523_EXT_PORT_E8        15    // E8对应的端口号
#define AW9523_DEVICE_ID          0x23  // 设备ID值

#define AW9523_RST_RST            0x01  // 软复位位

// GPIO模式定义
#define AW9523_GPIO_MODE_INPUT    1     // 输入模式
#define AW9523_GPIO_MODE_OUTPUT   0     // 输出模式

// LED模式定义
#define AW9523_LED_MODE_DISABLED  0     // LED模式禁用（GPIO模式）
#define AW9523_LED_MODE_ENABLED   1     // LED模式启用

/**
 * @brief AW9523设备句柄前向声明
 */
struct aw9523_handle_s;
typedef struct aw9523_handle_s aw9523_handle_t;

/**
 * @brief AW9523初始化配置
 */
typedef struct {
    uint8_t i2c_addr;                   // I2C设备地址，默认0x58
} aw9523_config_t;

/**
 * @brief AW9523设备句柄结构体定义
 */
struct aw9523_handle_s {
    i2c_master_bus_handle_t i2c_bus;    // I2C总线句柄
    uint8_t i2c_addr;                   // I2C设备地址
    i2c_master_dev_handle_t dev_handle; // I2C设备句柄
};

/**
 * @brief 重置AW9523芯片
 * 
 * @param handle AW9523句柄
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t aw9523_reset(aw9523_handle_t *handle);

/**
 * @brief 初始化AW9523芯片
 * 
 * @param config AW9523初始化配置
 * @param handle 输出参数，返回初始化后的AW9523句柄
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t aw9523_init(const aw9523_config_t *config, aw9523_handle_t **handle);

/**
 * @brief 释放AW9523资源
 * 
 * @param handle AW9523句柄
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t aw9523_deinit(aw9523_handle_t *handle);

/**
 * @brief 设置AW9523指定端口的模式
 * 
 * @param handle AW9523句柄
 * @param port_num 端口号(0-15)
 * @param mode 模式(AW9523_GPIO_MODE_INPUT或AW9523_GPIO_MODE_OUTPUT)
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t aw9523_set_port_mode(aw9523_handle_t *handle, uint8_t port_num, uint8_t mode);

/**
 * @brief 设置AW9523指定端口的输出电平
 * 
 * @param handle AW9523句柄
 * @param port_num 端口号(0-15)
 * @param level 电平(0或1)
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t aw9523_set_port_level(aw9523_handle_t *handle, uint8_t port_num, uint8_t level);

/**
 * @brief 读取AW9523指定端口的输入电平
 * 
 * @param handle AW9523句柄
 * @param port_num 端口号(0-15)
 * @param level 输出参数，返回读取的电平值(0或1)
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t aw9523_get_port_level(aw9523_handle_t *handle, uint8_t port_num, uint8_t *level);


/**
 * @brief 读取AW9523设备ID
 * 
 * @param handle AW9523句柄
 * @param device_id 输出参数，返回设备ID
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t aw9523_get_device_id(aw9523_handle_t *handle, uint8_t *device_id);

/**
 * @brief 重置AW9523芯片
 * 
 * @param handle AW9523句柄
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t aw9523_reset(aw9523_handle_t *handle);

/**
 * @brief 设置AW9523指定端口的LED模式
 * 
 * @param handle AW9523句柄
 * @param port_num 端口号(0-15)
 * @param led_mode LED模式(AW9523_LED_MODE_DISABLED或AW9523_LED_MODE_ENABLED)
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t aw9523_set_port_led_mode(aw9523_handle_t *handle, uint8_t port_num, uint8_t led_mode);

/**
 * @brief 关闭所有端口的LED模式，设置为GPIO模式
 * 
 * @param handle AW9523句柄
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t aw9523_disable_all_led_modes(aw9523_handle_t *handle);

/**
 * @brief 设置AW9523指定端口的恒流亮度（类似Adafruit库的analogWrite功能）
 * 
 * @param handle AW9523句柄
 * @param port_num 端口号(0-15)
 * @param brightness 亮度值(0-255)
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t aw9523_analog_write(aw9523_handle_t *handle, uint8_t port_num, uint8_t brightness);

/**
 * @brief 设置AW9523端口0的开漏模式
 * 
 * @param handle AW9523句柄
 * @param enable 是否启用开漏模式(true/false)
 * @return esp_err_t ESP_OK成功，其他值失败
 */
esp_err_t aw9523_set_port0_open_drain(aw9523_handle_t *handle, bool enable);

#ifdef __cplusplus
}
#endif

#endif // AW9523_H