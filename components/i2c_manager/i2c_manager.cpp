// 修复后的i2c_manager.cpp文件
#include <stdio.h>
#include <stddef.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "i2c_manager.h"
#include <esp_system.h>
#include <string.h>

static const char *TAG = "i2c_manager";



// 函数前向声明
static void i2c_complete_software_reset(void);

/**
 * @brief 诊断I2C总线状态
 * 
 * @return i2c_bus_status_t 总线状态枚举值
 */
static i2c_bus_status_t i2c_diagnose_bus(void) {
    // 配置引脚为输入模式
    gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(I2C_SCL_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(I2C_SDA_PIN);
    gpio_pullup_en(I2C_SCL_PIN);

    // 读取引脚状态
    int sda_level = gpio_get_level(I2C_SDA_PIN);
    int scl_level = gpio_get_level(I2C_SCL_PIN);

    // 根据引脚状态判断总线状态
    if (sda_level == 0 && scl_level == 0) {
        return I2C_BUS_STATUS_BOTH_LOW;
    } else if (sda_level == 0) {
        return I2C_BUS_STATUS_SDA_LOW;
    } else if (scl_level == 0) {
        return I2C_BUS_STATUS_SCL_LOW;
    }

    return I2C_BUS_STATUS_OK;
}

/**
 * @brief 获取总线状态字符串描述
 * 
 * @param status 总线状态枚举值
 * @return const char* 状态描述字符串
 */
static const char* i2c_get_bus_status_string(i2c_bus_status_t status) {
    switch (status) {
        case I2C_BUS_STATUS_OK: return "正常";
        case I2C_BUS_STATUS_SDA_LOW: return "SDA线路被拉低";
        case I2C_BUS_STATUS_SCL_LOW: return "SCL线路被拉低";
        case I2C_BUS_STATUS_BOTH_LOW: return "SDA和SCL线路都被拉低";
        default: return "未知状态";
    }
}

/**
 * @brief 清理I2C总线（释放SDA和SCL引脚）
 */
static void i2c_cleanup_bus(void) {
    // 重置引脚配置
    gpio_reset_pin(I2C_SDA_PIN);
    gpio_reset_pin(I2C_SCL_PIN);
    
    // 设置为输入模式并启用上拉
    gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(I2C_SCL_PIN, GPIO_MODE_INPUT);
    gpio_pullup_en(I2C_SDA_PIN);
    gpio_pullup_en(I2C_SCL_PIN);
}

static i2c_master_bus_handle_t s_i2c_bus_handle = NULL;
static bool s_initialized = false;
static SemaphoreHandle_t s_i2c_mutex = NULL;
static SemaphoreHandle_t s_reset_mutex = NULL;
static TaskHandle_t s_reset_task_handle = NULL;
static int s_bus_error_count = 0;
// 错误计数阈值 - 增加到10以减少不必要的重置
static const int MAX_ERROR_COUNT = 10;
static bool s_reset_requested = false;
// 设备特定的错误计数
static int s_device_error_count[128] = {0}; // 使用I2C地址作为索引
// 保持向后兼容性的错误计数变量
static int s_aw9523_error_count = 0; // AW9523设备的错误计数
static int s_oled_error_count = 0; // OLED设备的错误计数
// 设备特定的重置阈值
static const int DEVICE_RESET_THRESHOLD = 5;
// 当前正在访问的设备地址
static uint8_t s_current_device_addr = 0;
// 上次访问时间戳
static TickType_t s_last_access_time = 0;
// 最小访问间隔（毫秒）
static const uint32_t MIN_ACCESS_INTERVAL_MS = 5;

// 专用AW9523设备重置函数
esp_err_t i2c_manager_reset_aw9523(void)
{
    ESP_LOGI(TAG, "执行AW9523专用重置流程");
    
    // 第一步：发送多个I2C停止信号序列，确保总线处于空闲状态
    for (int i = 0; i < 3; i++) {
        gpio_reset_pin(I2C_SDA_PIN);
        gpio_reset_pin(I2C_SCL_PIN);
        vTaskDelay(pdMS_TO_TICKS(5));  // 增加延时
    }
    
    // 设置SCL和SDA为输出
    gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(I2C_SCL_PIN, GPIO_MODE_OUTPUT);
    vTaskDelay(pdMS_TO_TICKS(5));  // 增加延时
    
    // 连续发送多个停止信号序列以确保总线清理干净
    for (int seq = 0; seq < 2; seq++) {
        gpio_set_level(I2C_SDA_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SCL_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SDA_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(20));  // 增加延时以确保效果
    }
    
    // 第二步：发送更多的时钟脉冲（10个）来更彻底地清除任何挂起的通信
    for (int i = 0; i < 10; i++) {
        gpio_set_level(I2C_SCL_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(3));  // 增加每个时钟脉冲的延时
        gpio_set_level(I2C_SCL_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(3));
    }
    
    // 第三步：执行额外的GPIO重置序列以确保所有状态都被清除
    for (int i = 0; i < 3; i++) {
        gpio_set_level(I2C_SDA_PIN, 0);
        gpio_set_level(I2C_SCL_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(I2C_SDA_PIN, 1);
        gpio_set_level(I2C_SCL_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // 模拟写AW9523复位寄存器
    gpio_set_level(I2C_SDA_PIN, 0);  // 开始信号
    vTaskDelay(pdMS_TO_TICKS(2));
    
    // 发送地址+写命令 (0x58 << 1) + 0 = 0xB0
    uint8_t address_write = 0xB0;
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(I2C_SCL_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SDA_PIN, (address_write >> i) & 0x01);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SCL_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    
    // 发送CTL寄存器地址 0x7F
    uint8_t reg_addr = 0x7F;
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(I2C_SCL_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SDA_PIN, (reg_addr >> i) & 0x01);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SCL_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    
    // 发送复位值 0x01
    uint8_t reset_value = 0x01;
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(I2C_SCL_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SDA_PIN, (reset_value >> i) & 0x01);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SCL_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    
    // 停止信号
    gpio_set_level(I2C_SDA_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(I2C_SCL_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(I2C_SDA_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));  // 增加延时以确保设备有足够时间完成复位
    
    // 额外的复位序列 - 写入GCR寄存器(0x11)
    // 再次发送开始信号
    gpio_set_level(I2C_SDA_PIN, 0);  // 开始信号
    vTaskDelay(pdMS_TO_TICKS(2));
    
    // 发送地址+写命令
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(I2C_SCL_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SDA_PIN, (address_write >> i) & 0x01);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SCL_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    
    // 发送GCR寄存器地址 0x11
    reg_addr = 0x11;
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(I2C_SCL_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SDA_PIN, (reg_addr >> i) & 0x01);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SCL_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    
    // 发送GCR配置值 0x00
    reset_value = 0x00;
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(I2C_SCL_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SDA_PIN, (reset_value >> i) & 0x01);
        vTaskDelay(pdMS_TO_TICKS(2));
        gpio_set_level(I2C_SCL_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    
    // 停止信号
    gpio_set_level(I2C_SDA_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(I2C_SCL_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(2));
    gpio_set_level(I2C_SDA_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));  // 增加更长的延时以确保设备稳定
    
    ESP_LOGI(TAG, "AW9523专用重置流程完成");
    return ESP_OK;
}

// 低优先级的I2C总线重置任务
void i2c_reset_task(void *pvParameters) {
    while (1) {
        // 等待通知
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        // 获取重置互斥锁
        if (xSemaphoreTake(s_reset_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (s_reset_requested) {
                ESP_LOGW(TAG, "后台重置任务开始执行I2C总线重置");
                
                // 执行完整的软件重置
                i2c_complete_software_reset();
                vTaskDelay(pdMS_TO_TICKS(50)); // 增加延时时间
                
                // 连续执行多次清理操作以确保总线状态稳定
                for (int i = 0; i < 3; i++) {
                    i2c_cleanup_bus();
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                
                // 执行总线诊断
                i2c_bus_status_t status = i2c_diagnose_bus();
                ESP_LOGI(TAG, "后台重置后总线状态: %s", i2c_get_bus_status_string(status));
                
                // 如果总线状态仍异常，执行额外清理，包括AW9523专用重置作为通用清理手段
                if (status != I2C_BUS_STATUS_OK) {
                    ESP_LOGW(TAG, "后台重置后总线状态仍异常，执行额外清理");
                    i2c_manager_reset_aw9523(); // 使用AW9523专用重置作为更彻底的清理方法
                    vTaskDelay(pdMS_TO_TICKS(50));
                    
                    // 再次诊断
                    status = i2c_diagnose_bus();
                    ESP_LOGI(TAG, "后台额外清理后总线状态: %s", i2c_get_bus_status_string(status));
                }
                
                // 重置请求标记
                s_reset_requested = false;
            }
            xSemaphoreGive(s_reset_mutex);
        }
    }
}

esp_err_t i2c_manager_init(void)
{
    if (s_initialized)
    {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "初始化I2C总线");

    // 创建互斥锁
    s_i2c_mutex = xSemaphoreCreateMutex();
    if (s_i2c_mutex == NULL)
    {
        ESP_LOGE(TAG, "创建I2C互斥锁失败");
        return ESP_FAIL;
    }
    
    // 创建重置互斥锁
    s_reset_mutex = xSemaphoreCreateMutex();
    if (s_reset_mutex == NULL)
    {
        ESP_LOGE(TAG, "创建重置互斥锁失败");
        vSemaphoreDelete(s_i2c_mutex);
        s_i2c_mutex = NULL;
        return ESP_FAIL;
    }

    i2c_master_bus_config_t i2c_mst_config = 
    {
        .i2c_port = I2C_PORT,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = 
        {
            .enable_internal_pullup = true,
            .allow_pd = false
        }
    };

    // 使用新的esp-idf v5.x API创建总线，确保不与旧驱动混用
    esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, &s_i2c_bus_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C总线初始化失败: %s", esp_err_to_name(ret));
        if (ret == ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "错误: I2C总线ID(%d)已被占用! 可能存在其他组件在使用同一个总线ID", I2C_PORT);
            ESP_LOGE(TAG, "等待一段时间后再次尝试创建总线...");
            // 等待一段时间让其他组件释放总线
            vTaskDelay(pdMS_TO_TICKS(100));
            // 再次尝试创建总线
            ret = i2c_new_master_bus(&i2c_mst_config, &s_i2c_bus_handle);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "再次尝试I2C总线初始化失败: %s", esp_err_to_name(ret));
            }
        }
        
        if (ret != ESP_OK) {
            vSemaphoreDelete(s_i2c_mutex);
            vSemaphoreDelete(s_reset_mutex);
            s_i2c_mutex = NULL;
            s_reset_mutex = NULL;
            return ret;
        }
    }

    // 创建低优先级的重置任务，增加堆栈大小到4096防止溢出
    if (s_reset_task_handle == NULL) {
        BaseType_t task_ret = xTaskCreatePinnedToCore(
            i2c_reset_task,
            "i2c_reset_task",
            4096,  // 增加堆栈大小到4096，防止复杂的重置操作导致堆栈溢出
            NULL,
            1,
            &s_reset_task_handle,
            1
        );
        if (task_ret != pdPASS) {
            ESP_LOGE(TAG, "创建I2C重置任务失败");
            vSemaphoreDelete(s_i2c_mutex);
            vSemaphoreDelete(s_reset_mutex);
            s_i2c_mutex = NULL;
            s_reset_mutex = NULL;
            return ESP_FAIL;
        }
    }
    
    s_initialized = true;
    s_bus_error_count = 0;
    s_reset_requested = false;
    
    // 增加初始化后的延时，确保所有设备稳定
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "I2C总线初始化成功，频率: %dkHz", I2C_FREQ/1000);
    return ESP_OK;
}

i2c_master_bus_handle_t i2c_manager_get_bus_handle(void)
{
    return s_i2c_bus_handle;
}

esp_err_t i2c_manager_lock(void)
{
    if (!s_initialized || s_i2c_mutex == NULL)
    {
        ESP_LOGE(TAG, "I2C管理器未初始化");
        return ESP_FAIL;
    }

    // 增强的锁获取逻辑
    const int max_retries = 5; // 增加重试次数
    int retry_count = 0;
    
    while (retry_count < max_retries) {
        // 使用适当超时时间尝试获取锁
        if (xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(200)) == pdTRUE) { // 增加超时时间
            // 成功获取锁
            if (s_bus_error_count > 0) {
                i2c_manager_clear_error_count();
            }
            return ESP_OK;
        }
        
        retry_count++;
        ESP_LOGW(TAG, "获取I2C总线锁失败，重试 %d/%d", retry_count, max_retries);
        
        if (retry_count < max_retries) {
            vTaskDelay(pdMS_TO_TICKS(50)); // 增加重试间隔
        }
    }
    
    // 当多次获取锁失败时，记录详细错误
    ESP_LOGW(TAG, "多次获取I2C总线锁失败，尝试重置...");
    i2c_manager_report_error_ext(ESP_FAIL, "获取I2C锁", 0xFF);
    
    // 直接执行一次重置尝试
    if (i2c_manager_reset() == ESP_OK) {
        ESP_LOGI(TAG, "重置后再次尝试获取锁");
        // 重置后再次尝试获取锁，增加超时时间
        if (xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
            i2c_manager_clear_error_count();
            return ESP_OK;
        }
    }
    
    ESP_LOGE(TAG, "所有获取锁的尝试均失败");
    return ESP_FAIL;
}

esp_err_t i2c_manager_unlock(void)
{
    if (!s_initialized || s_i2c_mutex == NULL)
    {
        ESP_LOGE(TAG, "I2C管理器未初始化");
        return ESP_FAIL;
    }

    if (xSemaphoreGive(s_i2c_mutex) != pdTRUE)
    {
        ESP_LOGE(TAG, "释放I2C总线锁失败");
        return ESP_FAIL;
    }

    return ESP_OK;
}

/**
 * @brief 检查当前是否已持有I2C锁
 * 
 * @return true 如果当前任务已持有锁
 * @return false 如果当前任务未持有锁或锁状态不可确定
 */
bool i2c_manager_is_locked(void)
{
    if (!s_initialized || s_i2c_mutex == NULL)
    {
        return false;
    }
    
    // 尝试无等待获取锁，如果成功则表示之前未锁定
    if (xSemaphoreTake(s_i2c_mutex, 0) == pdTRUE)
    {
        // 立即释放锁，因为我们只是检查状态
        xSemaphoreGive(s_i2c_mutex);
        return false;
    }
    
    return true;
}

esp_err_t i2c_manager_reset(void)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "I2C管理器未初始化");
        return ESP_FAIL;
    }

    // 检查是否在中断上下文中，如果是则请求后台重置
    if (xPortInIsrContext()) {
        ESP_LOGW(TAG, "在中断上下文中检测到重置请求，记录请求但不执行重置");
        i2c_manager_request_reset();
        return ESP_OK;
    }

    ESP_LOGI(TAG, "执行I2C总线重置");

    // 清除错误计数
    s_bus_error_count = 0;
    memset(s_device_error_count, 0, sizeof(s_device_error_count));
    s_aw9523_error_count = 0;  // 保持向后兼容性
    s_oled_error_count = 0;    // 保持向后兼容性
    
    // 清除当前设备状态信息
    s_current_device_addr = 0;
    s_last_access_time = 0;
    
    // 执行完整的软件重置以清理总线状态
    i2c_complete_software_reset();
    
    // 增加延时确保总线完全稳定
    vTaskDelay(pdMS_TO_TICKS(30)); // 增加延时时间
    
    // 执行总线诊断
    i2c_bus_status_t status = i2c_diagnose_bus();
    ESP_LOGI(TAG, "重置后总线状态: %s", i2c_get_bus_status_string(status));
    
    // 如果总线状态异常，再执行一次轻量级清理
    if (status != I2C_BUS_STATUS_OK) {
        ESP_LOGW(TAG, "总线状态仍异常，执行额外清理");
        i2c_cleanup_bus();
        vTaskDelay(pdMS_TO_TICKS(20)); // 增加清理后延时
        
        // 再次诊断
        status = i2c_diagnose_bus();
        ESP_LOGI(TAG, "额外清理后总线状态: %s", i2c_get_bus_status_string(status));
    }
    
    return ESP_OK;
}

