/*
 * ESP-IDF Test Application Main Entry Point
 * 
 * 文件: main/test_app_main.cpp
 * 描述: Unity 测试套件专用入口点
 * 
 * 当 IDF_BUILD_TESTS=1 时，此文件会替代正常的 app_main()
 * 作为应用程序入口点，自动执行所有注册的测试用例。
 */

#include "unity.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

static const char* TAG = "TEST_MAIN";

/**
 * @brief 测试应用程序主入口点
 * 
 * 当使用 IDF_BUILD_TESTS=1 构建时，此函数会替代正常的 app_main()
 * 自动执行所有 TEST_CASE 宏定义的测试用例。
 */
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "========== Unity Test Runner Start ==========");
    
    // 禁用看门狗以避免测试期间超时
    ESP_ERROR_CHECK(esp_task_wdt_deinit());
    
    // 运行所有测试用例（自动模式，不等待用户输入）
    unity_run_all_tests();
    
    ESP_LOGI(TAG, "========== Unity Test Runner End ==========");
    
    // 测试完成后，让系统继续运行（防止重启）
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
