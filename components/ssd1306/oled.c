// oled.c - 轻量 SSD1306 驱动 for ESP-IDF
// #define SKIP_OLED_INIT_COMMANDS  // 取消注释此行可跳过发送初始化命令

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "i2c_manager.h"
#include "oled.h"
#include <esp_system.h>  // 替换stdlib.h
#include <string.h>  // 用于memcpy和memset

static const char *TAG = "oled";

// I2C设备句柄
static i2c_master_dev_handle_t i2c_dev_handle;
static uint8_t actual_oled_addr = 0;

// 帧缓冲（128x64 = 1024 字节）
static uint8_t oled_buffer[OLED_WIDTH * OLED_PAGES];

// 5x8 ASCII字体数据
static const uint8_t font5x8[95][5] = {
    // Character: ' ' (32)
    {0x00, 0x00, 0x00, 0x00, 0x00},
    // Character: '!' (33)
    {0x00, 0x00, 0x5F, 0x00, 0x00},
    // Character: '"' (34)
    {0x00, 0x07, 0x00, 0x07, 0x00},
    // Character: '#' (35)
    {0x14, 0x7F, 0x14, 0x7F, 0x14},
    // Character: '$' (36)
    {0x24, 0x2A, 0x7F, 0x2A, 0x12},
    // Character: '%' (37)
    {0x23, 0x13, 0x08, 0x64, 0x62},
    // Character: '&' (38)
    {0x36, 0x49, 0x55, 0x22, 0x50},
    // Character: ''' (39)
    {0x00, 0x05, 0x03, 0x00, 0x00},
    // Character: '(' (40)
    {0x00, 0x1C, 0x22, 0x41, 0x00},
    // Character: ')' (41)
    {0x00, 0x41, 0x22, 0x1C, 0x00},
    // Character: '*' (42)
    {0x08, 0x2A, 0x1C, 0x2A, 0x08},
    // Character: '+' (43)
    {0x08, 0x08, 0x3E, 0x08, 0x08},
    // Character: ',' (44)
    {0x00, 0x50, 0x30, 0x00, 0x00},
    // Character: '-' (45)
    {0x08, 0x08, 0x08, 0x08, 0x08},
    // Character: '.' (46)
    {0x00, 0x60, 0x60, 0x00, 0x00},
    // Character: '/' (47)
    {0x20, 0x10, 0x08, 0x04, 0x02},
    // Character: '0' (48)
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    // Character: '1' (49)
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    // Character: '2' (50)
    {0x42, 0x61, 0x51, 0x49, 0x46},
    // Character: '3' (51)
    {0x21, 0x41, 0x45, 0x4B, 0x31},
    // Character: '4' (52)
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    // Character: '5' (53)
    {0x27, 0x45, 0x45, 0x45, 0x39},
    // Character: '6' (54)
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    // Character: '7' (55)
    {0x01, 0x71, 0x09, 0x05, 0x03},
    // Character: '8' (56)
    {0x36, 0x49, 0x49, 0x49, 0x36},
    // Character: '9' (57)
    {0x06, 0x49, 0x49, 0x29, 0x1E},
    // Character: ':' (58)
    {0x00, 0x36, 0x36, 0x00, 0x00},
    // Character: ';' (59)
    {0x00, 0x56, 0x36, 0x00, 0x00},
    // Character: '<' (60)
    {0x08, 0x14, 0x22, 0x41, 0x00},
    // Character: '=' (61)
    {0x14, 0x14, 0x14, 0x14, 0x14},
    // Character: '>' (62)
    {0x00, 0x41, 0x22, 0x14, 0x08},
    // Character: '?' (63)
    {0x02, 0x01, 0x51, 0x09, 0x06},
    // Character: '@' (64)
    {0x3E, 0x51, 0x49, 0x41, 0x3E},
    // Character: 'A' (65)
    {0x7E, 0x09, 0x09, 0x09, 0x7E},
    // Character: 'B' (66)
    {0x7F, 0x49, 0x49, 0x49, 0x36},
    // Character: 'C' (67)
    {0x3E, 0x41, 0x41, 0x41, 0x22},
    // Character: 'D' (68)
    {0x7F, 0x41, 0x41, 0x22, 0x1C},
    // Character: 'E' (69)
    {0x7F, 0x49, 0x49, 0x49, 0x41},
    // Character: 'F' (70)
    {0x7F, 0x09, 0x09, 0x09, 0x01},
    // Character: 'G' (71)
    {0x3E, 0x41, 0x49, 0x49, 0x7A},
    // Character: 'H' (72)
    {0x7F, 0x08, 0x08, 0x08, 0x7F},
    // Character: 'I' (73)
    {0x00, 0x41, 0x7F, 0x41, 0x00},
    // Character: 'J' (74)
    {0x20, 0x40, 0x41, 0x3F, 0x01},
    // Character: 'K' (75)
    {0x7F, 0x08, 0x14, 0x22, 0x41},
    // Character: 'L' (76)
    {0x7F, 0x40, 0x40, 0x40, 0x40},
    // Character: 'M' (77)
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    // Character: 'N' (78)
    {0x7F, 0x04, 0x08, 0x10, 0x7F},
    // Character: 'O' (79)
    {0x3E, 0x41, 0x41, 0x41, 0x3E},
    // Character: 'P' (80)
    {0x7F, 0x09, 0x09, 0x09, 0x06},
    // Character: 'Q' (81)
    {0x3E, 0x41, 0x41, 0x41, 0x3E},
    // Character: 'R' (82)
    {0x7F, 0x09, 0x09, 0x09, 0x76},
    // Character: 'S' (83)
    {0x26, 0x49, 0x49, 0x49, 0x32},
    // Character: 'T' (84)
    {0x01, 0x01, 0x7F, 0x01, 0x01},
    // Character: 'U' (85)
    {0x3F, 0x40, 0x40, 0x40, 0x3F},
    // Character: 'V' (86)
    {0x1F, 0x20, 0x40, 0x20, 0x1F},
    // Character: 'W' (87)
    {0x3F, 0x40, 0x38, 0x40, 0x3F},
    // Character: 'X' (88)
    {0x63, 0x14, 0x08, 0x14, 0x63},
    // Character: 'Y' (89)
    {0x07, 0x08, 0x70, 0x08, 0x07},
    // Character: 'Z' (90)
    {0x61, 0x51, 0x49, 0x45, 0x43},
    // Character: '[' (91)
    {0x00, 0x7F, 0x41, 0x41, 0x00},
    // Character: '\' (92)
    {0x02, 0x04, 0x08, 0x10, 0x20},
    // Character: ']' (93)
    {0x00, 0x41, 0x41, 0x7F, 0x00},
    // Character: '^' (94)
    {0x04, 0x02, 0x01, 0x02, 0x04},
    // Character: '_' (95)
    {0x40, 0x40, 0x40, 0x40, 0x40},
    // Character: '`' (96)
    {0x00, 0x01, 0x02, 0x04, 0x00},
    // Character: 'a' (97)
    {0x20, 0x54, 0x54, 0x54, 0x78},
    // Character: 'b' (98)
    {0x7F, 0x48, 0x44, 0x44, 0x38},
    // Character: 'c' (99)
    {0x38, 0x44, 0x44, 0x44, 0x20},
    // Character: 'd' (100)
    {0x38, 0x44, 0x44, 0x48, 0x7F},
    // Character: 'e' (101)
    {0x38, 0x54, 0x54, 0x54, 0x18},
    // Character: 'f' (102)
    {0x08, 0x7E, 0x09, 0x01, 0x02},
    // Character: 'g' (103)
    {0x08, 0x44, 0x44, 0x44, 0x38},
    // Character: 'h' (104)
    {0x7F, 0x08, 0x04, 0x04, 0x78},
    // Character: 'i' (105)
    {0x00, 0x44, 0x7D, 0x40, 0x00},
    // Character: 'j' (106)
    {0x20, 0x40, 0x44, 0x3D, 0x00},
    // Character: 'k' (107)
    {0x7F, 0x10, 0x28, 0x44, 0x00},
    // Character: 'l' (108)
    {0x00, 0x41, 0x7F, 0x40, 0x00},
    // Character: 'm' (109)
    {0x7C, 0x04, 0x18, 0x04, 0x78},
    // Character: 'n' (110)
    {0x7C, 0x08, 0x04, 0x04, 0x78},
    // Character: 'o' (111)
    {0x38, 0x44, 0x44, 0x44, 0x38},
    // Character: 'p' (112)
    {0x7C, 0x14, 0x14, 0x14, 0x08},
    // Character: 'q' (113)
    {0x08, 0x14, 0x14, 0x18, 0x7C},
    // Character: 'r' (114)
    {0x7C, 0x08, 0x04, 0x04, 0x00},
    // Character: 's' (115)
    {0x48, 0x54, 0x54, 0x54, 0x20},
    // Character: 't' (116)
    {0x04, 0x3E, 0x44, 0x40, 0x20},
    // Character: 'u' (117)
    {0x3C, 0x40, 0x40, 0x20, 0x7C},
    // Character: 'v' (118)
    {0x1C, 0x20, 0x40, 0x20, 0x1C},
    // Character: 'w' (119)
    {0x3C, 0x40, 0x30, 0x40, 0x3C},
    // Character: 'x' (120)
    {0x44, 0x28, 0x10, 0x28, 0x44},
    // Character: 'y' (121)
    {0x0C, 0x50, 0x50, 0x50, 0x3C},
    // Character: 'z' (122)
    {0x64, 0x54, 0x4C, 0x44, 0x44},
    // Character: '{' (123)
    {0x08, 0x3E, 0x41, 0x41, 0x22},
    // Character: '|' (124)
    {0x00, 0x7F, 0x41, 0x41, 0x00},
    // Character: '}' (125)
    {0x22, 0x41, 0x41, 0x3E, 0x08},
    // Character: '~' (126)
    {0x02, 0x01, 0x02, 0x04, 0x02}
};