bool i2c_manager_is_healthy(void)
{
    if (!s_initialized || s_i2c_bus_handle == NULL)
    {
        return false;
    }
    
    // 检查错误计数是否超过阈值
    if (s_bus_error_count >= MAX_ERROR_COUNT)
    {
        ESP_LOGW(TAG, "I2C总线错误计数过高: %d", s_bus_error_count);
        return false;
    }
    
    return true;
}

void i2c_manager_request_reset(void)
{
    // 请求重置I2C总线，但不在高优先级任务中执行
    ESP_LOGW(TAG, "请求I2C总线重置");
    
    if (xSemaphoreTake(s_reset_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        if (!s_reset_requested) {
            s_reset_requested = true;
            ESP_LOGW(TAG, "已请求I2C总线重置，将在后台任务中执行");
            
            // 通知重置任务
            if (s_reset_task_handle != NULL) {
                xTaskNotifyGive(s_reset_task_handle);
            }
        }
        xSemaphoreGive(s_reset_mutex);
    }
}

// 保持向后兼容性的函数
void i2c_manager_report_error(esp_err_t error_code) {
    i2c_manager_report_error_ext(error_code, "未知操作", 0xFF);
}

// 实现带三个参数的函数
void i2c_manager_report_error_ext(esp_err_t error_code, const char* operation_desc, uint8_t dev_addr) {
    // 增加全局错误计数
    s_bus_error_count++;
    
    // 增加特定设备的错误计数
    if (dev_addr < 128) {
        s_device_error_count[dev_addr]++;
    }
    
    // 设备类型识别
    bool is_aw9523 = (dev_addr == 0x58);
    bool is_oled = (dev_addr == 0x3C || dev_addr == 0x3D);
    
    // 设备名称字符串，用于日志
    const char* device_name = "未知设备";
    if (is_aw9523) device_name = "AW9523";
    else if (is_oled) device_name = "OLED";
    
    // 记录详细的错误信息，包括操作和设备地址
    char error_desc[128] = {0};
    snprintf(error_desc, sizeof(error_desc), "%s, 设备地址: 0x%02X, 操作: %s", 
             device_name, dev_addr, operation_desc ? operation_desc : "未知");
    
    ESP_LOGW(TAG, "I2C错误 #%d: %s (错误码: %s)", 
             s_bus_error_count, error_desc, esp_err_to_name(error_code));
    
    // 增加总线状态诊断
    i2c_bus_status_t bus_status = i2c_diagnose_bus();
    ESP_LOGW(TAG, "当前总线状态: %s", i2c_get_bus_status_string(bus_status));
    
    // 检查设备特定错误计数，达到阈值时执行设备特定恢复
    if (dev_addr < 128 && s_device_error_count[dev_addr] >= DEVICE_RESET_THRESHOLD) {
        ESP_LOGW(TAG, "设备错误计数达到阈值 (设备地址: 0x%02X, 计数: %d)", 
                 dev_addr, s_device_error_count[dev_addr]);
        
        // 执行设备特定恢复
        esp_err_t recover_result = i2c_manager_recover_device(dev_addr);
        if (recover_result == ESP_OK) {
            ESP_LOGI(TAG, "设备恢复成功，重置错误计数");
            s_device_error_count[dev_addr] = 0;
        }
    }
    
    // 针对特定的错误类型进行处理
    if (error_code == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "检测到无效的I2C状态，执行清理但不立即重置");
        
        // 对于OLED设备，避免频繁重置总线
        if (is_oled) {
            ESP_LOGW(TAG, "OLED设备无效状态错误，执行轻量级清理而非完整重置");
            i2c_cleanup_bus();
            vTaskDelay(pdMS_TO_TICKS(50)); // 增加等待时间
        } else {
            // 对于其他设备，执行完整重置
            i2c_complete_software_reset();
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    } else if (error_code == ESP_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "I2C通信超时，增加通信参数余量");
    }
    
    // 设备特定的错误阈值处理
    if (is_oled) {
        // OLED设备有更高的错误容忍度，避免频繁重置
        if (s_oled_error_count >= MAX_ERROR_COUNT * 2) {
            ESP_LOGW(TAG, "OLED设备错误计数过高(%d)，执行完整软件重置", s_oled_error_count);
            i2c_complete_software_reset();
            vTaskDelay(pdMS_TO_TICKS(100)); // 为OLED增加更长的等待时间
            
            // 仅重置OLED设备的错误计数
            i2c_manager_clear_device_error_count(dev_addr);
        }
    } else if (s_bus_error_count >= MAX_ERROR_COUNT) {
        // 其他设备保持原有阈值
        ESP_LOGW(TAG, "错误计数超过阈值(%d)，执行完整软件重置", MAX_ERROR_COUNT);
        i2c_complete_software_reset();
        vTaskDelay(pdMS_TO_TICKS(30));
        
        // 重置错误计数
        i2c_manager_clear_error_count();
    }
}

