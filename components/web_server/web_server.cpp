#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <ctype.h>

// ESP-IDF组件头文件
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_err.h"
#include "cJSON.h"
#include "esp_private/esp_clk.h"

// 自定义组件头文件
#include "web_server.h"
#include "gpio_manager.h"
#include "motor_control.h"
#include "state_machine.h"
#include "logic_circuit.h"
#include "wifi_config_store.h"
#include "wifi_manager.h"

// 函数前向声明
esp_err_t gpio_control_handler(httpd_req_t *req);
esp_err_t motor_control_handler(httpd_req_t *req);
esp_err_t system_status_handler(httpd_req_t *req);
esp_err_t logic_circuit_handler(httpd_req_t *req);
esp_err_t gpio_state_handler(httpd_req_t *req);
esp_err_t state_machine_http_handler(httpd_req_t *req);
esp_err_t wifi_config_handler(httpd_req_t *req);
esp_err_t ping_handler(httpd_req_t *req);

// TAG常量，用于ESP_LOG函数
static const char *TAG = "WEB_SERVER";

// 辅助函数：注册URI处理程序并检查结果
static void register_uri_checked(httpd_handle_t server, httpd_uri_t *uri, const char *method_desc)
{
    esp_err_t res = httpd_register_uri_handler(server, uri);
    if (res != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register URI %s %s: %s", method_desc, uri->uri, esp_err_to_name(res));
    }
}

// 启动HTTP服务器
httpd_handle_t web_server_start(void)
{
    httpd_handle_t server = NULL;
    
    // 配置HTTP服务器
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 20;
    config.stack_size = 16384;
    
    // 启动HTTP服务器
    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return NULL;
    }
    
    // 注册URI处理程序
    // GPIO相关URI
    httpd_uri_t gpio_uri = {
        .uri      = "/gpio",
        .method   = HTTP_GET,
        .handler  = gpio_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &gpio_uri, "GET");
    
    // 注册GPIO POST方法处理程序
    httpd_uri_t gpio_post_uri = {
        .uri      = "/gpio",
        .method   = HTTP_POST,
        .handler  = gpio_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &gpio_post_uri, "POST");
    
    // 注册GPIO PUT方法处理程序
    httpd_uri_t gpio_put_uri = {
        .uri      = "/gpio",
        .method   = HTTP_PUT,
        .handler  = gpio_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &gpio_put_uri, "PUT");
    
    // 注册GPIO OPTIONS方法处理程序（用于CORS预检）
    httpd_uri_t gpio_options_uri = {
        .uri      = "/gpio",
        .method   = HTTP_OPTIONS,
        .handler  = gpio_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &gpio_options_uri, "OPTIONS");
    
    // 注册GPIO状态获取处理程序
    httpd_uri_t gpio_state_uri = {
        .uri      = "/gpio/state",
        .method   = HTTP_GET,
        .handler  = gpio_state_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &gpio_state_uri, "GET");
    
    // 同时注册/gpio_state端点，以兼容客户端请求
    httpd_uri_t gpio_state_uri_alt = {
        .uri      = "/gpio_state",
        .method   = HTTP_GET,
        .handler  = gpio_state_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &gpio_state_uri_alt, "GET");
    
    // 电机控制相关URI
    httpd_uri_t motor_uri_get = {
        .uri      = "/motor",
        .method   = HTTP_GET,
        .handler  = motor_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &motor_uri_get, "GET");

    httpd_uri_t motor_uri_post = {
        .uri      = "/motor",
        .method   = HTTP_POST,
        .handler  = motor_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &motor_uri_post, "POST");

    httpd_uri_t motor_uri_options = {
        .uri      = "/motor",
        .method   = HTTP_OPTIONS,
        .handler  = motor_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &motor_uri_options, "OPTIONS");
    
    // 系统状态获取URI
    httpd_uri_t status_uri = {
        .uri      = "/status",
        .method   = HTTP_GET,
        .handler  = system_status_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &status_uri, "GET");
    
    // 逻辑电路控制URI
    httpd_uri_t logic_uri = {
        .uri      = "/logic",
        .method   = HTTP_POST,
        .handler  = logic_circuit_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &logic_uri, "POST");
    
    // 逻辑电路OPTIONS请求处理
    httpd_uri_t logic_options_uri = {
        .uri      = "/logic",
        .method   = HTTP_OPTIONS,
        .handler  = logic_circuit_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &logic_options_uri, "OPTIONS");
    
    // 兼容旧版客户端的逻辑电路URI
    httpd_uri_t logic_circuit_uri = {
        .uri      = "/logic_circuit",
        .method   = HTTP_POST,
        .handler  = logic_circuit_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &logic_circuit_uri, "POST");
    
    // 兼容旧版客户端的逻辑电路OPTIONS请求处理
    httpd_uri_t logic_circuit_options_uri = {
        .uri      = "/logic_circuit",
        .method   = HTTP_OPTIONS,
        .handler  = logic_circuit_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &logic_circuit_options_uri, "OPTIONS");
    
    // 状态机相关URI
    httpd_uri_t state_machine_uri = {
        .uri      = "/state_machine",
        .method   = HTTP_GET,
        .handler  = state_machine_http_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &state_machine_uri, "GET");
    
    httpd_uri_t state_machine_post_uri = {
        .uri      = "/state_machine",
        .method   = HTTP_POST,
        .handler  = state_machine_http_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &state_machine_post_uri, "POST");
    
    httpd_uri_t state_machine_options_uri = {
        .uri      = "/state_machine",
        .method   = HTTP_OPTIONS,
        .handler  = state_machine_http_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &state_machine_options_uri, "OPTIONS");

    // WiFi配置URI
    httpd_uri_t wifi_config_get_uri = {
        .uri      = "/wifi/config",
        .method   = HTTP_GET,
        .handler  = wifi_config_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &wifi_config_get_uri, "GET");

    httpd_uri_t wifi_config_post_uri = {
        .uri      = "/wifi/config",
        .method   = HTTP_POST,
        .handler  = wifi_config_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &wifi_config_post_uri, "POST");

    httpd_uri_t wifi_config_options_uri = {
        .uri      = "/wifi/config",
        .method   = HTTP_OPTIONS,
        .handler  = wifi_config_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &wifi_config_options_uri, "OPTIONS");

    httpd_uri_t wifi_config_delete_uri = {
        .uri      = "/wifi/config",
        .method   = HTTP_DELETE,
        .handler  = wifi_config_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &wifi_config_delete_uri, "DELETE");

    // Ping健康检查URI
    httpd_uri_t ping_uri = {
        .uri      = "/ping",
        .method   = HTTP_GET,
        .handler  = ping_handler,
        .user_ctx = NULL
    };
    register_uri_checked(server, &ping_uri, "GET");

    ESP_LOGI(TAG, "HTTP server started successfully, listening on port: %d", config.server_port);
    return server;
}

