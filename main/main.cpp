/* #include <stdio.h>  // 未直接使用 */
/* #include <string.h> // 未直接使用 */
#include <stddef.h>  /* NULL 定义 */
#include "esp_log.h"
/* #include "esp_adc/adc_oneshot.h"  // 未直接使用 */
/* #include "driver/gpio.h"  // 未直接使用 */
/* #include "driver/i2c_master.h"  // 未直接使用 */
/* #include "logic_circuit.h"  // 未直接使用 */

#include "wifi_manager.h"
#include "i2c_manager.h"
#include "gpio_manager.h"
#include "gpio_monitor.h"
#include "sm_version.h"
#include "oled.h"
#include "nvs_manager.h"

static const char *TAG = "main";

// 正常应用入口点（测试模式下会被排除）
#ifndef IDF_BUILD_TESTS
extern "C" void app_main()
{
    ESP_LOGI(TAG, "开始系统初始化...");

    // ===== 第一阶段：基础组件初始化 =====
    // NVS（不阻塞，失败不影响启动，日志在组件内）
    nvs_mgr_init();

    // I2C（阻塞，失败日志在组件内）
    if (i2c_manager_init() != ESP_OK) return;

    // OLED（阻塞，失败日志在组件内）
    if (oled_init() != ESP_OK) return;

    // GPIO管理器（阻塞，失败日志在组件内）
    if (gpio_manager_init() != ESP_OK) return;

    // ===== 第二阶段：启动监控任务 =====
    // 启动GPIO监控（此后才能使用 gpio_monitor_update_status）
    if (gpio_monitor_init() != ESP_OK) return;

    // 更新第一阶段已初始化组件的监控状态
    gpio_monitor_update_component_status("i2c", true);
    gpio_monitor_update_component_status("oled", true);
    gpio_monitor_update_component_status("gpio", true);
    gpio_monitor_update_component_status("monitor", true);
    gpio_monitor_update_step("Basic init done");

    // ========== 应用层初始化 ==========
    if (sm_init() != ESP_OK) {
        gpio_monitor_update_component_status("state_machine", false);
        return;
    }
    gpio_monitor_update_component_status("state_machine", true);
    gpio_monitor_update_step("SM started");

    if (net_init() != ESP_OK) {
        gpio_monitor_update_component_status("net", false);
        return;
    }
    gpio_monitor_update_component_status("net", true);
    gpio_monitor_update_step("Services ready");
}
#endif // IDF_BUILD_TESTS