/**
 * @brief 清除特定设备的错误计数
 * 
 * @param dev_addr 设备地址，如果为0则清除所有设备的错误计数
 */
void i2c_manager_clear_device_error_count(uint8_t dev_addr) {
    if (dev_addr == 0) {
        // 清除所有设备的错误计数
        memset(s_device_error_count, 0, sizeof(s_device_error_count));
        s_aw9523_error_count = 0;  // 保持向后兼容性
        s_oled_error_count = 0;    // 保持向后兼容性
        ESP_LOGI(TAG, "已清除所有设备的错误计数");
    } else if (dev_addr < 128) {
        s_device_error_count[dev_addr] = 0;
        // 保持向后兼容性
        if (dev_addr == 0x58) {
            s_aw9523_error_count = 0;
            ESP_LOGI(TAG, "已清除AW9523设备的错误计数");
        } else if (dev_addr == 0x3C || dev_addr == 0x3D) {
            s_oled_error_count = 0;
            ESP_LOGI(TAG, "已清除OLED设备的错误计数");
        } else {
            ESP_LOGI(TAG, "已清除设备的错误计数，地址: 0x%02X", dev_addr);
        }
    }
}

void i2c_manager_clear_error_count(void)
{
    if (s_bus_error_count > 0)
    {
        ESP_LOGI(TAG, "清除I2C总线错误计数 (从%d到0)", s_bus_error_count);
    }
    s_bus_error_count = 0;
}

