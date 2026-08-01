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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// 自定义组件头文件
#include "web_server.h"
#include "gpio_manager.h"
#include "motor_control.h"
#include "state_machine.h"
#include "logic_circuit.h"
#include "wifi_config_store.h"
#include "wifi_manager.h"
#include "sm_http.h"
#include "sm_version.h"
#include "nvs_manager.h"
#include "gpio_monitor.h"

// 函数前向声明
esp_err_t gpio_control_handler(httpd_req_t *req);
esp_err_t motor_control_handler(httpd_req_t *req);
esp_err_t system_status_handler(httpd_req_t *req);
esp_err_t logic_circuit_handler(httpd_req_t *req);
esp_err_t gpio_state_handler(httpd_req_t *req);
esp_err_t state_machine_http_handler(httpd_req_t *req);
esp_err_t wifi_config_handler(httpd_req_t *req);
esp_err_t ping_handler(httpd_req_t *req);
esp_err_t sm_version_handler(httpd_req_t *req);
esp_err_t favicon_handler(httpd_req_t *req);
static esp_err_t wifi_config_page_handler(httpd_req_t *req);
static esp_err_t sm_config_page_handler(httpd_req_t *req);

// TAG常量，用于ESP_LOG函数
static const char *TAG = "WEB_SERVER";

// 静态变量（新）
static httpd_handle_t s_server = NULL;  // 内部管理的服务器句柄
static int s_wifi_cb_id = -1;             // WiFi 回调 ID

// 前向声明
static void on_wifi_status_changed(wifi_status_t status, const char* ip, void* arg);

// 辅助函数：注册URI处理程序并检查结果
static void register_uri_checked(httpd_handle_t server, httpd_uri_t *uri, const char *method_desc)
{
    esp_err_t res = httpd_register_uri_handler(server, uri);
    if (res != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register URI %s %s: %s", method_desc, uri->uri, esp_err_to_name(res));
    }
}

// WiFi 状态变化回调
static void on_wifi_status_changed(wifi_status_t status, const char* ip, void *arg)
{
    (void)arg;

    // 判断是否需要启动 Web Server
    bool should_run = (status == WIFI_STATUS_CONNECTED ||
                       status == WIFI_STATUS_AP_MODE);

    // 更新 IP 地址到监控器（任何有 IP 的状态变化都更新）
    if (ip && *ip) {
        gpio_monitor_update_ip(ip);
    }

    if (should_run && !s_server) {
        // 网络就绪，启动 Web Server
        esp_err_t err = web_server_start();
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Web Server 自动启动 (%s)",
                     status == WIFI_STATUS_AP_MODE ? "AP" : "STA");
        } else {
            ESP_LOGE(TAG, "Web Server 自动启动失败");
        }
    } else if (!should_run && s_server) {
        // 网络断开，停止 Web Server
        web_server_stop();
        ESP_LOGI(TAG, "Web Server 自动停止 (WiFi 断开)");
    }
}

