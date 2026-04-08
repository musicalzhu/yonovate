// 确保正确包含所有必要的头文件
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "i2c_manager.h"
#include "aw9523.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "aw9523";

// 标准日志宏定义
#define AW9523_LOGI(format, ...) ESP_LOGI(TAG, format, ##__VA_ARGS__)
#define AW9523_LOGD(format, ...) ESP_LOGD(TAG, format, ##__VA_ARGS__)
#define AW9523_LOGE(format, ...) ESP_LOGE(TAG, format, ##__VA_ARGS__)
#define AW9523_LOGW(format, ...) ESP_LOGW(TAG, format, ##__VA_ARGS__)

// 辅助函数：从AW9523寄存器读取数据
static esp_err_t aw9523_read_register(aw9523_handle_t *handle, uint8_t reg_addr, uint8_t *data)
{
    if (!handle || !data) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!handle->i2c_bus || !handle->dev_handle) {
        // 设备未连接时，返回错误而不是成功
        AW9523_LOGE("Failed to read register 0x%02X: I2C device not connected", reg_addr);
        return ESP_ERR_NOT_FOUND;
    }
    
    // 使用i2c_manager_transfer函数执行I2C通信，增加超时时间提高稳定性
    esp_err_t ret = i2c_manager_transfer(handle->i2c_addr, &reg_addr, 1, data, 1, 2000);
    if (ret != ESP_OK) {
        AW9523_LOGE("Failed to read register 0x%02X: %s", reg_addr, esp_err_to_name(ret));
    }
    return ret;
}

// 辅助函数：向AW9523寄存器写入数据
static esp_err_t aw9523_write_register(aw9523_handle_t *handle, uint8_t reg_addr, uint8_t data)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!handle->i2c_bus || !handle->dev_handle) {
        // 设备未连接时，返回错误而不是成功
        AW9523_LOGE("Failed to write register 0x%02X: I2C device not connected", reg_addr);
        return ESP_ERR_NOT_FOUND;
    }
    
    // 准备写入数据
    uint8_t write_buf[2] = {reg_addr, data};
    
    // 使用i2c_manager_transfer函数执行I2C通信，增加超时时间提高稳定性
    return i2c_manager_transfer(handle->i2c_addr, write_buf, sizeof(write_buf), NULL, 0, 2000);
}