/**
 * @brief 执行I2C总线轻量级清理
 * 
 * 重置引脚配置，尝试释放总线，不执行完整的硬件重置
 */
void i2c_manager_cleanup_bus(void)
{
    ESP_LOGD(TAG, "执行I2C总线轻量级清理");
    i2c_cleanup_bus();
}

/**
 * @brief 执行I2C设备通信（专注预防错误，优化总线预检）
 */
esp_err_t i2c_manager_transfer(uint8_t dev_addr, const uint8_t *write_data, size_t write_size, uint8_t *read_data, size_t read_size, uint32_t timeout_ms) {
    if (!s_initialized || s_i2c_bus_handle == NULL) {
        ESP_LOGE(TAG, "I2C管理器未初始化");
        return ESP_ERR_INVALID_STATE;
    }

    // 使用默认超时如果未指定
    if (timeout_ms == 0) {
        timeout_ms = I2C_DEFAULT_TIMEOUT_MS;
    }

    // 设备类型识别
    bool is_oled_device = (dev_addr == 0x3C || dev_addr == 0x3D);
    bool is_aw9523_device = (dev_addr == 0x58);

    // 设备名称字符串，用于日志
    const char* device_name = "未知设备";
    if (is_oled_device) device_name = "OLED";
    else if (is_aw9523_device) device_name = "AW9523";

    ESP_LOGD(TAG, "开始I2C传输 - 设备: %s (0x%02X), 写入长度: %d, 读取长度: %d, 超时: %dms", 
             device_name, dev_addr, write_size, read_size, timeout_ms); // 提高日志级别到DEBUG

    // 通信前的总线状态预检 - 仅在错误计数达到阈值的一半时执行
    if (s_bus_error_count > MAX_ERROR_COUNT / 2) {
        i2c_bus_status_t status = i2c_diagnose_bus();
        if (status != I2C_BUS_STATUS_OK) {
            ESP_LOGW(TAG, "通信前总线状态异常: %s，执行清理", 
                    i2c_get_bus_status_string(status));
            // 执行清理操作
            i2c_cleanup_bus();
            vTaskDelay(pdMS_TO_TICKS(20));
            
            // 再次诊断
            status = i2c_diagnose_bus();
            if (status != I2C_BUS_STATUS_OK) {
                ESP_LOGW(TAG, "清理后总线状态仍异常: %s，执行完整软件重置", 
                        i2c_get_bus_status_string(status));
                i2c_complete_software_reset();
                vTaskDelay(pdMS_TO_TICKS(30));
                
                // 再次诊断
                status = i2c_diagnose_bus();
                if (status != I2C_BUS_STATUS_OK) {
                    ESP_LOGE(TAG, "完整软件重置后总线状态仍异常: %s", 
                            i2c_get_bus_status_string(status));
                    return ESP_ERR_INVALID_STATE;
                }
            }
        }
    }

    // 获取I2C总线锁
    esp_err_t lock_result = i2c_manager_lock();
    if (lock_result != ESP_OK) {
        ESP_LOGE(TAG, "获取I2C总线锁失败: %s", esp_err_to_name(lock_result));
        return lock_result;
    }
    
    // 设备切换时增加延迟，避免快速切换导致的通信问题
    if (s_current_device_addr != 0 && s_current_device_addr != dev_addr) {
        TickType_t current_time = xTaskGetTickCount();
        TickType_t elapsed = pdTICKS_TO_MS(current_time - s_last_access_time);
        
        if (elapsed < MIN_ACCESS_INTERVAL_MS) {
            TickType_t delay_needed = MIN_ACCESS_INTERVAL_MS - elapsed;
            ESP_LOGV(TAG, "设备切换延迟: %dms (当前设备: 0x%02X, 目标设备: 0x%02X)", 
                    delay_needed, s_current_device_addr, dev_addr);
            vTaskDelay(pdMS_TO_TICKS(delay_needed));
        }
    }
    
    // 更新当前设备地址和访问时间
    s_current_device_addr = dev_addr;
    s_last_access_time = xTaskGetTickCount();

    // 创建I2C设备配置 - 为OLED和AW9523增加稳定性参数
    // 重要：所有设备必须使用相同的频率以避免共享总线时的通信冲突
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_addr,
        .scl_speed_hz = (uint32_t)I2C_FREQ, // 所有设备使用相同频率
        .scl_wait_us = is_oled_device ? 200U : (is_aw9523_device ? 50U : 10U), // 为不同设备设置不同等待时间
        .flags = {
            .disable_ack_check = 0  // 使用默认标志，启用ACK检查
        }
    };

    // 创建设备句柄
    i2c_master_dev_handle_t dev_handle = NULL;
    esp_err_t ret = i2c_master_bus_add_device(s_i2c_bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "添加I2C设备失败: %s", esp_err_to_name(ret));
        i2c_manager_unlock();
        return ret;
    }

    // 执行传输
    esp_err_t result = ESP_OK;
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    // 优先处理写后读（带RESTART）事务
    if (write_data != NULL && write_size > 0 && read_data != NULL && read_size > 0) {
        result = i2c_master_transmit_receive(dev_handle, write_data, write_size, read_data, read_size, timeout_ticks);
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "I2C写后读(带RESTART)失败 - 设备: %s (0x%02X), 写长度: %d, 读长度: %d, 错误: %s", 
                     device_name, dev_addr, write_size, read_size, esp_err_to_name(result));
        } else {
            ESP_LOGV(TAG, "I2C写后读成功 - 设备: %s (0x%02X), 写长度: %d, 读长度: %d", 
                     device_name, dev_addr, write_size, read_size);
        }
    } else if (read_data != NULL && read_size > 0) {
        // 仅读
        result = i2c_master_receive(dev_handle, read_data, read_size, timeout_ticks);
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "I2C读取失败 - 设备: %s (0x%02X), 长度: %d, 错误: %s", 
                     device_name, dev_addr, read_size, esp_err_to_name(result));
        } else {
            ESP_LOGV(TAG, "I2C读取成功 - 设备: %s (0x%02X), 长度: %d", 
                     device_name, dev_addr, read_size);
        }
    } else if (write_data != NULL && write_size > 0) {
        // 仅写
        result = i2c_master_transmit(dev_handle, write_data, write_size, timeout_ticks);
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "I2C写入失败 - 设备: %s (0x%02X), 长度: %d, 错误: %s", 
                     device_name, dev_addr, write_size, esp_err_to_name(result));
        } else {
            ESP_LOGV(TAG, "I2C写入成功 - 设备: %s (0x%02X), 长度: %d", 
                     device_name, dev_addr, write_size);
        }
    } else {
        // 无数据传输，仅检查设备是否存在
        uint8_t dummy_data = 0;
        result = i2c_master_transmit(dev_handle, &dummy_data, 0, timeout_ticks);
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "I2C设备探测失败 - 设备: %s (0x%02X), 错误: %s", 
                     device_name, dev_addr, esp_err_to_name(result));
        } else {
            ESP_LOGV(TAG, "I2C设备探测成功 - 设备: %s (0x%02X)", 
                     device_name, dev_addr);
        }
    }

    // 如果传输失败，尝试恢复
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "I2C传输失败，尝试恢复 - 设备: %s (0x%02X), 错误码: %s", 
                 device_name, dev_addr, esp_err_to_name(result));
        
        // 对于ESP_ERR_INVALID_STATE错误，需要特殊的处理流程
        if (result == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "检测到无效状态错误，执行特殊恢复流程");
            
            // 清理设备句柄
            if (dev_handle != NULL) {
                i2c_master_bus_rm_device(dev_handle);
                dev_handle = NULL;
            }
            
            // 针对OLED设备的特殊处理
            if (is_oled_device) {
                ESP_LOGW(TAG, "OLED设备无效状态错误，采用特殊恢复策略");
                // 1. 先执行轻量级总线清理，而不是立即请求完整重置
                i2c_cleanup_bus();
                vTaskDelay(pdMS_TO_TICKS(100)); // 为OLED增加更长的等待时间
                
                // 2. 检查当前总线状态
                i2c_bus_status_t status = i2c_diagnose_bus();
                ESP_LOGW(TAG, "OLED错误后总线状态: %s", i2c_get_bus_status_string(status));
                
                // 3. 只有在总线状态真正异常时才请求完整重置
                if (status != I2C_BUS_STATUS_OK) {
                    ESP_LOGW(TAG, "总线状态异常，请求I2C总线重置");
                    i2c_manager_request_reset();
                    vTaskDelay(pdMS_TO_TICKS(200)); // 更长的重置后等待时间
                }
            } else {
                // 对于其他设备，保持原有处理方式
                i2c_manager_request_reset();
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            
            // 增加额外的延时以确保总线完全稳定
            vTaskDelay(pdMS_TO_TICKS(50));
            
            // 重新创建设备句柄
            ret = i2c_master_bus_add_device(s_i2c_bus_handle, &dev_cfg, &dev_handle);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "重新创建I2C设备失败: %s", esp_err_to_name(ret));
                i2c_manager_unlock();
                return ret;
            }
            
            // 增加额外的延时以确保设备句柄完全初始化
            vTaskDelay(pdMS_TO_TICKS(50));
            
            // 再次尝试传输
            if (write_size > 0 && read_size > 0) {
                result = i2c_master_transmit_receive(dev_handle, write_data, write_size, read_data, read_size, pdMS_TO_TICKS(timeout_ms * 2)); // 增加超时时间
            } else if (write_size > 0) {
                result = i2c_master_transmit(dev_handle, write_data, write_size, pdMS_TO_TICKS(timeout_ms * 2)); // 增加超时时间
            } else if (read_size > 0) {
                result = i2c_master_receive(dev_handle, read_data, read_size, pdMS_TO_TICKS(timeout_ms * 2)); // 增加超时时间
            }
            
            if (result != ESP_OK) {
                ESP_LOGE(TAG, "重新传输仍然失败: %s", esp_err_to_name(result));
            }
        }
        
        // 如果仍然失败，尝试设备特定的恢复
        if (result != ESP_OK) {
            // 对于关键设备，如果通信失败，尝试设备特定的恢复
            if (is_aw9523_device) {
                ESP_LOGW(TAG, "AW9523设备通信失败，执行专用恢复");
                i2c_manager_recover_device(0x58);
            } else if (is_oled_device) {
                ESP_LOGW(TAG, "OLED设备通信失败，执行恢复");
                // 对于OLED设备，增加更长的等待时间后再尝试恢复
                vTaskDelay(pdMS_TO_TICKS(100));
                i2c_manager_recover_device(dev_addr);
            }
        }
    }

    // 删除设备句柄
    if (dev_handle != NULL) {
        i2c_master_bus_rm_device(dev_handle);
    }

    // 释放I2C总线锁
    i2c_manager_unlock();

    return result;
}