// 基础发送命令函数
static esp_err_t oled_write_cmd(uint8_t cmd)
{
    esp_err_t ret = ESP_OK;
    int retry_count = 0;
    const int max_retries = 3;
    
    // SSD1306命令需要控制字节0x00
    uint8_t cmd_with_control[2] = {0x00, cmd};
    
    do {
        // 使用更长的超时时间提高稳定性
        ret = i2c_manager_transfer(actual_oled_addr, cmd_with_control, 2, NULL, 0, 500);
        if (ret == ESP_OK) {
            break;
        }
        
        retry_count++;
        ESP_LOGD(TAG, "OLED写命令失败 (0x%02X)，重试 %d/%d: %s", 
                 cmd, retry_count, max_retries, esp_err_to_name(ret));
        
        // 增加延时以确保设备稳定
        vTaskDelay(pdMS_TO_TICKS(10 * retry_count)); // 递增延时
        
        // 如果是ESP_ERR_INVALID_STATE错误，执行轻量级清理
        if (ret == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "检测到无效状态错误，执行轻量级总线清理");
            i2c_manager_cleanup_bus(); // 执行轻量级总线清理而非完整重置
            vTaskDelay(pdMS_TO_TICKS(50));
        } else if (ret == ESP_ERR_TIMEOUT) {
            // 超时错误，增加额外延时
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    } while (retry_count < max_retries);
    
    if (ret != ESP_OK) {
        // 仅在DEBUG级别记录详细错误，减少日志噪音
        ESP_LOGD(TAG, "OLED写命令最终失败 (0x%02X): %s", cmd, esp_err_to_name(ret));
        // 报告错误但不中断流程
        i2c_manager_report_error_ext(ret, "OLED写命令", 0);
    }
    
    return ret;
}