// 停止HTTP服务器
esp_err_t web_server_stop(httpd_handle_t server)
{
    if (server == NULL) {
        ESP_LOGE(TAG, "Invalid server handle");
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = httpd_stop(server);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP服务器已停止");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "停止HTTP服务器失败: %s", esp_err_to_name(err));
        return err;
    }
}

// 处理GPIO控制请求
esp_err_t gpio_control_handler(httpd_req_t *req)
{
    // 处理OPTIONS请求（CORS预检）
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    // 对于GET请求，返回GPIO控制帮助信息
    if (req->method == HTTP_GET) {
        const char *resp = "{\"status\": \"success\", \"message\": \"GPIO control API\", \"usage\": \"POST/PUT JSON with {\\\"group_id\\\": <id>, \\\"pin_index\\\": <index>, \\\"value\\\": <0|1>}\", \"available_groups\": [0, 1, 2, 3]}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_OK;
    }
    
    // 对于POST和PUT请求，处理GPIO控制
    if (req->method == HTTP_POST || req->method == HTTP_PUT) {
        char *buf = (char *)malloc(req->content_len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to allocate buffer");
            return ESP_FAIL;
        }
    
        // 读取请求体
        int ret = httpd_req_recv(req, buf, req->content_len);
        if (ret <= 0) {
            free(buf);
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
        buf[ret] = '\0';
    
        // 解析JSON
        cJSON *root = cJSON_Parse(buf);
        if (root == NULL) {
            ESP_LOGE(TAG, "Failed to parse JSON");
            free(buf);
            return ESP_FAIL;
        }
    
        // 检查是否是完整配置格式（包含groupModes和outputStates）
        cJSON *groupModes_json = cJSON_GetObjectItem(root, "groupModes");
        cJSON *outputStates_json = cJSON_GetObjectItem(root, "outputStates");
    
        if (groupModes_json) {
            // 处理完整配置格式
            cJSON *group_item;
            char group_key[4];

            for (int i = 0; i < 12; i++) {
                sprintf(group_key, "%d", i);
                group_item = cJSON_GetObjectItem(groupModes_json, group_key);
                if (group_item && cJSON_IsNumber(group_item)) {
                    gpio_manager_set_group_mode(i, group_item->valueint);
                }
            }
        }

        if (outputStates_json) {
            cJSON *group_item;
            char group_key[4];

            for (int i = 0; i < 12; i++) {
                sprintf(group_key, "%d", i);
                group_item = cJSON_GetObjectItem(outputStates_json, group_key);
                if (group_item && cJSON_IsObject(group_item)) {
                    cJSON *pin_item;
                    char pin_key[4];
                    for (int j = 0; j < 2; j++) {
                        sprintf(pin_key, "%d", j);
                        pin_item = cJSON_GetObjectItem(group_item, pin_key);
                        if (pin_item && cJSON_IsNumber(pin_item)) {
                            gpio_manager_set_level(i, j, pin_item->valueint);
                        }
                    }
                }
            }
        }

        // 发送成功响应
        const char *resp = "{\"status\": \"success\", \"message\": \"GPIO configuration updated\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, resp, strlen(resp));
    
        // 释放资源
        cJSON_Delete(root);
        free(buf);
        return ESP_OK;
    }
    return ESP_OK;
}

// 处理GPIO状态请求
esp_err_t gpio_state_handler(httpd_req_t *req)
{
    // 处理OPTIONS请求（CORS预检）
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, OPTIONS");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    // 对于GET请求，返回GPIO状态
    if (req->method == HTTP_GET) {
        // 创建JSON响应
        cJSON *root = cJSON_CreateObject();
        if (root == NULL) {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        // 添加成功状态
        cJSON_AddStringToObject(root, "status", "success");
        
        // 创建GPIO状态数组
        cJSON *gpio_states = cJSON_CreateObject();
        if (gpio_states == NULL) {
            cJSON_Delete(root);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        // 获取并添加所有GPIO组的状态
        for (int group_id = 0; group_id < GPIO_GROUP_COUNT; group_id++) {
            cJSON *group_obj = cJSON_CreateObject();
            if (group_obj == NULL) {
                cJSON_Delete(root);
                cJSON_Delete(gpio_states);
                httpd_resp_send_500(req);
                return ESP_FAIL;
            }
            
            for (int pin_index = 0; pin_index < GPIO_PER_GROUP; pin_index++) {
                int level = 0;
                if (gpio_manager_get_level(group_id, pin_index, &level) == ESP_OK) {
                    cJSON_AddNumberToObject(group_obj, "pin", pin_index);
                    cJSON_AddNumberToObject(group_obj, "value", level);
                } else {
                    cJSON_AddNumberToObject(group_obj, "pin", pin_index);
                    cJSON_AddNumberToObject(group_obj, "value", 0);
                }
            }
            
            char group_key[10];
            sprintf(group_key, "group%d", group_id);
            cJSON_AddItemToObject(gpio_states, group_key, group_obj);
        }
        
        // 将GPIO状态数组添加到根对象
        cJSON_AddItemToObject(root, "gpio_states", gpio_states);
        
        // 转换为字符串
        char *json_str = cJSON_PrintUnformatted(root);
        if (json_str == NULL) {
            cJSON_Delete(root);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        // 发送响应
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, json_str, strlen(json_str));
        
        // 释放资源
        cJSON_Delete(root);
        free(json_str);
        return ESP_OK;
    }
    
    return ESP_OK;
}

// 处理电机控制请求 (此函数现在不再直接由URI调用，但保留以备将来使用或重构)
esp_err_t motor_control_handler(httpd_req_t *req)
{
    // ... (函数体保持不变，但不再注册到 /motor URI)
    // 处理OPTIONS请求（CORS预检）
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }

    int motor_id = -1;
    int direction = -1;

    if (req->method == HTTP_GET) {
        char buf[100];
        if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
            char param[32];
            if (httpd_query_key_value(buf, "motor", param, sizeof(param)) == ESP_OK) {
                motor_id = atoi(param);
            }
            if (httpd_query_key_value(buf, "direction", param, sizeof(param)) == ESP_OK) {
                direction = atoi(param);
            }
        }
    } else if (req->method == HTTP_POST) {
        char *buf = (char *)malloc(req->content_len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to allocate buffer");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
    
        int ret = httpd_req_recv(req, buf, req->content_len);
        if (ret <= 0) {
            free(buf);
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            } else {
                httpd_resp_send_500(req);
            }
            return ESP_FAIL;
        }
        buf[ret] = '\0';
    
        cJSON *root = cJSON_Parse(buf);
        if (root == NULL) {
            ESP_LOGE(TAG, "Failed to parse JSON");
            free(buf);
            httpd_resp_set_status(req, "400 Bad Request");
            httpd_resp_send(req, "{\"status\":\"error\", \"message\":\"Invalid JSON\"}", -1);
            return ESP_FAIL;
        }
    
        cJSON *motor_id_json = cJSON_GetObjectItem(root, "motor_id");
        cJSON *direction_json = cJSON_GetObjectItem(root, "direction");
    
        if (motor_id_json) motor_id = motor_id_json->valueint;
        if (direction_json) direction = direction_json->valueint;
    
        cJSON_Delete(root);
        free(buf);
    }

    if (motor_id >= 0 && motor_id < MOTOR_GROUP_COUNT &&
        (direction >= -9 && direction <= 9)) {
        
        gpio_manager_set_motor(motor_id, direction);
        
        const char *resp = "{\"status\": \"success\", \"message\": \"Motor command sent\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, resp, strlen(resp));
    } else {
        const char *resp = "{\"status\": \"error\", \"message\": \"Invalid motor parameters\"}";
        httpd_resp_set_status(req, "400 Bad Request");
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, resp, strlen(resp));
    }
    
    return ESP_OK;
}