// 公共启动接口
esp_err_t web_server_start(void)
{
    if (s_server) {
        ESP_LOGW(TAG, "Web Server 已在运行");
        return ESP_OK;
    }

    // 检查 WiFi 是否就绪（对回调调用也无害，因为状态已就绪）
    wifi_status_t status = wifi_manager_get_status();
    if (status != WIFI_STATUS_CONNECTED && status != WIFI_STATUS_AP_MODE) {
        ESP_LOGW(TAG, "无法启动：WiFi 未就绪 (状态=%d)", status);
        return ESP_ERR_INVALID_STATE;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 35;  // 增加槽位以支持所有API
    config.stack_size = 16384;

    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "启动 HTTP 服务器失败");
        return ESP_FAIL;
    }

    // 注册URI处理程序
    // GPIO相关URI
    httpd_uri_t gpio_uri = {
        .uri      = "/gpio",
        .method   = HTTP_GET,
        .handler  = gpio_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &gpio_uri, "GET");

    // 注册GPIO POST方法处理程序
    httpd_uri_t gpio_post_uri = {
        .uri      = "/gpio",
        .method   = HTTP_POST,
        .handler  = gpio_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &gpio_post_uri, "POST");

    // 注册GPIO PUT方法处理程序
    httpd_uri_t gpio_put_uri = {
        .uri      = "/gpio",
        .method   = HTTP_PUT,
        .handler  = gpio_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &gpio_put_uri, "PUT");

    // 注册GPIO OPTIONS方法处理程序（用于CORS预检）
    httpd_uri_t gpio_options_uri = {
        .uri      = "/gpio",
        .method   = HTTP_OPTIONS,
        .handler  = gpio_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &gpio_options_uri, "OPTIONS");

    // 注册GPIO状态获取处理程序
    httpd_uri_t gpio_state_uri = {
        .uri      = "/gpio/state",
        .method   = HTTP_GET,
        .handler  = gpio_state_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &gpio_state_uri, "GET");

    // 同时注册/gpio_state端点，以兼容客户端请求
    httpd_uri_t gpio_state_uri_alt = {
        .uri      = "/gpio_state",
        .method   = HTTP_GET,
        .handler  = gpio_state_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &gpio_state_uri_alt, "GET");

    // 电机控制相关URI
    httpd_uri_t motor_uri_get = {
        .uri      = "/motor",
        .method   = HTTP_GET,
        .handler  = motor_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &motor_uri_get, "GET");

    httpd_uri_t motor_uri_post = {
        .uri      = "/motor",
        .method   = HTTP_POST,
        .handler  = motor_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &motor_uri_post, "POST");

    httpd_uri_t motor_uri_options = {
        .uri      = "/motor",
        .method   = HTTP_OPTIONS,
        .handler  = motor_control_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &motor_uri_options, "OPTIONS");

    // 系统状态获取URI
    httpd_uri_t status_uri = {
        .uri      = "/status",
        .method   = HTTP_GET,
        .handler  = system_status_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &status_uri, "GET");

    // 逻辑电路控制URI
    httpd_uri_t logic_uri = {
        .uri      = "/logic",
        .method   = HTTP_POST,
        .handler  = logic_circuit_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &logic_uri, "POST");

    // 逻辑电路OPTIONS请求处理
    httpd_uri_t logic_options_uri = {
        .uri      = "/logic",
        .method   = HTTP_OPTIONS,
        .handler  = logic_circuit_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &logic_options_uri, "OPTIONS");

    // 兼容旧版客户端的逻辑电路URI
    httpd_uri_t logic_circuit_uri = {
        .uri      = "/logic_circuit",
        .method   = HTTP_POST,
        .handler  = logic_circuit_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &logic_circuit_uri, "POST");

    // 兼容旧版客户端的逻辑电路OPTIONS请求处理
    httpd_uri_t logic_circuit_options_uri = {
        .uri      = "/logic_circuit",
        .method   = HTTP_OPTIONS,
        .handler  = logic_circuit_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &logic_circuit_options_uri, "OPTIONS");

    // 状态机相关URI
    httpd_uri_t state_machine_uri = {
        .uri      = "/state_machine",
        .method   = HTTP_GET,
        .handler  = state_machine_http_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &state_machine_uri, "GET");

    httpd_uri_t state_machine_post_uri = {
        .uri      = "/state_machine",
        .method   = HTTP_POST,
        .handler  = state_machine_http_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &state_machine_post_uri, "POST");

    httpd_uri_t state_machine_options_uri = {
        .uri      = "/state_machine",
        .method   = HTTP_OPTIONS,
        .handler  = state_machine_http_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &state_machine_options_uri, "OPTIONS");

    /* 状态机V3.0 API端点 - 通过V3组件注册（组件化封装）
     * sm_v3_register_endpoints() 注册 /api/v3/state-machine
     * 包含 GET/POST/DELETE/OPTIONS 四个方法
     */
    sm_v3_register_endpoints(s_server);

    // WiFi配置URI（带/api前缀，与main.cpp提示一致）
    httpd_uri_t wifi_config_get_uri = {
        .uri      = "/api/wifi/config",
        .method   = HTTP_GET,
        .handler  = wifi_config_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &wifi_config_get_uri, "GET");

    httpd_uri_t wifi_config_post_uri = {
        .uri      = "/api/wifi/config",
        .method   = HTTP_POST,
        .handler  = wifi_config_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &wifi_config_post_uri, "POST");

    httpd_uri_t wifi_config_options_uri = {
        .uri      = "/api/wifi/config",
        .method   = HTTP_OPTIONS,
        .handler  = wifi_config_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &wifi_config_options_uri, "OPTIONS");

    httpd_uri_t wifi_config_delete_uri = {
        .uri      = "/api/wifi/config",
        .method   = HTTP_DELETE,
        .handler  = wifi_config_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &wifi_config_delete_uri, "DELETE");

    // 状态机版本配置URI
    httpd_uri_t sm_version_get_uri = {
        .uri      = "/api/sm/version",
        .method   = HTTP_GET,
        .handler  = sm_version_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &sm_version_get_uri, "GET");

    httpd_uri_t sm_version_post_uri = {
        .uri      = "/api/sm/version",
        .method   = HTTP_POST,
        .handler  = sm_version_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &sm_version_post_uri, "POST");

    httpd_uri_t sm_version_options_uri = {
        .uri      = "/api/sm/version",
        .method   = HTTP_OPTIONS,
        .handler  = sm_version_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &sm_version_options_uri, "OPTIONS");

    // Ping健康检查URI
    httpd_uri_t ping_uri = {
        .uri      = "/ping",
        .method   = HTTP_GET,
        .handler  = ping_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &ping_uri, "GET");

    // WiFi配置页面URI（内联HTML，无需文件系统）
    httpd_uri_t wifi_page_uri = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = wifi_config_page_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &wifi_page_uri, "GET");

    // 状态机配置页面URI
    httpd_uri_t sm_config_page_uri = {
        .uri      = "/sm-config",
        .method   = HTTP_GET,
        .handler  = sm_config_page_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &sm_config_page_uri, "GET");

    // Favicon URI - 浏览器默认请求，避免 404 警告
    httpd_uri_t favicon_uri = {
        .uri      = "/favicon.ico",
        .method   = HTTP_GET,
        .handler  = favicon_handler,
        .user_ctx = NULL
    };
    register_uri_checked(s_server, &favicon_uri, "GET");

    ESP_LOGI(TAG, "HTTP server started successfully, listening on port: %d", config.server_port);
    return ESP_OK;
}