// 基础发送数据函数 - 增强版
esp_err_t oled_write_data(const uint8_t *data, size_t len) {
    // 分配缓冲区，第一个字节是控制字节0x40（数据）
    uint8_t *write_buf = (uint8_t*)malloc(len + 1);
    if (write_buf == NULL) {
        ESP_LOGE(TAG, "内存分配失败，长度: %d", len + 1);
        return ESP_ERR_NO_MEM;
    }
    write_buf[0] = 0x40; // 控制字节：0x40 = 数据
    memcpy(write_buf + 1, data, len);
    
    // 添加重试机制
    int retry_count = 0;
    const int max_retries = 4;  // 增加重试次数
    esp_err_t ret = ESP_FAIL;
    
    do {
        // 对于大数据传输，使用更长的超时时间
        int timeout = (len > 64) ? 3000 : 2000;
        ret = i2c_manager_transfer(actual_oled_addr, write_buf, len + 1, NULL, 0, timeout);
        if (ret == ESP_OK) {
            break;
        }
        
        retry_count++;
        ESP_LOGD(TAG, "发送数据失败, 长度: %d, 错误码: %s, 重试 %d/%d", 
                 len, esp_err_to_name(ret), retry_count, max_retries);
        
        // 处理特定错误
        if (ret == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "检测到无效状态错误，执行轻量级总线清理");
            i2c_manager_cleanup_bus(); // 执行轻量级总线清理而非完整重置
            i2c_manager_report_error_ext(ret, "OLED写入数据", 0);
            vTaskDelay(pdMS_TO_TICKS(100)); // 增加恢复时间
        } else if (ret == ESP_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "I2C总线超时，增加等待时间并重试");
            i2c_manager_report_error_ext(ret, "OLED写入数据", 0);
            vTaskDelay(pdMS_TO_TICKS(200)); // 超时错误使用更长延时
        } else {
            // 其他错误，使用递增延时
            vTaskDelay(pdMS_TO_TICKS(50 * retry_count));
        }
    } while (retry_count < max_retries);
    
    free(write_buf);
    
    // 即使失败也继续执行，不要中断整个流程
    if (ret != ESP_OK) {
        ESP_LOGD(TAG, "OLED数据写入最终失败: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

// 初始化 - 增强版，处理I2C冲突
esp_err_t oled_init() {
    ESP_LOGI(TAG, "开始OLED初始化");
    
    // 确保I2C管理器已初始化
    esp_err_t ret = i2c_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2C管理器初始化失败: %s", esp_err_to_name(ret));
        // 如果初始化失败，可能是因为总线已被初始化，继续尝试
    }
    
    // 增加延时确保总线稳定
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 检查总线健康状态
    if (!i2c_manager_is_healthy()) {
        ESP_LOGW(TAG, "I2C总线状态异常，记录状态但不执行重置");
        // 总线状态异常，请求重置但不中断流程
        // i2c_manager_request_reset();
        // vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // 直接使用已知的OLED设备地址0x3D，跳过探测过程
    ESP_LOGI(TAG, "使用已知OLED设备地址: 0x3D");
    actual_oled_addr = 0x3D;
    
    // 移除总线重置，仅记录日志
    ESP_LOGI(TAG, "总线重置已禁用，仅记录日志");
    // i2c_manager_request_reset();
    // vTaskDelay(pdMS_TO_TICKS(100));  // 等待重置完成
    
    // 获取I2C总线句柄
    i2c_master_bus_handle_t i2c_bus_handle = i2c_manager_get_bus_handle();
    if (i2c_bus_handle == NULL) {
        // 尝试重新初始化总线
        ret = i2c_manager_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "重新初始化I2C总线失败: %s", esp_err_to_name(ret));
            return ret;
        }
        i2c_bus_handle = i2c_manager_get_bus_handle();
        if (i2c_bus_handle == NULL) {
            ESP_LOGE(TAG, "获取I2C总线句柄失败");
            return ESP_FAIL;
        }
    }

    // 初始化设备配置，使用与AW9523相同的I2C频率以避免总线混乱
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = actual_oled_addr,
        .scl_speed_hz = I2C_FREQ,      // 与AW9523使用相同的50kHz频率
        .scl_wait_us = 200U,           // 保持较长的等待时间以提高稳定性
        .flags = {
            .disable_ack_check = 0     // 启用ACK检查
        }
    };
    
    ESP_LOGI(TAG, "为OLED设置I2C频率: %dkHz (与AW9523相同以避免总线混乱)", I2C_FREQ/1000);
    
    // 添加设备到总线，最多尝试3次
    int add_device_retries = 0;
    const int max_add_device_retries = 3;
    while (add_device_retries < max_add_device_retries) {
        ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &i2c_dev_handle);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "成功添加OLED设备到总线");
            break;
        }
        
        add_device_retries++;
        ESP_LOGW(TAG, "添加OLED设备失败 (尝试 %d/%d): %s", 
                 add_device_retries, max_add_device_retries, esp_err_to_name(ret));
        
        if (ret == ESP_ERR_INVALID_STATE) {
            // 记录错误但不执行重置
            ESP_LOGW(TAG, "检测到无效状态，记录错误但不执行总线重置");
            // i2c_manager_request_reset();
            // vTaskDelay(pdMS_TO_TICKS(50));
        } else {
            // 其他错误，短暂延迟后重试
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "添加OLED设备失败，已达到最大重试次数");
        return ret;
    }
    
    // 在文件开头添加条件编译选项
    // #define SKIP_OLED_INIT_COMMANDS  // 取消注释此行可跳过发送初始化命令
    
    // 短暂延迟，确保设备稳定
    vTaskDelay(pdMS_TO_TICKS(20));