/**
 * @brief 探测指定地址的I2C设备是否存在
 */
bool i2c_manager_probe_device(uint8_t dev_addr)
{
    if (!s_initialized || s_i2c_bus_handle == NULL) {
        ESP_LOGE(TAG, "I2C管理器未初始化");
        return false;
    }

    // 获取I2C总线锁
    if (i2c_manager_lock() != ESP_OK) {
        ESP_LOGE(TAG, "获取I2C总线锁失败");
        return false;
    }

    // 创建I2C设备配置，增加可靠性参数
    // 重要：必须使用与i2c_manager_transfer相同的频率以避免共享总线时的通信冲突
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = dev_addr,
        .scl_speed_hz = I2C_FREQ, // 与i2c_manager_transfer使用相同频率
        .scl_wait_us = 100, // 增加SCL等待时间以提高探测稳定性
        .flags = {
            .disable_ack_check = 0  // 使用默认标志，启用ACK检查
        }
    };

    // 创建设备句柄
    i2c_master_dev_handle_t dev_handle = NULL;
    esp_err_t ret = i2c_master_bus_add_device(s_i2c_bus_handle, &dev_cfg, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "添加I2C设备失败: %s", esp_err_to_name(ret));
        i2c_manager_unlock();
        return false;
    }

    // 通过尝试读取1个字节来探测设备，避免空写操作导致的问题
    bool device_exists = false;
    const int probe_timeout_ms = 100; // 探测超时时间
    uint8_t read_buffer = 0;
    
    // 尝试读取1个字节，这是一种更可靠的设备探测方法
    ret = i2c_master_receive(dev_handle, &read_buffer, 1, pdMS_TO_TICKS(probe_timeout_ms));
    if (ret == ESP_OK) {
        device_exists = true;
        ESP_LOGD(TAG, "探测到I2C设备地址: 0x%02X", dev_addr);
    } else {
        // 如果只读失败，尝试发送一个包含寄存器地址的写操作
        uint8_t reg_addr = 0; // 使用任意寄存器地址
        ret = i2c_master_transmit(dev_handle, &reg_addr, 1, pdMS_TO_TICKS(probe_timeout_ms));
        if (ret == ESP_OK) {
            device_exists = true;
            ESP_LOGD(TAG, "通过写操作探测到I2C设备地址: 0x%02X", dev_addr);
        } else {
            ESP_LOGD(TAG, "未探测到I2C设备地址: 0x%02X, 错误: %s", 
                    dev_addr, esp_err_to_name(ret));
        }
    }

    // 清理
    i2c_master_bus_rm_device(dev_handle);
    i2c_manager_unlock();

    return device_exists;
}

