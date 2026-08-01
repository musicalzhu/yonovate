// i2c_manager.h - 增强型I2C管理器头文件
#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include <stddef.h>
#include <stdio.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

// I2C配置
#define I2C_PORT           I2C_NUM_0
#define I2C_SDA_PIN        GPIO_NUM_21
#define I2C_SCL_PIN        GPIO_NUM_22
#define I2C_FREQ           50000  // 50kHz
#define I2C_DEFAULT_TIMEOUT_MS 1000

// I2C总线状态枚举
typedef enum {
    I2C_BUS_STATUS_OK = 0,      // 总线正常
    I2C_BUS_STATUS_SDA_LOW,     // SDA线路被拉低
    I2C_BUS_STATUS_SCL_LOW,     // SCL线路被拉低
    I2C_BUS_STATUS_BOTH_LOW,    // SDA和SCL线路都被拉低
    I2C_BUS_STATUS_TIMEOUT,     // 总线超时
    I2C_BUS_STATUS_INVALID      // 总线状态无效
} i2c_bus_status_t;

/**
 * @brief 初始化I2C总线
 *
 * @return esp_err_t ESP_OK表示成功，其他表示失败
 */
esp_err_t i2c_manager_init(void);

/**
 * @brief 获取I2C主设备总线句柄
 * 
 * @return i2c_master_bus_handle_t I2C总线句柄
 */
i2c_master_bus_handle_t i2c_manager_get_bus_handle(void);

/**
 * @brief 锁定I2C总线
 * 
 * @return esp_err_t ESP_OK表示成功，其他表示失败
 */
esp_err_t i2c_manager_lock(void);

/**
 * @brief 解锁I2C总线
 * 
 * @return esp_err_t ESP_OK表示成功，其他表示失败
 */
esp_err_t i2c_manager_unlock(void);

/**
 * @brief 检查当前是否已持有I2C锁
 * 
 * @return bool true表示已持有锁，false表示未持有
 */
bool i2c_manager_is_locked(void);

/**
 * @brief 重置I2C总线（当总线被锁定时使用）
 * 
 * @return esp_err_t ESP_OK表示成功，其他表示失败
 */
esp_err_t i2c_manager_reset(void);

/**
 * @brief 请求后台重置I2C总线
 */
void i2c_manager_request_reset(void);

/**
 * @brief 检查I2C总线状态
 * 
 * @return bool true表示总线健康，false表示总线异常
 */
bool i2c_manager_is_healthy(void);

/**
 * @brief 报告I2C错误（增强版本，包含操作描述和设备地址）
 * 
 * @param error_code 错误代码
 * @param operation_desc 操作描述
 * @param dev_addr 设备地址
 */
void i2c_manager_report_error_ext(esp_err_t error_code, const char* operation_desc, uint8_t dev_addr);

/**
 * @brief 报告I2C错误（保持向后兼容性）
 * 
 * @param error_code 错误代码
 */
void i2c_manager_report_error(esp_err_t error_code);

/**
 * @brief 清除I2C总线错误计数
 */
void i2c_manager_clear_error_count(void);

/**
 * @brief 清除特定设备的错误计数
 * 
 * @param dev_addr 设备地址，如果为0则清除所有设备的错误计数
 */
void i2c_manager_clear_device_error_count(uint8_t dev_addr);

/**
 * @brief 执行I2C总线轻量级清理
 * 
 * 重置引脚配置，尝试释放总线，不执行完整的硬件重置
 */
void i2c_manager_cleanup_bus(void);

/**
 * @brief 执行I2C设备通信
 * 
 * @param dev_addr 设备地址
 * @param write_data 要写入的数据
 * @param write_size 要写入的数据大小
 * @param read_data 用于读取的数据缓冲区
 * @param read_size 要读取的数据大小
 * @param timeout_ms 超时时间（毫秒）
 * @return esp_err_t ESP_OK表示成功，其他表示失败
 */
esp_err_t i2c_manager_transfer(uint8_t dev_addr, const uint8_t *write_data, size_t write_size, uint8_t *read_data, size_t read_size, uint32_t timeout_ms);

/**
 * @brief 探测指定地址的I2C设备是否存在
 * 
 * @param dev_addr 设备地址
 * @return bool true表示设备存在，false表示设备不存在
 */
bool i2c_manager_probe_device(uint8_t dev_addr);

/**
 * @brief 尝试恢复指定I2C设备的通信
 * 
 * @param dev_addr 设备地址
 * @return esp_err_t ESP_OK表示成功，其他表示失败
 */
esp_err_t i2c_manager_recover_device(uint8_t dev_addr);

/**
 * @brief 安全执行I2C操作（带自动恢复）
 * 
 * @param operation 要执行的操作函数指针
 * @param user_data 用户数据
 * @return esp_err_t ESP_OK表示成功，其他表示失败
 */
esp_err_t i2c_manager_safe_execute(esp_err_t (*operation)(void *), void *user_data);

/**
 * @brief 重置AW9523设备的I2C通信
 * 
 * @return esp_err_t ESP_OK表示成功，其他表示失败
 */
esp_err_t i2c_manager_reset_aw9523(void);

#ifdef __cplusplus
}
#endif

#endif // I2C_MANAGER_H
