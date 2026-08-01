#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "string.h"
#include "esp_chip_info.h"
#include "esp_system.h"
#include "spi_flash_mmap.h"
#include "esp_timer.h"
#include "system_status.h"
#include "gpio_manager.h"

static const char *TAG = "system_status";

// 系统状态处理函数
esp_err_t system_status_handler(httpd_req_t *req)
{
    // 设置CORS头部
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    
    // 处理OPTIONS请求
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_set_status(req, "204 No Content");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    // 只处理GET请求
    if (req->method != HTTP_GET) {
        httpd_resp_set_status(req, "405 Method Not Allowed");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "接收到系统状态请求");
    
    // 创建JSON响应
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "无法创建JSON对象");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    // 添加芯片信息
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON *chip_info_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(chip_info_obj, "cores", chip_info.cores);
    cJSON_AddStringToObject(chip_info_obj, "model", "ESP32");
    cJSON_AddNumberToObject(chip_info_obj, "revision", chip_info.revision);
    cJSON_AddItemToObject(root, "chipInfo", chip_info_obj);
    
    // 跳过Flash大小信息获取
    
    // 添加可用内存信息
    cJSON_AddNumberToObject(root, "freeHeap", esp_get_free_heap_size());
    
    // 添加系统运行时间
    cJSON_AddNumberToObject(root, "uptime", esp_timer_get_time() / 1000000);
    
    // 添加GPIO管理器状态
    cJSON *gpio_status = cJSON_CreateObject();
    
    // 获取并添加GPIO组信息
    for (int i = 0; i < GPIO_GROUP_COUNT; i++) {
        gpio_group_t group_info;
        esp_err_t err = gpio_manager_get_group_info(i, &group_info);
        if (err == ESP_OK) {
            cJSON *group_obj = cJSON_CreateObject();
            cJSON_AddBoolToObject(group_obj, "isInput", group_info.is_input[0]);
            
            cJSON *states_array = cJSON_CreateArray();
            for (int j = 0; j < GPIO_PER_GROUP; j++) {
                cJSON_AddItemToArray(states_array, cJSON_CreateNumber(group_info.states[j]));
            }
            cJSON_AddItemToObject(group_obj, "states", states_array);
            
            char group_key[10];
            sprintf(group_key, "group%d", i);
            cJSON_AddItemToObject(gpio_status, group_key, group_obj);
        }
    }
    
    // 获取并添加电机信息
    for (int i = 0; i < MOTOR_GROUP_COUNT; i++) {
        motor_group_t motor_info;
        esp_err_t err = gpio_manager_get_motor_info(i, &motor_info);
        if (err == ESP_OK) {
            cJSON *motor_obj = cJSON_CreateObject();
            cJSON *states_array = cJSON_CreateArray();
            for (int j = 0; j < GPIO_PER_GROUP; j++) {
                cJSON_AddItemToArray(states_array, cJSON_CreateNumber(motor_info.states[j]));
            }
            cJSON_AddItemToObject(motor_obj, "states", states_array);
            
            char motor_key[10];
            sprintf(motor_key, "motor%d", i);
            cJSON_AddItemToObject(gpio_status, motor_key, motor_obj);
        }
    }
    
    cJSON_AddItemToObject(root, "gpioStatus", gpio_status);
    
    // 转换JSON为字符串
    char *json_response = cJSON_Print(root);
    if (!json_response) {
        ESP_LOGE(TAG, "无法打印JSON数据");
        cJSON_Delete(root);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    // 发送响应
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
    
    // 释放资源
    free(json_response);
    cJSON_Delete(root);
    
    return ESP_OK;
}