#ifndef SKIP_OLED_INIT_COMMANDS
    // 标准初始化命令序列
    const uint8_t init_cmds[] = {
        0xAE, // Display OFF
        0xD5, 0x80, // Set Clock Divide Ratio
        0xA8, 0x3F, // Set Multiplex Ratio (64)
        0xD3, 0x00, // Set Display Offset
        0x40,       // Set Display Start Line
        0x8D, 0x14, // Enable Charge Pump
        0x20, 0x00, // Set Memory Addressing Mode to horizontal
        0xA1,       // Segment Re-map (left-right)
        0xC8,       // COM Output Scan Direction (bottom-top)
        0xDA, 0x12, // Set COM Pins
        0x81, 0xCF, // Set Contrast (0xCF default)
        0xD9, 0xF1, // Set Pre-charge Period
        0xDB, 0x40, // Set VCOMH
        0xA4,       // Disable Entire Display On
        0xA6,       // Set Display Mode (Normal)
        0xAF        // Display ON
    };
    
    // 发送初始化命令，最多尝试5次
    int cmd_retries = 0;
    const int max_cmd_retries = 5;
    bool init_success = false;

    // 在开始发送命令前，先执行一次总线健康检查
    if (!i2c_manager_is_healthy()) {
        ESP_LOGW(TAG, "I2C总线状态异常，记录状态但不执行重置");
        // i2c_manager_request_reset();
        // vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    while (!init_success && cmd_retries < max_cmd_retries) {
        init_success = true;
        ESP_LOGI(TAG, "开始发送初始化命令序列 (尝试 %d/%d)", cmd_retries + 1, max_cmd_retries);
        
        for (int i = 0; i < sizeof(init_cmds); i++) {
            ret = oled_write_cmd(init_cmds[i]);
            if (ret != ESP_OK) {
                init_success = false;
                ESP_LOGW(TAG, "发送初始化命令失败 (命令: 0x%02X): %s", 
                         init_cmds[i], esp_err_to_name(ret));
                
                // 特殊处理ESP_ERR_INVALID_STATE错误
                if (ret == ESP_ERR_INVALID_STATE) {
                    ESP_LOGW(TAG, "检测到无效状态错误，记录错误但不执行特殊恢复流程");
                    // 请求总线重置并等待
                    // i2c_manager_request_reset();
                    // vTaskDelay(pdMS_TO_TICKS(100));
                    
                    // 重新初始化I2C管理器
                    // 注意：这里我们不调用deinit函数，因为I2C管理器没有提供deinit接口
                    // 直接重新初始化
                    if (i2c_manager_init() != ESP_OK) {
                        ESP_LOGE(TAG, "重新初始化I2C管理器失败");
                        break;
                    }
                    
                    // 增加额外延时确保I2C管理器完全初始化
                    vTaskDelay(pdMS_TO_TICKS(100));
                    
                    // 重新探测OLED设备
                    if (!i2c_manager_probe_device(actual_oled_addr)) {
                        ESP_LOGE(TAG, "重新探测OLED设备失败");
                        // 尝试多次探测
                        bool device_found = false;
                        for (int probe_retry = 0; probe_retry < 3 && !device_found; probe_retry++) {
                            ESP_LOGW(TAG, "重新探测OLED设备 (尝试 %d/3)", probe_retry + 1);
                            vTaskDelay(pdMS_TO_TICKS(50));
                            device_found = i2c_manager_probe_device(actual_oled_addr);
                        }
                        
                        if (!device_found) {
                            ESP_LOGE(TAG, "多次尝试后仍无法探测到OLED设备");
                            break;
                        }
                    }
                    
                    ESP_LOGI(TAG, "完成特殊恢复流程，重新尝试发送命令");
                    // 重新开始发送命令序列
                    init_success = true;  // 重置状态
                    i = -1;  // 重新从第一个命令开始
                    continue;
                }
                
                break;
            }
            
            // 必要的最小延时
            if (init_cmds[i] == 0x8D || init_cmds[i] == 0xAE || init_cmds[i] == 0xAF) {
                vTaskDelay(pdMS_TO_TICKS(15));  // 增加延时到15ms
            } else {
                vTaskDelay(pdMS_TO_TICKS(2));   // 其他命令之间增加2ms延时
            }
        }
        
        if (!init_success) {
            cmd_retries++;
            if (cmd_retries < max_cmd_retries) {
                ESP_LOGW(TAG, "OLED初始化失败 (尝试 %d/%d)，重试中...", 
                         cmd_retries, max_cmd_retries);
                // 执行设备恢复
                // i2c_manager_recover_device(actual_oled_addr);
                // vTaskDelay(pdMS_TO_TICKS(100));  // 增加恢复后的延时
                
                // 在重试前再次检查总线健康状态
                if (!i2c_manager_is_healthy()) {
                    ESP_LOGW(TAG, "I2C总线状态异常，记录状态但不执行重置");
                    // i2c_manager_request_reset();
                    // vTaskDelay(pdMS_TO_TICKS(100));
                }
            }
        }
    }
    
    if (!init_success) {
        ESP_LOGE(TAG, "OLED初始化失败，已达到最大重试次数");
        return ESP_FAIL;
    }
