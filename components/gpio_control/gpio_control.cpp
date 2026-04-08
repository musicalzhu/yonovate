#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "driver/gpio.h"

#include "gpio_manager.h"
#include "gpio_control.h"
#include "cJSON.h"

static const char *TAG = "gpio_control";

// GPIO控制处理函数
esp_err_t gpio_control_handler(httpd_req_t *req)
{
    // 添加CORS头部，允许跨源请求
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    // 如果是OPTIONS请求（预检请求），直接返回
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    // GET请求处理现有功能
    if (req->method == HTTP_GET) {
        int pin = -1;
        int state = -1;
        int group = -1;
        int index = -1;
        int mode = -1; // -1表示不是设置模式的请求
        char device[10] = {0}; // 用于区分设备 "esp" 或 "xl" 或 "group"
        
        // 获取URL参数
        size_t query_len = httpd_req_get_url_query_len(req);
        if (query_len > 0) {
            char* buf_ptr = (char*) malloc(query_len + 1);
            if(!buf_ptr){
                httpd_resp_send_500(req);
                return ESP_FAIL;
            }
            
            if (httpd_req_get_url_query_str(req, buf_ptr, query_len + 1) == ESP_OK) {
                char param[32];
                /* 获取pin参数 */
                if (httpd_query_key_value(buf_ptr, "pin", param, sizeof(param)) == ESP_OK) {
                    pin = atoi(param);
                }
                
                /* 获取state参数 */
                if (httpd_query_key_value(buf_ptr, "state", param, sizeof(param)) == ESP_OK) {
                    state = atoi(param);
                }
                
                /* 获取device参数 */
                if (httpd_query_key_value(buf_ptr, "device", param, sizeof(param)) == ESP_OK) {
                    strncpy(device, param, sizeof(device) - 1);
                }
                
                /* 获取group参数 */
                if (httpd_query_key_value(buf_ptr, "group", param, sizeof(param)) == ESP_OK) {
                    group = atoi(param);
                }
                
                /* 获取index参数 */
                if (httpd_query_key_value(buf_ptr, "index", param, sizeof(param)) == ESP_OK) {
                    index = atoi(param);
                }
                
                /* 获取mode参数 */
                if (httpd_query_key_value(buf_ptr, "mode", param, sizeof(param)) == ESP_OK) {
                    mode = atoi(param);
                }
                
                /* 获取test参数 */
                if (httpd_query_key_value(buf_ptr, "test", param, sizeof(param)) == ESP_OK) {
                    // 处理测试连接请求
                    if (strcmp(param, "connection") == 0) {
                        ESP_LOGI(TAG, "收到连接测试请求");
                        const char resp[] = "{\"status\":\"success\",\"message\":\"Connected successfully\"}";
                        httpd_resp_set_type(req, "application/json");
                        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
                        free(buf_ptr);
                        return ESP_OK;
                    }
                }
            }
            free(buf_ptr);
        }
        
        // 根据请求类型记录不同的日志信息
        if (mode != -1) {
            // 设置GPIO组模式的请求
            ESP_LOGI(TAG, "GPIO组模式设置请求: group=%d, mode=%s", group, mode == 0 ? "output" : "input");
        } else if (pin >= 0) {
            // 控制单个GPIO引脚的请求
            ESP_LOGI(TAG, "GPIO引脚控制请求: device=%s, pin=%d, state=%d", device, pin, state);
        } else if (group >= 0 && index >= 0) {
            // 控制GPIO组中特定引脚的请求
            ESP_LOGI(TAG, "GPIO组引脚控制请求: group=%d, index=%d, state=%d", group, index, state);
        } else {
            // 其他类型的请求
            ESP_LOGI(TAG, "GPIO控制请求: device=%s, pin=%d, state=%d, group=%d, index=%d, mode=%d", 
                     device, pin, state, group, index, mode);
        }
        
        // 处理设置GPIO组模式的请求
        if (mode != -1) {
            if (group < 0) {
                ESP_LOGW(TAG, "设置GPIO组模式时缺少group参数");
                httpd_resp_send_500(req);
                return ESP_FAIL;
            }
            
            esp_err_t ret = gpio_manager_set_group_mode(group, mode);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "设置GPIO组%d模式失败: %s", group, esp_err_to_name(ret));
                httpd_resp_send_500(req);
                return ESP_FAIL;
            }
            
            const char resp[] = "{\"status\":\"success\"}";
            httpd_resp_set_type(req, "application/json");
            httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
            return ESP_OK;
        }
        
        // 处理GPIO控制请求
        if (pin >= 0) {
            // 控制单个GPIO引脚
            if (strcmp(device, "esp") == 0) {
                // ESP32原生GPIO
                gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT);
                esp_err_t ret = gpio_set_level((gpio_num_t)pin, state);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "设置ESP32 GPIO%d电平失败: %s", pin, esp_err_to_name(ret));
                    httpd_resp_send_500(req);
                    return ESP_FAIL;
                }
            } else {
                ESP_LOGW(TAG, "未知的设备类型: %s", device);
                httpd_resp_send_500(req);
                return ESP_FAIL;
            }
        } else if (group >= 0 && index >= 0) {
            // 验证 state 参数
            if (state != 0 && state != 1) {
                ESP_LOGW(TAG, "无效的state参数: %d", state);
                const char resp[] = "Error: Invalid state parameter. Valid values are 0 or 1.";
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, resp);
                return ESP_FAIL;
            }

            // 通过gpio_manager控制GPIO组中的特定引脚
            esp_err_t ret = gpio_manager_set_level(group, index, state);
            if (ret != ESP_OK) {
                if (ret == ESP_ERR_INVALID_ARG) {
                    ESP_LOGE(TAG, "设置GPIO组%d索引%d电平失败: 引脚配置为输入模式，无法设置电平", group, index);
                    const char resp[] = "Error: Cannot set level for input pin. Please set pin to output mode first.";
                    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, resp);
                } else {
                    ESP_LOGE(TAG, "设置GPIO组%d索引%d电平失败: %s", group, index, esp_err_to_name(ret));
                    httpd_resp_send_500(req);
                }
                return ESP_FAIL;
            }
        } else {
            ESP_LOGW(TAG, "缺少必要的参数");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        const char resp[] = "{\"status\":\"success\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // POST请求处理同步配置
    if (req->method == HTTP_POST) {
        ESP_LOGI(TAG, "处理POST请求，同步GPIO配置");
        
        // 解析JSON请求
        char* content = (char*)calloc(1, req->content_len + 1);
        if (!content) {
            ESP_LOGE(TAG, "内存分配失败");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        int ret = httpd_req_recv(req, content, req->content_len);
        if (ret <= 0) {
            free(content);
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            } else {
                httpd_resp_send_500(req);
            }
            return ESP_FAIL;
        }
        
        cJSON *json_root = cJSON_Parse(content);
        free(content);
        
        if (!json_root) {
            ESP_LOGE(TAG, "JSON解析失败");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        // 解析并设置GPIO组模式
        cJSON *group_modes = cJSON_GetObjectItem(json_root, "groupModes");
        if (group_modes && cJSON_IsObject(group_modes)) {
            cJSON *mode_item = NULL;
            cJSON_ArrayForEach(mode_item, group_modes) {
                int group_id = atoi(mode_item->string);
                int mode = mode_item->valueint;
                
                ESP_LOGI(TAG, "设置GPIO组%d模式为%s", group_id, mode == 0 ? "输出" : "输入");
                esp_err_t ret = gpio_manager_set_group_mode(group_id, mode);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "设置GPIO组%d模式失败: %s", group_id, esp_err_to_name(ret));
                    cJSON_Delete(json_root);
                    httpd_resp_send_500(req);
                    return ESP_FAIL;
                }
            }
        }
        
        // 解析并设置GPIO输出状态
        cJSON *output_states = cJSON_GetObjectItem(json_root, "outputStates");
        if (output_states && cJSON_IsObject(output_states)) {
            cJSON *group_item = NULL;
            cJSON_ArrayForEach(group_item, output_states) {
                int group_id = atoi(group_item->string);
                
                if (cJSON_IsObject(group_item)) {
                    cJSON *pin_item = NULL;
                    cJSON_ArrayForEach(pin_item, group_item) {
                        int pin_index = atoi(pin_item->string);
                        int state = pin_item->valueint;
                        
                        ESP_LOGI(TAG, "设置GPIO组%d引脚%d状态为%d", group_id, pin_index, state);
                        esp_err_t ret = gpio_manager_set_level(group_id, pin_index, state);
                        if (ret != ESP_OK && ret != ESP_ERR_INVALID_ARG) {
                            ESP_LOGE(TAG, "设置GPIO组%d引脚%d状态失败: %s", group_id, pin_index, esp_err_to_name(ret));
                            cJSON_Delete(json_root);
                            httpd_resp_send_500(req);
                            return ESP_FAIL;
                        }
                    }
                }
            }
        }
        
        cJSON_Delete(json_root);
        
        // 返回成功响应
        const char resp[] = "{\"status\":\"success\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // 不支持的方法
    ESP_LOGW(TAG, "不支持的HTTP方法: %d", req->method);
    httpd_resp_set_status(req, "405 Method Not Allowed");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

// GPIO状态获取处理函数
esp_err_t gpio_state_handler(httpd_req_t *req)
{
    // 添加CORS头部，允许跨源请求
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    // 如果是OPTIONS请求（预检请求），直接返回
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    int pin = -1;
    int group = -1;
    int index = -1;
    char device[10] = {0}; // 用于区分设备 "esp" 或 "xl" 或 "group"
    
    // 如果是OPTIONS请求（预检请求），直接返回
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    // 获取URL参数
    size_t query_len = httpd_req_get_url_query_len(req);
    if (query_len > 0) {
        char* buf_ptr = (char*) malloc(query_len + 1);
        if(!buf_ptr){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        if (httpd_req_get_url_query_str(req, buf_ptr, query_len + 1) == ESP_OK) {
            char param[32];
            /* 获取pin参数 */
            if (httpd_query_key_value(buf_ptr, "pin", param, sizeof(param)) == ESP_OK) {
                pin = atoi(param);
            }
            
            /* 获取device参数 */
            if (httpd_query_key_value(buf_ptr, "device", param, sizeof(param)) == ESP_OK) {
                strncpy(device, param, sizeof(device) - 1);
            }
            
            /* 获取group参数 */
            if (httpd_query_key_value(buf_ptr, "group", param, sizeof(param)) == ESP_OK) {
                group = atoi(param);
            }
            
            /* 获取index参数 */
            if (httpd_query_key_value(buf_ptr, "index", param, sizeof(param)) == ESP_OK) {
                index = atoi(param);
            }
        }
        free(buf_ptr);
    }
    
    ESP_LOGI(TAG, "GPIO状态请求: device=%s, pin=%d, group=%d, index=%d", device, pin, group, index);
    
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        ESP_LOGE(TAG, "创建JSON对象失败");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    if (pin >= 0) {
        if (strcmp(device, "esp") == 0) {
            // ESP32原生GPIO
            int level = gpio_get_level((gpio_num_t)pin);
            cJSON_AddNumberToObject(root, "level", level);
        } else {
            ESP_LOGW(TAG, "未知的设备类型: %s", device);
            cJSON_Delete(root);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
    } else if (group >= 0 && index >= 0) {
        int level = 0;
        esp_err_t ret = gpio_manager_get_level(group, index, &level);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "获取GPIO组%d索引%d电平失败: %s", group, index, esp_err_to_name(ret));
            cJSON_Delete(root);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        cJSON_AddNumberToObject(root, "level", level);
    } else {
        // 获取所有GPIO组的状态
        cJSON *groups_array = cJSON_CreateArray();
        if (!groups_array) {
            ESP_LOGE(TAG, "创建JSON数组失败");
            cJSON_Delete(root);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        // 添加每个GPIO组的信息
        for (int i = 0; i < 8; i++) {
            cJSON *group_obj = cJSON_CreateObject();
            if (!group_obj) {
                ESP_LOGE(TAG, "创建组对象失败");
                cJSON_Delete(groups_array);
                cJSON_Delete(root);
                httpd_resp_send_500(req);
                return ESP_FAIL;
            }
            
            cJSON_AddNumberToObject(group_obj, "id", i);
            
            // 添加组内每个引脚的状态
            cJSON *pins_array = cJSON_CreateArray();
            if (!pins_array) {
                ESP_LOGE(TAG, "创建引脚数组失败");
                cJSON_Delete(group_obj);
                cJSON_Delete(groups_array);
                cJSON_Delete(root);
                httpd_resp_send_500(req);
                return ESP_FAIL;
            }
            
            for (int j = 0; j < 4; j++) {
                cJSON *pin_obj = cJSON_CreateObject();
                if (!pin_obj) {
                    ESP_LOGE(TAG, "创建引脚对象失败");
                    cJSON_Delete(pins_array);
                    cJSON_Delete(group_obj);
                    cJSON_Delete(groups_array);
                    cJSON_Delete(root);
                    httpd_resp_send_500(req);
                    return ESP_FAIL;
                }
                
                int level = 0;
                esp_err_t ret = gpio_manager_get_level(i, j, &level);
                if (ret != ESP_OK) {
                    ESP_LOGW(TAG, "获取GPIO组%d引脚%d状态失败: %s", i, j, esp_err_to_name(ret));
                    level = -1; // 表示获取失败
                }
                
                cJSON_AddNumberToObject(pin_obj, "index", j);
                cJSON_AddNumberToObject(pin_obj, "level", level);
                cJSON_AddItemToArray(pins_array, pin_obj);
            }
            
            cJSON_AddItemToObject(group_obj, "pins", pins_array);
            cJSON_AddItemToArray(groups_array, group_obj);
        }
        
        cJSON_AddItemToObject(root, "groups", groups_array);
    }
    
    char *json_response = cJSON_Print(root);
    if (json_response) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
        free(json_response);
    } else {
        httpd_resp_send_500(req);
    }
    
    cJSON_Delete(root);
    return ESP_OK;
}