/**
 * @brief 尝试恢复指定I2C设备的通信
 */
esp_err_t i2c_manager_recover_device(uint8_t dev_addr)
{
    ESP_LOGW(TAG, "尝试恢复I2C设备通信，地址: 0x%02X", dev_addr);
    
    // 对于AW9523设备，使用专用重置函数
    if (dev_addr == 0x58) {
        ESP_LOGW(TAG, "针对AW9523设备使用专用恢复流程");
        
        // 第一步：执行完整的I2C总线重置以确保总线空闲
        ESP_LOGW(TAG, "执行完整I2C总线重置以准备AW9523恢复");
        i2c_complete_software_reset();
        vTaskDelay(pdMS_TO_TICKS(50));
        
        // 第二步：执行多次AW9523专用重置
        int reset_retries = 2;
        esp_err_t ret = ESP_FAIL;
        
        for (int i = 0; i < reset_retries; i++) {
            ESP_LOGI(TAG, "执行AW9523专用重置 (%d/%d)", i + 1, reset_retries);
            ret = i2c_manager_reset_aw9523();
            
            if (ret == ESP_OK) {
                break; // 重置成功，退出循环
            }
            
            ESP_LOGE(TAG, "AW9523专用重置失败，将在100ms后重试");
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        
        // 等待较长时间让设备稳定
        ESP_LOGI(TAG, "等待AW9523设备初始化和稳定");
        vTaskDelay(pdMS_TO_TICKS(150));
        
        // 执行多次轻量级总线清理
        for (int i = 0; i < 3; i++) {
            ESP_LOGD(TAG, "执行总线清理操作 (%d/3)", i + 1);
            i2c_cleanup_bus();
            vTaskDelay(pdMS_TO_TICKS(15));
        }
        
        // 执行总线诊断
        i2c_bus_status_t status = i2c_diagnose_bus();
        ESP_LOGI(TAG, "AW9523重置后总线状态: %s", i2c_get_bus_status_string(status));
        
        // 尝试多次探测设备，增加成功率
        int probe_retries = 5;
        bool device_found = false;
        
        for (int i = 0; i < probe_retries && !device_found; i++) {
            if (i > 0) {
                ESP_LOGW(TAG, "AW9523设备探测重试 (%d/%d)", i + 1, probe_retries);
                vTaskDelay(pdMS_TO_TICKS(20 * (i + 1)));
                
                // 每次重试前执行额外的轻量级清理和总线状态检查
                i2c_cleanup_bus();
                vTaskDelay(pdMS_TO_TICKS(10));
                
                // 额外的时钟脉冲序列，帮助AW9523退出任何错误状态
                ESP_LOGD(TAG, "发送额外的时钟脉冲序列");
                for (int clk_pulse = 0; clk_pulse < 10; clk_pulse++) {
                    gpio_set_level(I2C_SCL_PIN, 0);
                    vTaskDelay(pdMS_TO_TICKS(1));
                    gpio_set_level(I2C_SCL_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(1));
                }
            }
            
            device_found = i2c_manager_probe_device(dev_addr);
        }
        
        if (!device_found) {
            ESP_LOGE(TAG, "设备恢复失败，无法探测到设备地址: 0x%02X", dev_addr);
            return ESP_FAIL;
        }
        
        // 设备恢复成功，清除错误计数
        i2c_manager_clear_error_count();
        i2c_manager_clear_device_error_count(0x58);
        ESP_LOGI(TAG, "AW9523设备通信恢复成功，地址: 0x%02X", dev_addr);
        
        return ESP_OK;
    }
    
    // 非AW9523设备的常规恢复流程
    // 对于OLED设备，增加更长的等待时间
    if (dev_addr == 0x3C || dev_addr == 0x3D) {
        ESP_LOGW(TAG, "OLED设备恢复流程，增加等待时间");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // 执行轻量级总线清理
    i2c_cleanup_bus();
    vTaskDelay(pdMS_TO_TICKS(20));
    
    // 执行总线诊断
    i2c_bus_status_t status = i2c_diagnose_bus();
    ESP_LOGI(TAG, "设备重置前总线状态: %s", i2c_get_bus_status_string(status));
    
    // 如果总线状态异常，执行更彻底的软件重置
    if (status != I2C_BUS_STATUS_OK) {
        ESP_LOGW(TAG, "总线状态异常，执行完整软件重置");
        i2c_complete_software_reset();
        // 对于OLED设备，增加更长的等待时间
        if (dev_addr == 0x3C || dev_addr == 0x3D) {
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            vTaskDelay(pdMS_TO_TICKS(30));
        }
        
        // 再次诊断
        status = i2c_diagnose_bus();
        ESP_LOGI(TAG, "完整软件重置后总线状态: %s", i2c_get_bus_status_string(status));
    }
    
    // 检查设备是否存在
    int probe_retries = 3;
    bool device_found = false;
    
    for (int i = 0; i < probe_retries && !device_found; i++) {
        if (i > 0) {
            ESP_LOGW(TAG, "设备探测重试 (%d/%d)", i + 1, probe_retries);
            vTaskDelay(pdMS_TO_TICKS(10 * i));
        }
        
        device_found = i2c_manager_probe_device(dev_addr);
    }
    
    if (!device_found) {
        ESP_LOGE(TAG, "设备恢复失败，无法探测到设备地址: 0x%02X", dev_addr);
        return ESP_FAIL;
    }
    
    // 设备恢复成功，清除错误计数
    i2c_manager_clear_error_count();
    if (dev_addr == 0x3C || dev_addr == 0x3D) {
        i2c_manager_clear_device_error_count(dev_addr);
    }
    ESP_LOGI(TAG, "I2C设备通信恢复成功，地址: 0x%02X", dev_addr);
    
    return ESP_OK;
}

/**
 * @brief 安全执行I2C操作（带自动恢复）
 */
esp_err_t i2c_manager_safe_execute(esp_err_t (*operation)(void *), void *user_data)
{
    if (operation == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // 执行操作
    esp_err_t result = operation(user_data);

    // 如果操作失败，报告错误
    if (result != ESP_OK) {
        i2c_manager_report_error(result);
    }

    return result;
}

/**
 * @brief 执行完整的软件重置
 */
void i2c_complete_software_reset(void) {
    ESP_LOGW(TAG, "执行完整的I2C总线软件重置");
    
    // 保存当前引脚配置
    gpio_reset_pin(I2C_SDA_PIN);
    gpio_reset_pin(I2C_SCL_PIN);
    vTaskDelay(pdMS_TO_TICKS(5));
    
    // 设置SCL为输出，SDA为输入
    gpio_set_direction(I2C_SCL_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_INPUT);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    // 读取SDA线状态
    uint8_t sda_level = gpio_get_level(I2C_SDA_PIN);
    ESP_LOGD(TAG, "重置前SDA状态: %d", sda_level);
    
    // 如果SDA被拉低，强制将其拉高
    if (sda_level == 0) {
        ESP_LOGW(TAG, "SDA被拉低，强制释放SDA线路");
        gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(I2C_SDA_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // 发送128个时钟脉冲，比之前更多，确保任何设备都能完成操作
    ESP_LOGD(TAG, "发送128个时钟脉冲");
    for (int i = 0; i < 128; i++) {
        gpio_set_level(I2C_SCL_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(1));
        gpio_set_level(I2C_SCL_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    // 检查SDA是否已经释放
    sda_level = gpio_get_level(I2C_SDA_PIN);
    ESP_LOGD(TAG, "时钟脉冲后SDA状态: %d", sda_level);
    
    if (sda_level == 0) {
        ESP_LOGE(TAG, "警告：时钟脉冲后SDA仍被拉低，可能存在硬件问题");
    }
    
    // 发送停止条件
    gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(I2C_SDA_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(I2C_SCL_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(I2C_SDA_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    
    // 恢复引脚为开漏输出模式
    gpio_set_direction(I2C_SDA_PIN, GPIO_MODE_OUTPUT_OD);
    gpio_set_direction(I2C_SCL_PIN, GPIO_MODE_OUTPUT_OD);
    vTaskDelay(pdMS_TO_TICKS(5));
    
    ESP_LOGI(TAG, "完整的I2C总线软件重置完成");
}