#else
    ESP_LOGW(TAG, "跳过发送OLED初始化命令序列");
#endif

    // 清空并刷新显示
    ESP_LOGI(TAG, "初始化命令发送成功，开始清空显示缓冲区");
    oled_clear();
    ESP_LOGI(TAG, "开始刷新显示");
    oled_refresh();
    
    ESP_LOGI(TAG, "OLED初始化成功");
    return ESP_OK;
}

// 清空缓冲区
esp_err_t oled_clear() {
    memset(oled_buffer, 0, sizeof(oled_buffer));
    return ESP_OK;
}

// 刷新：将缓冲区发送到 OLED - 增强版
esp_err_t oled_refresh() {
    ESP_LOGD(TAG, "开始刷新OLED显示"); // 提高日志级别到DEBUG
    
    int total_error_count = 0;
    const int max_total_errors = 10;
    
    // 检查I2C总线健康状态
    if (!i2c_manager_is_healthy()) {
        ESP_LOGW(TAG, "刷新前检测到I2C总线状态异常，执行轻量级清理");
        i2c_manager_clear_error_count();
        i2c_manager_recover_device(actual_oled_addr); // 使用设备特定恢复
        vTaskDelay(pdMS_TO_TICKS(50)); // 增加延时确保稳定
    }
    
    for (int page = 0; page < OLED_PAGES; page++) {
        int page_retries = 0;
        const int max_page_retries = 2;
        bool page_success = false;
        
        while (!page_success && page_retries < max_page_retries && total_error_count < max_total_errors) {
            // 设置页地址
            esp_err_t ret = oled_write_cmd(0xB0 + page);
            if (ret != ESP_OK) {
                total_error_count++;
                if (ret == ESP_ERR_INVALID_STATE) {
                    ESP_LOGW(TAG, "设置页地址时检测到无效状态错误 (页: %d): %s", page, esp_err_to_name(ret));
                    i2c_manager_report_error_ext(ret, "OLED刷新-设置页地址", 0);
                    i2c_manager_cleanup_bus(); // 执行轻量级总线清理而非完整重置
                    vTaskDelay(pdMS_TO_TICKS(100)); // 增加恢复时间
                } else {
                    ESP_LOGW(TAG, "设置页地址失败 (页: %d): %s", page, esp_err_to_name(ret));
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
                page_retries++;
                continue;
            }
            
            // 设置列低位
            ret = oled_write_cmd(0x00);
            if (ret != ESP_OK) {
                total_error_count++;
                ESP_LOGW(TAG, "设置列低位失败: %s", esp_err_to_name(ret));
                vTaskDelay(pdMS_TO_TICKS(20));
                page_retries++;
                continue;
            }
            
            // 设置列高位
            ret = oled_write_cmd(0x10);
            if (ret != ESP_OK) {
                total_error_count++;
                ESP_LOGW(TAG, "设置列高位失败: %s", esp_err_to_name(ret));
                vTaskDelay(pdMS_TO_TICKS(20));
                page_retries++;
                continue;
            }
            
            // 写入页数据
            ret = oled_write_data(&oled_buffer[page * OLED_WIDTH], OLED_WIDTH);
            if (ret != ESP_OK) {
                total_error_count++;
                if (ret == ESP_ERR_INVALID_STATE) {
                    ESP_LOGW(TAG, "写入页数据时检测到无效状态错误 (页: %d): %s", page, esp_err_to_name(ret));
                    i2c_manager_report_error_ext(ret, "OLED刷新-写入页数据", 0);
                    i2c_manager_cleanup_bus(); // 执行轻量级总线清理
                    vTaskDelay(pdMS_TO_TICKS(100));
                } else {
                    ESP_LOGW(TAG, "写入页数据失败 (页: %d): %s", page, esp_err_to_name(ret));
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
                page_retries++;
                continue;
            }
            
            page_success = true;
        }
        
        if (!page_success) {
            ESP_LOGE(TAG, "OLED刷新页 %d 失败，已达到最大重试次数或总错误数");
            // 即使部分页失败，也继续尝试刷新其他页
        }
        
        // 在页之间增加短暂延时
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    if (total_error_count > 0) {
        ESP_LOGW(TAG, "OLED刷新完成，但有 %d 个错误", total_error_count);
    }
    
    return ESP_OK;
}

// 显示字符 - 基础版
esp_err_t oled_display_char(int x, int y, uint8_t c) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_PAGES) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // 只显示可打印字符
    if (c < 32 || c > 126) {
        c = ' '; // 替换为空格
    }
    
    // 将字符数据复制到帧缓冲
    for (int i = 0; i < 5; i++) {
        if (x + i < OLED_WIDTH) {
            oled_buffer[y * OLED_WIDTH + x + i] = font5x8[c - 32][i];
        }
    }
    
    return ESP_OK;
}

// 显示字符串 - 增强版
esp_err_t oled_display_string(int line, const char* str) {
    if (line < 0 || line >= OLED_PAGES) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (str == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int x = 0;
    int y = line;
    
    // 清空该行
    for (x = 0; x < OLED_WIDTH; x++) {
        oled_buffer[y * OLED_WIDTH + x] = 0;
    }
    
    x = 0;
    while (*str && x < OLED_WIDTH) {
        // 显示当前字符
        esp_err_t ret = oled_display_char(x, y, *str);
        if (ret != ESP_OK) {
            return ret;
        }
        
        // 移动到下一个字符位置，不留额外间隔（5x8字体更紧凑）
        x += 5;
        str++;
    }
    
    return ESP_OK;
}