// 处理系统状态请求
esp_err_t system_status_handler(httpd_req_t *req)
{
    // 处理OPTIONS请求（CORS预检）
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, OPTIONS");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    // 对于GET请求，返回系统状态
    if (req->method == HTTP_GET) {
        // 获取系统信息
        uint32_t free_heap = esp_get_free_heap_size();
        uint32_t min_free_heap = esp_get_minimum_free_heap_size();
        uint32_t cpu_freq = esp_clk_cpu_freq() / 1000000; // 转换为MHz
        
        // 创建JSON响应
        cJSON *root = cJSON_CreateObject();
        if (root == NULL) {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        // 添加系统信息
        cJSON_AddNumberToObject(root, "free_heap_bytes", free_heap);
        cJSON_AddNumberToObject(root, "min_free_heap_bytes", min_free_heap);
        cJSON_AddNumberToObject(root, "cpu_freq_mhz", cpu_freq);
        cJSON_AddStringToObject(root, "status", "running");
        cJSON_AddStringToObject(root, "version", "v1.0.0");
        cJSON_AddStringToObject(root, "device", "Yonovate V2");
        
        // 转换为字符串
        char *json_str = cJSON_PrintUnformatted(root);
        if (json_str == NULL) {
            cJSON_Delete(root);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        // 发送响应
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, json_str, strlen(json_str));
        
        // 释放资源
        cJSON_Delete(root);
        free(json_str);
        return ESP_OK;
    }
    
    return ESP_OK;
}

// 逻辑电路请求处理函数在逻辑电路组件中实现
// 此处仅保留函数声明，实际实现请参考 logic_circuit.cpp
 * @brief WiFi配置处理程序
 * @param req HTTP请求结构体
 * @return esp_err_t ESP_OK成功，其他错误码失败
 * @details 处理GET/POST/DELETE/OPTIONS请求，支持查询、更新和清除WiFi配置
 *          GET: 返回当前存储的WiFi配置（不含密码）
 *          POST: 更新WiFi配置并触发重连
 *          DELETE: 清除NVS中的WiFi配置（恢复未配网状态）
 *          OPTIONS: CORS预检响应
 */
esp_err_t wifi_config_handler(httpd_req_t *req)
{
    // 处理OPTIONS请求（CORS预检）
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }

    // GET请求 - 返回当前存储的WiFi配置
    if (req->method == HTTP_GET) {
        char ssid[33] = {0};
        char pass[65] = {0};
        cJSON *root = cJSON_CreateObject();
        if (root == NULL) {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        if (wifi_config_store_load(ssid, sizeof(ssid), pass, sizeof(pass)) == ESP_OK) {
            cJSON_AddStringToObject(root, "status", "success");
            cJSON_AddBoolToObject(root, "stored", true);
            cJSON_AddStringToObject(root, "ssid", ssid);
        } else {
            cJSON_AddStringToObject(root, "status", "success");
            cJSON_AddBoolToObject(root, "stored", false);
            cJSON_AddStringToObject(root, "ssid", "");
        }

        char *json_str = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);
        if (json_str == NULL) {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, json_str, strlen(json_str));
        free(json_str);
        return ESP_OK;
    }

    // POST请求 - 更新WiFi配置
    if (req->method == HTTP_POST) {
        // 限制请求体大小
        if (req->content_len == 0 || req->content_len > 512) {
            const char *err = "{\"status\":\"error\",\"message\":\"请求体大小无效\"}";
            httpd_resp_set_status(req, "400 Bad Request");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_send(req, err, strlen(err));
            return ESP_FAIL;
        }

        char *buf = (char *)malloc(req->content_len + 1);
        if (buf == NULL) {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        int r = httpd_req_recv(req, buf, req->content_len);
        if (r <= 0) {
            free(buf);
            if (r == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            return ESP_FAIL;
        }
        buf[r] = '\0';

        cJSON *json = cJSON_Parse(buf);
        free(buf);
        if (json == NULL) {
            const char *err = "{\"status\":\"error\",\"message\":\"JSON格式错误\"}";
            httpd_resp_set_status(req, "400 Bad Request");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_send(req, err, strlen(err));
            return ESP_FAIL;
        }

        cJSON *ssid_item = cJSON_GetObjectItem(json, "ssid");
        cJSON *pass_item = cJSON_GetObjectItem(json, "password");
        if (!ssid_item || !cJSON_IsString(ssid_item) || ssid_item->valuestring == NULL ||
            strlen(ssid_item->valuestring) == 0) {
            cJSON_Delete(json);
            const char *err = "{\"status\":\"error\",\"message\":\"缺少SSID参数\"}";
            httpd_resp_set_status(req, "400 Bad Request");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_send(req, err, strlen(err));
            return ESP_FAIL;
        }

        const char *ssid_str = ssid_item->valuestring;
        const char *pass_str = "";
        if (pass_item && cJSON_IsString(pass_item) && pass_item->valuestring != NULL) {
            pass_str = pass_item->valuestring;
        }

        esp_err_t saverr = wifi_config_store_save(ssid_str, pass_str);
        if (saverr != ESP_OK) {
            char msg[128];
            snprintf(msg, sizeof(msg), "{\"status\":\"error\",\"message\":\"NVS保存失败: %s\"}",
                     esp_err_to_name(saverr));
            httpd_resp_set_status(req, "500 Internal Server Error");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_send(req, msg, strlen(msg));
            cJSON_Delete(json);
            return ESP_FAIL;
        }

        esp_err_t werr = wifi_manager_update_credentials(ssid_str, pass_str);

        if (werr != ESP_OK) {
            char msg[128];
            snprintf(msg, sizeof(msg), "{\"status\":\"warn\",\"message\":\"已保存但重连失败: %s\"}",
                     esp_err_to_name(werr));
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_send(req, msg, strlen(msg));
            cJSON_Delete(json);
            return ESP_OK;
        }

        const char *ok = "{\"status\":\"success\",\"message\":\"WiFi配置已保存，正在重新连接\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, ok, strlen(ok));
        cJSON_Delete(json);
        return ESP_OK;
    }

    // DELETE请求 - 清除WiFi配置
    if (req->method == HTTP_DELETE) {
        ESP_LOGI(TAG, "接收到清除WiFi配置请求");

        esp_err_t err = wifi_config_store_erase();
        if (err != ESP_OK) {
            char msg[128];
            snprintf(msg, sizeof(msg), "{\"status\":\"error\",\"message\":\"清除配置失败: %s\"}",
                     esp_err_to_name(err));
            httpd_resp_set_status(req, "500 Internal Server Error");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_send(req, msg, strlen(msg));
            return ESP_FAIL;
        }

        const char *ok = "{\"status\":\"success\",\"message\":\"WiFi配置已清除，请重新配网\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, ok, strlen(ok));
        ESP_LOGI(TAG, "WiFi配置已清除");
        return ESP_OK;
    }

    return ESP_OK;
}

/**
 * @brief Ping健康检查处理程序
 * @param req HTTP请求结构体
 * @return esp_err_t ESP_OK成功
 * @details 返回简单的JSON状态，用于前端连通性测试
 *          响应格式: {"status":"ok"}
 */
esp_err_t ping_handler(httpd_req_t *req)
{
    // 处理OPTIONS请求（CORS预检）
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, OPTIONS");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }

    // GET请求 - 返回健康状态
    if (req->method == HTTP_GET) {
        const char *resp = "{\"status\":\"ok\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, resp, strlen(resp));
        return ESP_OK;
    }

    return ESP_OK;
}