// 公共停止接口
esp_err_t web_server_stop(void)
{
    if (!s_server) {
        ESP_LOGW(TAG, "Web Server 未运行");
        return ESP_OK;
    }

    esp_err_t err = httpd_stop(s_server);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP 服务器已停止");
    } else {
        ESP_LOGE(TAG, "停止 HTTP 服务器失败: %s", esp_err_to_name(err));
    }
    s_server = NULL;
    return ESP_OK;
}

// 初始化 Web Server，注册 WiFi 状态监听
void web_server_init(void)
{
    // 注册 WiFi 状态监听
    s_wifi_cb_id = wifi_manager_register_status_callback(
        on_wifi_status_changed, NULL);

    if (s_wifi_cb_id > 0) {
        ESP_LOGI(TAG, "Web Server 初始化完成，WiFi 监听已注册 (ID=%d)", s_wifi_cb_id);
    } else {
        ESP_LOGE(TAG, "Web Server 初始化失败：无法注册 WiFi 监听");
    }
}

// 检查运行状态
bool web_server_is_running(void)
{
    return s_server != NULL;
}

// 获取服务器句柄
httpd_handle_t web_server_get_handle(void)
{
    return s_server;
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
/**
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

        // 使用 wifi_config_store 组件读取 WiFi 配置
        esp_err_t load_err = wifi_config_store_load(ssid, sizeof(ssid), pass, sizeof(pass));
        if (load_err == ESP_OK && strlen(ssid) > 0) {
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
        ESP_LOGI(TAG, "接收到WiFi配置POST请求, content_len=%d", req->content_len);
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
            const char *err = "{\"status\":\"error\",\"message\":\"JSON parse error\"}";
            httpd_resp_set_status(req, "400 Bad Request");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_send(req, err, strlen(err));
            return ESP_FAIL;
        }

        cJSON *ssid_item = cJSON_GetObjectItem(json, "ssid");
        cJSON *pass_item = cJSON_GetObjectItem(json, "password");
        cJSON *is_default_item = cJSON_GetObjectItem(json, "is_default");
        if (!ssid_item || !cJSON_IsString(ssid_item) || ssid_item->valuestring == NULL ||
            strlen(ssid_item->valuestring) == 0) {
            cJSON_Delete(json);
            const char *err = "{\"status\":\"error\",\"message\":\"Missing SSID param\"}";
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

        // Check if this is restore default config operation
        bool is_restore_default = (is_default_item && cJSON_IsBool(is_default_item) &&
                                   cJSON_IsTrue(is_default_item));

        // 保存到 NVS (使用 wifi_config_store 组件)
        esp_err_t saverr = wifi_config_store_save(ssid_str, pass_str);
        if (saverr != ESP_OK) {
            cJSON_Delete(json);
            char msg[128];
            snprintf(msg, sizeof(msg), "{\"status\":\"error\",\"message\":\"NVS save failed: %s\"}",
                     esp_err_to_name(saverr));
            httpd_resp_set_status(req, "500 Internal Server Error");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_send(req, msg, strlen(msg));
            return ESP_FAIL;
        }

        // 立即返回成功响应，提示需要重启
        const char *ok_msg = is_restore_default
            ? "{\"status\":\"success\",\"message\":\"默认配置已恢复，请重启设备以应用新的WiFi设置\"}"
            : "{\"status\":\"success\",\"message\":\"WiFi配置已保存，请重启设备以应用新的WiFi设置\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, ok_msg, strlen(ok_msg));
        cJSON_Delete(json);
        return ESP_OK;
    }

    // DELETE请求 - 清除WiFi配置
    if (req->method == HTTP_DELETE) {
        ESP_LOGI(TAG, "接收到清除WiFi配置请求");

        // 清除 NVS 中的 WiFi 配置 (使用 wifi_config_store 组件)
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

        const char *ok = "{\"status\":\"success\",\"message\":\"WiFi配置已清除，请重启设备以应用更改\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, ok, strlen(ok));
        ESP_LOGI(TAG, "WiFi配置已清除，需重启生效");
        return ESP_OK;
    }

    return ESP_OK;
}

/**
 * @brief 内联WiFi配置页面HTML内容
 * @details 简化的单页面配置界面，直接嵌入固件，无需文件系统
 */
static const char* wifi_config_page_html = R"rawliteral(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 WiFi配置</title>
    <style>
        body { font-family: Arial; max-width: 400px; margin: 20px auto; padding: 10px; background: #f5f5f5; }
        h1 { color: #333; text-align: center; margin-bottom: 10px; }
        .subtitle { text-align: center; color: #666; font-size: 14px; margin-bottom: 20px; }
        .box { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; color: #555; }
        input { width: 100%; padding: 12px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; font-size: 16px; }
        input:focus { border-color: #4CAF50; outline: none; }
        button { width: 100%; padding: 14px; margin-top: 10px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }
        .btn-save { background: #4CAF50; color: white; }
        .btn-clear { background: #f44336; color: white; }
        .status { margin-top: 15px; padding: 12px; border-radius: 4px; font-size: 14px; }
        .success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .warn { background: #fff3cd; color: #856404; border: 1px solid #ffeaa7; }
        .info { background: #d1ecf1; color: #0c5460; border: 1px solid #bee5eb; }
        .current { background: #e7f3ff; padding: 12px; margin-bottom: 15px; border-radius: 4px; font-size: 14px; border-left: 4px solid #2196F3; }
    </style>
</head>
<body>
    <h1>ESP32 WiFi配置</h1>
    <div class="subtitle">配网模式 (AP: Yonovate-Setup) <span style="color:#999;font-size:12px;">v1.6</span></div>
    <div class="nav" style="text-align:center;margin-bottom:15px;font-size:13px;">
        <a href="/" style="color:#666;text-decoration:none;">WiFi配置</a> |
        <a href="/sm-config" style="color:#2196F3;text-decoration:none;">状态机配置</a>
    </div>
    <div class="box">
        <div class="current" id="current">正在连接设备...</div>
        <div class="form-group">
            <label>WiFi名称 (SSID)</label>
            <input type="text" id="ssid" placeholder="输入你的WiFi名称">
        </div>
        <div class="form-group">
            <label>WiFi密码</label>
            <input type="password" id="password" placeholder="至少8位，留空表示开放网络">
        </div>
        <button class="btn-save" onclick="save()">保存配置</button>
        <button class="btn-clear" onclick="clearConfig()">清除配置</button>
        <button style="background:#FF9800;color:white;width:100%;padding:14px;margin-top:10px;border:none;border-radius:4px;cursor:pointer;font-size:16px;" onclick="resetDefault()">恢复默认配置</button>
        <div id="status" style="margin-top:15px;padding:12px;border-radius:4px;display:none;"></div>

        <!-- 备选方案：传统表单提交 -->
        <form id="fallbackForm" action="/api/wifi/config" method="POST" style="display:none;margin-top:20px;padding:15px;border:1px dashed #ccc;background:#f9f9f9;">
            <p style="margin:0 0 10px 0;color:#666;">JavaScript方式失败，使用备选表单提交：</p>
            <input type="text" name="ssid" id="formSsid" placeholder="WiFi名称"><br><br>
            <input type="password" name="password" id="formPassword" placeholder="WiFi密码"><br><br>
            <input type="submit" value="表单方式保存" style="width:auto;padding:8px 16px;background:#2196F3;color:white;border:none;border-radius:4px;cursor:pointer;">
        </form>
    </div>
    <script>
        function showStatus(msg, type) {
            var el = document.getElementById('status');
            el.className = type;
            el.innerHTML = msg;
            el.style.display = 'block';
        }
        function loadCurrent() {
            document.getElementById('current').innerHTML = '<strong>状态:</strong> 正在连接设备...';
            var retryCount = 0;
            var maxRetries = 3;

            function tryLoad() {
                var xhr = new XMLHttpRequest();
                xhr.timeout = 5000;
                xhr.onreadystatechange = function() {
                    if (xhr.readyState === 4) {
                        if (xhr.status === 200) {
                            try {
                                var d = JSON.parse(xhr.responseText);
                                if (d.stored) {
                                    document.getElementById('current').innerHTML = '<strong>当前配置:</strong> ' + d.ssid;
                                    document.getElementById('ssid').value = d.ssid;
                                    showStatus('已保存配置: ' + d.ssid, 'success');
                                } else {
                                    document.getElementById('current').innerHTML = '<strong>当前状态:</strong> 未配置（等待配网）';
                                    document.getElementById('status').style.display = 'none';
                                }
                            } catch(e) {
                                document.getElementById('current').innerHTML = '<strong>当前状态:</strong> 数据解析错误';
                            }
                        } else if (xhr.status === 0) {
                            // Network error or timeout
                            if (retryCount < maxRetries) {
                                retryCount++;
                                document.getElementById('current').innerHTML = '<strong>状态:</strong> 连接中... (重试 ' + retryCount + '/' + maxRetries + ')';
                                setTimeout(tryLoad, 1000);
                            } else {
                                document.getElementById('current').innerHTML = '<strong>状态:</strong> 设备响应超时，请确认已连接到: <strong>Yonovate-Setup</strong>';
                            }
                        } else {
                            document.getElementById('current').innerHTML = '<strong>状态:</strong> HTTP错误 ' + xhr.status + '，请刷新重试';
                        }
                    }
                };
                xhr.onerror = function() {
                    if (retryCount < maxRetries) {
                        retryCount++;
                        document.getElementById('current').innerHTML = '<strong>状态:</strong> 连接中... (重试 ' + retryCount + '/' + maxRetries + ')';
                        setTimeout(tryLoad, 1000);
                    } else {
                        document.getElementById('current').innerHTML = '<strong>状态:</strong> 网络错误，请确认已连接到: <strong>Yonovate-Setup</strong>';
                    }
                };
                xhr.open('GET', '/api/wifi/config', true);
                xhr.send();
            }
            tryLoad();
        }
        function save() {
            var statusEl = document.getElementById('status');
            statusEl.style.display = 'block';
            statusEl.className = 'info';
            statusEl.innerHTML = '正在保存配置...';

            var ssid = document.getElementById('ssid').value.trim();
            var password = document.getElementById('password').value;

            if (!ssid) {
                statusEl.className = 'error';
                statusEl.innerHTML = '请输入WiFi名称（SSID）';
                return;
            }
            if (password.length > 0 && password.length < 8) {
                statusEl.className = 'error';
                statusEl.innerHTML = '密码至少需要8位，或留空连接开放网络';
                return;
            }

            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function() {
                if (xhr.readyState === 4) {
                    if (xhr.status === 200) {
                        try {
                            var d = JSON.parse(xhr.responseText);
                            if (d.status === 'success') {
                                statusEl.className = 'success';
                                statusEl.innerHTML = '<strong>保存成功！</strong><br>WiFi配置已写入ESP32<br>SSID: ' + ssid + '<br><small>' + (d.message || '请重启设备以应用新的WiFi设置') + '</small>';
                                document.getElementById('current').innerHTML = '<strong>当前配置:</strong> ' + ssid + ' <small>(待重启生效)</small>';
                            } else if (d.status === 'warn') {
                                // 配置已保存但连接失败（警告而非错误）
                                statusEl.className = 'warn';
                                statusEl.innerHTML = '<strong>配置已保存，但连接遇到问题：</strong><br>' + (d.message || '请检查WiFi信号或密码') + '<br><small>设备将保持AP配网模式，可重新配置</small>';
                                document.getElementById('current').innerHTML = '<strong>当前配置:</strong> ' + ssid + ' <small>(待连接)</small>';
                            } else {
                                statusEl.className = 'error';
                                statusEl.innerHTML = '<strong>保存失败:</strong> ' + (d.message || '未知错误');
                            }
                        } catch(e) {
                            statusEl.className = 'error';
                            statusEl.innerHTML = '响应解析失败: ' + e.message;
                        }
                    } else {
                        statusEl.className = 'error';
                        statusEl.innerHTML = '保存失败，HTTP错误: ' + xhr.status;
                    }
                }
            };
            xhr.onerror = function() {
                statusEl.className = 'error';
                statusEl.innerHTML = '请求出错（网络错误），请检查连接';
            };
            xhr.open('POST', '/api/wifi/config', true);
            xhr.setRequestHeader('Content-Type', 'application/json');
            xhr.send(JSON.stringify({ssid: ssid, password: password}));
        }
        function clearConfig() {
            if (!confirm('确定要清除WiFi配置吗？\n\n注意：需要重启设备才能应用更改。')) return;
            showStatus('正在清除配置...', 'info');
            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function() {
                if (xhr.readyState === 4) {
                    if (xhr.status === 200) {
                        try {
                            var d = JSON.parse(xhr.responseText);
                            if (d.status === 'success') {
                                showStatus('<strong>清除成功！</strong><br>WiFi配置已删除。<br><small>⚠️ ' + (d.message || '请重启设备以应用更改') + '</small>', 'success');
                                document.getElementById('ssid').value = '';
                                document.getElementById('password').value = '';
                                document.getElementById('current').innerHTML = '<strong>当前状态:</strong> 未配置（等待配网）';
                            } else {
                                showStatus('<strong>清除失败:</strong> ' + (d.message || '未知错误'), 'error');
                            }
                        } catch(e) {
                            showStatus('响应解析失败: ' + e.message, 'error');
                        }
                    } else {
                        showStatus('清除失败，HTTP错误: ' + xhr.status, 'error');
                    }
                }
            };
            xhr.open('DELETE', '/api/wifi/config', true);
            xhr.send();
        }
        function resetDefault() {
            if (!confirm('确定要恢复默认配置吗？\n\n将恢复为出厂默认WiFi：\n- SSID: XINGHUI\n- 密码: 88888888\n\n注意：需要重启设备才能应用新的WiFi设置。')) return;
            showStatus('正在恢复默认配置...', 'info');
            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function() {
                if (xhr.readyState === 4) {
                    if (xhr.status === 200) {
                        try {
                            var d = JSON.parse(xhr.responseText);
                            if (d.status === 'success') {
                                showStatus('<strong>恢复默认配置成功！</strong><br>已恢复出厂默认WiFi<br>SSID: XINGHUI<br>密码: 88888888<br><small>⚠️ ' + (d.message || '请重启设备以应用新的WiFi设置') + '</small>', 'success');
                                document.getElementById('ssid').value = 'XINGHUI';
                                document.getElementById('password').value = '88888888';
                                document.getElementById('current').innerHTML = '<strong>当前配置:</strong> XINGHUI (默认，待重启生效)';
                            } else {
                                showStatus('<strong>恢复失败:</strong> ' + (d.message || '未知错误'), 'error');
                            }
                        } catch(e) {
                            showStatus('响应解析失败: ' + e.message, 'error');
                        }
                    } else {
                        showStatus('恢复失败，HTTP错误: ' + xhr.status, 'error');
                    }
                }
            };
            xhr.open('POST', '/api/wifi/config', true);
            xhr.setRequestHeader('Content-Type', 'application/json');
            xhr.send(JSON.stringify({ssid: 'XINGHUI', password: '88888888', is_default: true}));
        }
        loadCurrent();
    </script>
</body>
</html>)rawliteral";

/**
 * @brief 状态机版本配置页面HTML内容
 * @details 内联单页面，支持V2/V3版本选择
 */
static const char* sm_config_page_html = R"rawliteral(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>状态机版本配置</title>
    <style>
        body { font-family: Arial; max-width: 400px; margin: 20px auto; padding: 10px; background: #f5f5f5; }
        h1 { color: #333; text-align: center; margin-bottom: 10px; font-size: 24px; }
        .subtitle { text-align: center; color: #666; font-size: 14px; margin-bottom: 20px; }
        .nav { text-align: center; margin-bottom: 15px; font-size: 13px; }
        .nav a { color: #2196F3; text-decoration: none; padding: 0 8px; }
        .nav a:hover { text-decoration: underline; }
        .nav a.active { color: #666; }
        .box { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; color: #555; }
        select { width: 100%; padding: 12px; border: 1px solid #ddd; border-radius: 4px; font-size: 16px; background: white; cursor: pointer; }
        select:focus { border-color: #4CAF50; outline: none; }
        .version-desc { font-size: 13px; color: #666; margin-top: 8px; padding: 10px; background: #f9f9f9; border-radius: 4px; border-left: 3px solid #2196F3; }
        button { width: 100%; padding: 14px; margin-top: 10px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; background: #4CAF50; color: white; }
        button:hover { background: #45a049; }
        button:disabled { background: #ccc; cursor: not-allowed; }
        .status { margin-top: 15px; padding: 12px; border-radius: 4px; font-size: 14px; display: none; }
        .success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .info { background: #d1ecf1; color: #0c5460; border: 1px solid #bee5eb; }
        .reboot-notice { background: #fff3cd; color: #856404; border: 1px solid #ffeaa7; margin-top: 10px; }
        .current { background: #e7f3ff; padding: 12px; margin-bottom: 15px; border-radius: 4px; font-size: 14px; border-left: 4px solid #2196F3; }
    </style>
</head>
<body>
    <h1>状态机版本配置</h1>
    <div class="subtitle">系统配置</div>
    <div class="nav">
        <a href="/">WiFi配置</a> |
        <a href="/sm-config" class="active">状态机配置</a>
    </div>
    <div class="box">
        <div class="current" id="current">正在加载当前版本...</div>
        <div class="form-group">
            <label>版本选择</label>
            <select id="version" onchange="updateDescription()">
                <option value="2">V2 - 经典状态机</option>
                <option value="3">V3 - 条件触发状态机</option>
            </select>
            <div class="version-desc" id="versionDesc">支持条件触发、定时器、GPIO 输入等高级功能</div>
        </div>
        <button onclick="saveVersion()" id="saveBtn">保存版本</button>
        <div id="status" class="status"></div>
    </div>
    <script>
        const versionInfo = {
            2: "经典状态机 - 简单状态转换，适合基础 STEM 项目",
            3: "条件触发状态机 - 支持逻辑条件、定时器、GPIO 输入等高级功能"
        };
        function loadCurrent() {
            var xhr = new XMLHttpRequest();
            xhr.timeout = 5000;
            xhr.onreadystatechange = function() {
                if (xhr.readyState === 4 && xhr.status === 200) {
                    try {
                        var d = JSON.parse(xhr.responseText);
                        document.getElementById('version').value = d.version;
                        document.getElementById('current').innerHTML = '<strong>当前版本:</strong> ' + d.version_name;
                        updateDescription();
                    } catch(e) { showStatus('数据解析错误', 'error'); }
                }
            };
            xhr.open('GET', '/api/sm/version', true);
            xhr.send();
        }
        function updateDescription() {
            var v = document.getElementById('version').value;
            document.getElementById('versionDesc').textContent = versionInfo[v];
        }
        function saveVersion() {
            var version = document.getElementById('version').value;
            var btn = document.getElementById('saveBtn');
            btn.disabled = true;
            btn.textContent = '保存中...';
            var xhr = new XMLHttpRequest();
            xhr.timeout = 5000;
            xhr.onreadystatechange = function() {
                if (xhr.readyState === 4) {
                    btn.disabled = false;
                    btn.textContent = '保存版本';
                    if (xhr.status === 200) {
                        try {
                            var d = JSON.parse(xhr.responseText);
                            if (d.status === 'success') {
                                showStatus(d.message + '<br><strong>请重启设备以应用新配置</strong>', 'success reboot-notice');
                            } else {
                                showStatus('保存失败: ' + (d.message || '未知错误'), 'error');
                            }
                        } catch(e) { showStatus('响应解析错误', 'error'); }
                    } else {
                        showStatus('请求失败: HTTP ' + xhr.status, 'error');
                    }
                }
            };
            xhr.open('POST', '/api/sm/version', true);
            xhr.setRequestHeader('Content-Type', 'application/json');
            xhr.send(JSON.stringify({version: parseInt(version)}));
        }
        function showStatus(msg, type) {
            var el = document.getElementById('status');
            el.className = 'status ' + type;
            el.innerHTML = msg;
            el.style.display = 'block';
        }
        loadCurrent();
    </script>
</body>
</html>)rawliteral";

/**
 * @brief 状态机版本配置处理程序
 * @param req HTTP请求结构体
 * @return esp_err_t ESP_OK成功
 * @details 支持 GET/POST/OPTIONS 请求：
 *          GET: 返回当前状态机版本和可用版本列表
 *          POST: 设置新版本并保存到NVS（重启后生效）
 *          OPTIONS: CORS预检响应
 */
esp_err_t sm_version_handler(httpd_req_t *req)
{
    // 处理OPTIONS请求（CORS预检）
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_set_status(req, "200 OK");
        httpd_resp_set_type(req, "text/plain");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }

    // GET请求 - 返回当前版本和可用版本
    if (req->method == HTTP_GET) {
        int8_t current_version = sm_version_get();

        cJSON *root = cJSON_CreateObject();
        if (root == NULL) {
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        cJSON_AddStringToObject(root, "status", "success");
        cJSON_AddNumberToObject(root, "version", current_version);
        cJSON_AddStringToObject(root, "version_name", current_version == 2 ? "V2" : "V3");

        // 添加可用版本列表
        cJSON *available = cJSON_CreateArray();
        cJSON_AddItemToArray(available, cJSON_CreateNumber(2));
        cJSON_AddItemToArray(available, cJSON_CreateNumber(3));
        cJSON_AddItemToObject(root, "available", available);

        char *json_str = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);

        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, json_str, strlen(json_str));
        free(json_str);

        return ESP_OK;
    }

    // POST请求 - 设置新版本
    if (req->method == HTTP_POST) {
        char buf[64];
        int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
        if (ret <= 0) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty request body");
            return ESP_FAIL;
        }
        buf[ret] = '\0';

        cJSON *json = cJSON_Parse(buf);
        if (json == NULL) {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
            return ESP_FAIL;
        }

        cJSON *version_item = cJSON_GetObjectItem(json, "version");
        if (!cJSON_IsNumber(version_item)) {
            cJSON_Delete(json);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing or invalid 'version' field");
            return ESP_FAIL;
        }

        int8_t new_version = (int8_t)version_item->valueint;
        if (new_version != 2 && new_version != 3) {
            cJSON_Delete(json);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Version must be 2 or 3");
            return ESP_FAIL;
        }

        // 保存到 NVS
        esp_err_t save_err = sm_version_set(new_version);
        if (save_err != ESP_OK) {
            cJSON_Delete(json);
            char msg[128];
            snprintf(msg, sizeof(msg), "{\"status\":\"error\",\"message\":\"Failed to save version: %s\"}",
                     esp_err_to_name(save_err));
            httpd_resp_set_status(req, "500 Internal Server Error");
            httpd_resp_set_type(req, "application/json");
            httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
            httpd_resp_send(req, msg, strlen(msg));
            return ESP_FAIL;
        }

        cJSON *response = cJSON_CreateObject();
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddNumberToObject(response, "version", new_version);
        cJSON_AddStringToObject(response, "message", "版本已保存，重启后生效");
        cJSON_AddBoolToObject(response, "reboot_required", true);

        char *resp_str = cJSON_PrintUnformatted(response);
        cJSON_Delete(response);
        cJSON_Delete(json);

        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_send(req, resp_str, strlen(resp_str));
        free(resp_str);

        ESP_LOGI(TAG, "状态机版本已设置为 %d，重启后生效", new_version);
        return ESP_OK;
    }

    return ESP_ERR_INVALID_STATE;
}

/**
 * @brief 状态机版本配置页面处理程序
 * @param req HTTP请求结构体
 * @return esp_err_t ESP_OK成功
 * @details 返回内联的HTML配置页面，支持GET请求
 */
esp_err_t favicon_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        // 直接返回 204 No Content，避免浏览器为 favicon 请求产生 404 日志
        httpd_resp_set_status(req, "204 No Content");
        return httpd_resp_send(req, NULL, 0);
    }
    return ESP_ERR_INVALID_STATE;
}

static esp_err_t sm_config_page_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        httpd_resp_set_type(req, "text/html; charset=utf-8");
        httpd_resp_send(req, sm_config_page_html, strlen(sm_config_page_html));
        return ESP_OK;
    }
    return ESP_ERR_INVALID_STATE;
}

/**
 * @brief WiFi配置页面处理程序
 * @param req HTTP请求结构体
 * @return esp_err_t ESP_OK成功
 * @details 返回内联的HTML配置页面，支持GET请求
 */
static esp_err_t wifi_config_page_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        httpd_resp_set_type(req, "text/html; charset=utf-8");
        httpd_resp_send(req, wifi_config_page_html, strlen(wifi_config_page_html));
        return ESP_OK;
    }
    return ESP_ERR_INVALID_STATE;
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