// 初始化AW9523芯片
esp_err_t aw9523_init(const aw9523_config_t *config, aw9523_handle_t **handle)
{
    if (!config || !handle) {
        return ESP_ERR_INVALID_ARG;
    }

    // 初始化I2C总线
    esp_err_t ret = i2c_manager_init();
    if (ret != ESP_OK) {
        // 处理总线已被占用的情况
        if (ret == ESP_ERR_INVALID_STATE) {
            AW9523_LOGI("I2C总线已被初始化，尝试继续使用现有总线");
            // 继续尝试初始化，不直接返回错误
        } else {
            AW9523_LOGI("I2C管理器初始化失败: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    // 分配AW9523句柄
    *handle = (aw9523_handle_t *)malloc(sizeof(aw9523_handle_t));
    if (!(*handle)) {
        return ESP_ERR_NO_MEM;
    }

    // 设置I2C总线句柄和设备地址
    (*handle)->i2c_bus = i2c_manager_get_bus_handle();
    (*handle)->i2c_addr = config->i2c_addr;
    (*handle)->dev_handle = NULL;

    // 增强的设备探测和恢复逻辑
    int max_probe_retries = 3;
    int probe_retry_count = 0;
    bool device_present = false;
    
    while (probe_retry_count < max_probe_retries && !device_present) {
        // 探测设备是否存在
        device_present = i2c_manager_probe_device(config->i2c_addr);
        if (device_present) {
            break;
        }
        
        AW9523_LOGI("未探测到AW9523设备(0x%02X)，第%d/%d次尝试", 
                 config->i2c_addr, probe_retry_count+1, max_probe_retries);
        
        // 尝试恢复设备 - 只在第一次尝试失败时执行重置
        if (probe_retry_count == 0) {
            AW9523_LOGI("执行AW9523专用重置流程...");
            // 调用AW9523重置函数
            esp_err_t reset_ret = aw9523_reset(*handle);
            if (reset_ret != ESP_OK) {
                AW9523_LOGW("重置尝试失败: %s", esp_err_to_name(reset_ret));
            }
            
            // 重置后等待更长时间
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            // 后续重试时执行完整的总线重置
            // 这里可以调用总线重置函数
            
            // 重置后等待
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        
        probe_retry_count++;
    }

    // 设备存在时，创建I2C设备
    if (device_present) {
        i2c_device_config_t i2c_dev_conf = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = (*handle)->i2c_addr,
            .scl_speed_hz = I2C_FREQ,     // 使用正常频率
            .scl_wait_us = 20,            // 适度的等待时间
            .flags = 0
        };
        
        ret = i2c_master_bus_add_device((*handle)->i2c_bus, &i2c_dev_conf, &((*handle)->dev_handle));
        if (ret != ESP_OK) {
            free(*handle);
            *handle = NULL;
            return ret;
        }

        // 读取设备ID以确认通信正常，但不强制要求匹配
        if ((*handle)->dev_handle) {
            uint8_t device_id = 0;
            ret = aw9523_get_device_id(*handle, &device_id);
            if (ret == ESP_OK) {
                AW9523_LOGI("设备ID: 0x%02X", device_id);
            } else {
                AW9523_LOGW("无法读取设备ID，但继续初始化");
                // 不设置dev_handle为NULL，继续尝试初始化
            }
        }
    } else {
        AW9523_LOGE("所有探测尝试均失败，无法初始化AW9523");
        free(*handle);
        *handle = NULL;
        return ESP_ERR_NOT_FOUND;
    }

    // 重置芯片到默认状态
    if ((*handle)->dev_handle) {
        ret = aw9523_reset(*handle);
        if (ret != ESP_OK) {
            AW9523_LOGE("Failed to reset AW9523");
            // 不释放句柄，继续执行
        }
    }


    // 关闭所有端口的LED模式，确保都是纯IO模式
    if ((*handle)->dev_handle) {
        // 1. 先禁用所有LED模式
        aw9523_disable_all_led_modes(*handle);
        
        // 2. 增加额外延时确保配置生效
        vTaskDelay(pdMS_TO_TICKS(20));
        
        // 3. 初始化所有端口为输出模式
        for (uint8_t i = 0; i < AW9523_NUM_PORTS; i++) {
            aw9523_set_port_mode(*handle, i, AW9523_GPIO_MODE_OUTPUT);
        }
        

        
        // 5. 先设置所有端口为低电平
        for (uint8_t i = 0; i < AW9523_NUM_PORTS; i++) {
            aw9523_set_port_level(*handle, i, 0);
        }
        
        // 6. 等待更长时间让配置生效
        vTaskDelay(pdMS_TO_TICKS(50));
        
        

    }

    // 设备存在时，初始化完成
    if ((*handle)->dev_handle) {
        // 初始化成功
        AW9523_LOGI("AW9523初始化完成，所有端口设置为纯IO模式，电流源已关闭");
        return ESP_OK;
    } else {
        // 设备未正确连接，返回错误
        AW9523_LOGE("AW9523初始化失败，设备未正确连接");
        free(*handle);
        *handle = NULL;
        return ESP_ERR_NOT_FOUND;
    }
}

// 释放AW9523资源
esp_err_t aw9523_deinit(aw9523_handle_t *handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    // 重置芯片
    aw9523_reset(handle);
    
    // 移除I2C设备（如果存在）
    if (handle->dev_handle) {
        esp_err_t ret = i2c_master_bus_rm_device(handle->dev_handle);
        if (ret != ESP_OK) {
            AW9523_LOGE("Failed to remove I2C device: %s", esp_err_to_name(ret));
            // 继续执行后续清理操作
        }
        handle->dev_handle = NULL;
    }
    
    // 释放句柄内存
    free(handle);
    
    AW9523_LOGI("AW9523 deinitialized");
    
    return ESP_OK;
}

// 设置AW9523指定端口的模式
esp_err_t aw9523_set_port_mode(aw9523_handle_t *handle, uint8_t port_num, uint8_t mode)
{
    if (!handle || port_num >= AW9523_NUM_PORTS) {
        return ESP_ERR_INVALID_ARG;
    }

    // 确定要使用的配置寄存器和位
    uint8_t reg_addr = (port_num < 8) ? AW9523_REG_CONFIG0 : AW9523_REG_CONFIG1;
    uint8_t bit_pos = port_num % 8;
    uint8_t bit_mask = (1 << bit_pos);
    
    // 使用辅助函数来执行读写操作
    uint8_t config_val = 0;
    esp_err_t ret = aw9523_read_register(handle, reg_addr, &config_val);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 更新配置值
    if (mode == AW9523_GPIO_MODE_INPUT) {
        AW9523_LOGI("设置端口%d为输入模式", port_num);
        config_val |= bit_mask;  // 输入模式：设置为1
    } else {
        AW9523_LOGI("设置端口%d为输出模式", port_num);
        config_val &= ~bit_mask; // 输出模式：设置为0
    }
    
    // 写回配置寄存器
    return aw9523_write_register(handle, reg_addr, config_val);
}

// 设置AW9523指定端口的电平
// 注意：使用前必须先设置端口模式为输出
esp_err_t aw9523_set_port_level(aw9523_handle_t *handle, uint8_t port_num, uint8_t level)
{
    if (!handle || port_num >= AW9523_NUM_PORTS || (level != 0 && level != 1)) {
        return ESP_ERR_INVALID_ARG;
    }


    // 确定要使用的输出寄存器和位
    uint8_t reg_addr = (port_num < 8) ? AW9523_REG_OUTPUT0 : AW9523_REG_OUTPUT1;
    uint8_t bit_pos = port_num % 8;
    uint8_t bit_mask = (1 << bit_pos);
    
    // 读取当前输出值
    uint8_t output_val = 0;
    esp_err_t ret = aw9523_read_register(handle, reg_addr, &output_val);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // 更新输出值
    if (level == 1) {
        output_val |= bit_mask;  // 设置为高电平
    } else {
        output_val &= ~bit_mask; // 设置为低电平
    }
    
    // 写回输出寄存器
    return aw9523_write_register(handle, reg_addr, output_val);
}

// 读取AW9523指定端口的输入电平
esp_err_t aw9523_get_port_level(aw9523_handle_t *handle, uint8_t port_num, uint8_t *level)
{
    if (!handle || !level || port_num >= AW9523_NUM_PORTS) {
        AW9523_LOGE("Invalid arguments for aw9523_get_port_level: handle=%p, port=%d, level=%p", 
                 handle, port_num, level);
        return ESP_ERR_INVALID_ARG;
    }

    if (!handle->i2c_bus || !handle->dev_handle) {
        // 设备未连接时，返回错误而不是成功，以保持函数行为的一致性
        AW9523_LOGE("Failed to read port %d level: I2C device not connected", port_num);
        return ESP_ERR_NOT_FOUND;
    }

    // 确定要使用的输入寄存器和位
    uint8_t reg_addr = (port_num < 8) ? AW9523_REG_INPUT0 : AW9523_REG_INPUT1;
    uint8_t bit_pos = port_num % 8;
    uint8_t bit_mask = (1 << bit_pos);
    
    // 读取输入值
    uint8_t input_val = 0;
    esp_err_t ret = aw9523_read_register(handle, reg_addr, &input_val);
    if (ret != ESP_OK) {
        AW9523_LOGE("Failed to read input register 0x%02X: %s", reg_addr, esp_err_to_name(ret));
        return ret;
    }
    
    // 提取指定端口的电平值
    *level = (input_val & bit_mask) ? 1 : 0;
    
    return ESP_OK;
}


// 读取AW9523设备ID
esp_err_t aw9523_get_device_id(aw9523_handle_t *handle, uint8_t *device_id)
{
    if (!handle || !device_id) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!handle->i2c_bus || !handle->dev_handle) {
        // 设备未连接时，返回默认ID 0x23和成功状态
        AW9523_LOGD("I2C device not connected, returning default device ID 0x23");
        *device_id = 0x23;
        return ESP_OK;
    }

    // 读取ID寄存器，添加重试机制
    int max_retries = 2;
    int retry_count = 0;
    esp_err_t ret = ESP_FAIL;
    
    while (retry_count < max_retries && ret != ESP_OK) {
        ret = aw9523_read_register(handle, AW9523_REG_ID, device_id);
        if (ret == ESP_OK) {
            break;
        }
        retry_count++;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    if (ret != ESP_OK) {
        // 如果读取失败，返回默认ID值
        *device_id = 0x23;
        AW9523_LOGW("读取设备ID失败，使用默认值: 0x%02X", *device_id);
    }
    
    return ret;
}

// 重置AW9523芯片
esp_err_t aw9523_reset(aw9523_handle_t *handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!handle->i2c_bus || !handle->dev_handle) {
        // 设备未连接时，返回成功但不执行实际操作
        AW9523_LOGD("I2C device not connected, skipping chip reset");
        return ESP_OK;
    }

    // 正确的软复位：写 RST 寄存器 bit0 = 1
    esp_err_t ret = aw9523_write_register(handle, AW9523_REG_RST, AW9523_RST_RST);
    if (ret != ESP_OK) {
        AW9523_LOGE("Failed to reset via RST register");
        return ret;
    }
    
    // 等待重置完成
    vTaskDelay(pdMS_TO_TICKS(10));
    
    return ESP_OK;
}


// 关闭所有端口的LED模式，设置为GPIO模式
esp_err_t aw9523_disable_all_led_modes(aw9523_handle_t *handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!handle->i2c_bus || !handle->dev_handle) {
        // 设备未连接时，返回成功但不执行实际操作
        AW9523_LOGD("I2C device not connected, skipping all LED modes disabling");
        return ESP_OK;
    }

    AW9523_LOGI("Disable all port LED modes, set to GPIO mode");
    
    // 设置CTL寄存器为GPIO模式（设置第4位为1）
    uint8_t ctl_value = 0;
    esp_err_t ret = aw9523_read_register(handle, AW9523_REG_CTL, &ctl_value);
    if (ret == ESP_OK) {
        ctl_value |= 0x10; // 设置第4位(bit 4, 0-based)为1，设置为GPIO模式
        ret = aw9523_write_register(handle, AW9523_REG_CTL, ctl_value);
        if (ret != ESP_OK) {
            AW9523_LOGE("Failed to write CTL register");
        }
    }
    
    // 清除LED模式寄存器0和1，禁用所有端口的LED模式
    ret = aw9523_write_register(handle, AW9523_REG_LED_MODE0, 0xFF);
    if (ret != ESP_OK) {
        AW9523_LOGE("Failed to clear LED_MODE0 register");
        return ret;
    }
    
    ret = aw9523_write_register(handle, AW9523_REG_LED_MODE1, 0xFF);
    if (ret != ESP_OK) {
        AW9523_LOGE("Failed to clear LED_MODE1 register");
        return ret;
    }
    
    // 等待配置生效
    vTaskDelay(pdMS_TO_TICKS(10));
    
    AW9523_LOGI("All port LED modes disabled successfully");
    return ESP_OK;
}

