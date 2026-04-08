#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"
#include "gpio_manager.h"
#include "logic_circuit.h"

static const char *TAG = "logic_circuit";

// 任务句柄
static TaskHandle_t s_logic_circuit_task_handle = NULL;

// 逻辑电路状态
static int state_values[129] = {0}; // s1-s128的状态值

// 函数声明
static int parse_state_value(const char *state_str);
static esp_err_t set_state_value(const char *state_str, int value);
static int execute_logic_operation(const char *type, int input1, int input2);
static esp_err_t parse_json_request(httpd_req_t *req, cJSON **json_root);
static void update_logic_circuit_states(void);
static void logic_circuit_monitor_task(void* arg);

// 逻辑电路配置结构
typedef struct {
    int id;
    char state[32];
} io_group_t;

typedef struct {
    char id[32];
    char type[10];
    char inputs[2][32];
    char output[32];
} logic_gate_t;

// 逻辑电路配置
static io_group_t input_groups[8];
static logic_gate_t logic_gates[50];
static io_group_t output_groups[8];
static int input_group_count = 0;
static int logic_gate_count = 0;
static int output_group_count = 0;

// 解析状态值
static int parse_state_value(const char *state_str)
{
    if (!state_str || strlen(state_str) == 0) {
        ESP_LOGE(TAG, "状态值字符串为空");
        return 0;
    }
    
    if (strlen(state_str) < 2 || state_str[0] != 's') {
        ESP_LOGE(TAG, "无效的状态值字符串格式: %s", state_str ? state_str : "NULL");
        return 0;
    }
    
    int index = atoi(state_str + 1);
    if (index >= 1 && index <= 128) {
        int value = state_values[index];
        // 将解析状态值日志改为DEBUG级别
        ESP_LOGD(TAG, "解析状态值: %s -> 索引 %d -> 值 %d", state_str, index, value);
        return value;
    }
    
    ESP_LOGE(TAG, "状态值索引超出范围: %s (索引 %d)", state_str, index);
    return 0;
}

// 设置状态值
static esp_err_t set_state_value(const char *state_str, int value)
{
    if (!state_str || strlen(state_str) == 0) {
        // 空状态值，不进行设置
        return ESP_OK;
    }
    
    if (strlen(state_str) < 2 || state_str[0] != 's') {
        // 将无效状态值警告改为详细级别
        ESP_LOGD(TAG, "无效的状态值字符串: %s", state_str ? state_str : "NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    int index = atoi(state_str + 1);
    if (index >= 1 && index <= 128) {
        state_values[index] = value;
        // 将状态值设置日志改为详细级别
        ESP_LOGD(TAG, "设置状态值 %s (索引 %d) = %d", state_str, index, value);
        return ESP_OK;
    }
    
    // 将索引超出范围警告改为详细级别
    ESP_LOGD(TAG, "状态值索引超出范围: %s (索引 %d)", state_str, index);
    return ESP_ERR_INVALID_ARG;
}

// 执行逻辑运算
static int execute_logic_operation(const char *type, int input1, int input2)
{
    int result = 0;
    if (!type) {
        ESP_LOGE(TAG, "逻辑运算类型为空");
        return 0;
    }
    
    // 将逻辑运算日志改为DEBUG级别
    ESP_LOGD(TAG, "执行逻辑运算: 类型=%s, 输入1=%d, 输入2=%d", type, input1, input2);
    
    if (strcmp(type, "AND") == 0) {
        result = input1 & input2;
        // 将AND运算结果日志改为DEBUG级别
        ESP_LOGD(TAG, "AND运算结果: %d AND %d = %d", input1, input2, result);
    } else if (strcmp(type, "OR") == 0) {
        result = input1 | input2;
        // 将OR运算结果日志改为DEBUG级别
        ESP_LOGD(TAG, "OR运算结果: %d OR %d = %d", input1, input2, result);
    } else if (strcmp(type, "NOT") == 0) {
        result = !input1;
        // 将NOT运算结果日志改为DEBUG级别
        ESP_LOGD(TAG, "NOT运算结果: NOT %d = %d", input1, result);
    } else {
        ESP_LOGE(TAG, "未知的逻辑运算类型: %s", type);
        result = 0;
    }
    
    return result;
}

// 更新逻辑电路状态
static void update_logic_circuit_states(void)
{
    // 将状态更新日志改为DEBUG级别
    ESP_LOGD(TAG, "更新逻辑电路状态: 输入组=%d, 逻辑门=%d, 输出组=%d", 
             input_group_count, logic_gate_count, output_group_count);
    
    // 如果没有任何配置，直接返回
    if (input_group_count == 0 && logic_gate_count == 0 && output_group_count == 0) {
        ESP_LOGD(TAG, "没有配置任何逻辑电路组件，跳过状态更新");
        return;
    }
    
    // 首先从输入组获取状态值
    ESP_LOGD(TAG, "处理%d个输入组", input_group_count);
    for (int i = 0; i < input_group_count; i++) {
        if (strlen(input_groups[i].state) > 0) {
            // 直接获取第一个引脚的状态，避免使用结构体变量
            int group_state = 0;
            esp_err_t ret = gpio_manager_get_level(input_groups[i].id, 0, &group_state);
            if (ret == ESP_OK) {
                // 将输入组状态日志改为DEBUG级别
                ESP_LOGD(TAG, "输入组%d状态 (第一个引脚): %d", input_groups[i].id, group_state);
                set_state_value(input_groups[i].state, group_state);
            } else {
                ESP_LOGE(TAG, "无法获取输入组%d信息: %s", input_groups[i].id, esp_err_to_name(ret));
            }
        } else {
            ESP_LOGD(TAG, "输入组%d没有关联状态值", input_groups[i].id);
        }
    }
    
    // 执行逻辑门运算
    ESP_LOGD(TAG, "开始处理%d个逻辑门", logic_gate_count);
    for (int i = 0; i < logic_gate_count; i++) {
        // 将逻辑门处理日志改为DEBUG级别
        ESP_LOGD(TAG, "处理逻辑门 %d: ID=%s, 类型=%s", i, logic_gates[i].id, logic_gates[i].type);
        
        // 获取输入1的值
        ESP_LOGD(TAG, "  正在读取输入1 (%s)", logic_gates[i].inputs[0]);
        int input1 = parse_state_value(logic_gates[i].inputs[0]);
        int input2 = 0;
        
        // 对于非门，只需要一个输入，其他类型读取第二个输入
        if (strcmp(logic_gates[i].type, "NOT") != 0) {
            ESP_LOGD(TAG, "  正在读取输入2 (%s)", logic_gates[i].inputs[1]);
            input2 = parse_state_value(logic_gates[i].inputs[1]);
        }
        
        // 执行逻辑运算
        int result = execute_logic_operation(logic_gates[i].type, input1, input2);
        
        // 设置输出
        if (strlen(logic_gates[i].output) > 0) {
            ESP_LOGD(TAG, "  设置输出 (%s) = %d", logic_gates[i].output, result);
            esp_err_t ret = set_state_value(logic_gates[i].output, result);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "  设置逻辑门%s输出失败: %s", logic_gates[i].id, esp_err_to_name(ret));
            }
        } else {
            ESP_LOGW(TAG, "  逻辑门%s没有指定输出", logic_gates[i].id);
        }
    }
    
    // 更新输出组 (支持GPIO和电机)
    ESP_LOGD(TAG, "处理%d个输出组", output_group_count);
    for (int i = 0; i < output_group_count; i++) {
        if (strlen(output_groups[i].state) > 0) {
            int state = parse_state_value(output_groups[i].state);
            
            // 检查是否是电机输出 (ID 12-13 对应 M1-M2)
            if (output_groups[i].id >= 12 && output_groups[i].id <= 13) {
                int motor_id = output_groups[i].id - 12;  // 转换为电机ID (0-1)
                // 逻辑电路中简单映射: 0=停止, 1=正转(全速，即9对应90%)
                int8_t speed_direction = (state == 0) ? 0 : 9;
                ESP_LOGD(TAG, "设置电机%d状态为%d (速度方向: %d)", motor_id, state, speed_direction);
                esp_err_t ret = gpio_manager_set_motor(motor_id, speed_direction);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "设置电机%d状态失败: %s", motor_id, esp_err_to_name(ret));
                }
            } else {
                // GPIO输出 (只设置第一个引脚)
                ESP_LOGD(TAG, "设置GPIO组%d第一个引脚状态为%d", output_groups[i].id, state);
                esp_err_t ret = gpio_manager_set_level(output_groups[i].id, 0, state);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "设置输出组%d状态失败: %s", output_groups[i].id, esp_err_to_name(ret));
                }
            }
        } else {
            ESP_LOGD(TAG, "输出组%d没有关联状态值", output_groups[i].id);
        }
    }
    
    ESP_LOGD(TAG, "逻辑电路状态更新完成");
}

// 解析JSON请求体
static esp_err_t parse_json_request(httpd_req_t *req, cJSON **json_root)
{
    if (req->content_len == 0) {
        ESP_LOGW(TAG, "请求体为空");
        return ESP_ERR_INVALID_ARG;
    }
    
    char *buf = (char*)calloc(1, req->content_len + 1);
    if (!buf) {
        ESP_LOGE(TAG, "分配JSON缓冲区失败");
        return ESP_ERR_NO_MEM;
    }
    
    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) {
        free(buf);
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            ESP_LOGW(TAG, "接收请求超时");
            httpd_resp_send_408(req);
        } else {
            ESP_LOGE(TAG, "接收请求失败: %d", ret);
            httpd_resp_send_500(req);
        }
        return ESP_FAIL;
    }
    
    *json_root = cJSON_Parse(buf);
    free(buf);
    
    if (!(*json_root)) {
        ESP_LOGW(TAG, "解析JSON失败: %s", cJSON_GetErrorPtr());
        return ESP_ERR_INVALID_ARG;
    }
    
    return ESP_OK;
}

// 重置逻辑电路配置
void logic_circuit_reset_config(void)
{
    // 重置配置计数
    input_group_count = 0;
    logic_gate_count = 0;
    output_group_count = 0;
    
    // 重置配置数组
    memset(input_groups, 0, sizeof(input_groups));
    memset(logic_gates, 0, sizeof(logic_gates));
    memset(output_groups, 0, sizeof(output_groups));
    
    // 重置状态值
    memset(state_values, 0, sizeof(state_values));
    
    // 停止逻辑电路监控任务
    logic_circuit_monitor_stop();
    
    ESP_LOGI(TAG, "逻辑电路配置已重置");
}

// 逻辑电路监控任务函数
static void logic_circuit_monitor_task(void* arg)
{
    ESP_LOGI(TAG, "逻辑电路监控任务已启动");
    
    while (1) {
        // 更新逻辑电路状态
        update_logic_circuit_states();
        
        // 每100ms更新一次状态
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// 启动逻辑电路监控任务
esp_err_t logic_circuit_monitor_start(void)
{
    ESP_LOGI(TAG, "正在启动逻辑电路...");
    
    // 检查任务是否已经在运行
    if (s_logic_circuit_task_handle != NULL) {
        ESP_LOGW(TAG, "逻辑电路监控任务已经在运行");
        return ESP_OK;
    }
    
    // 初始化状态值数组
    memset(state_values, 0, sizeof(state_values));
    
    // 创建监控任务
    xTaskCreate(logic_circuit_monitor_task, "logic_circuit_monitor", 4096, NULL, 5, &s_logic_circuit_task_handle);
    if (s_logic_circuit_task_handle == NULL) {
        ESP_LOGE(TAG, "创建逻辑电路监控任务失败");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "逻辑电路已启动");
    return ESP_OK;
}

// 停止逻辑电路监控任务
void logic_circuit_monitor_stop(void)
{
    ESP_LOGI(TAG, "正在停止逻辑电路...");
    
    if (s_logic_circuit_task_handle != NULL) {
        vTaskDelete(s_logic_circuit_task_handle);
        s_logic_circuit_task_handle = NULL;
        
        // 清空所有状态值
        memset(state_values, 0, sizeof(state_values));
        
        ESP_LOGI(TAG, "逻辑电路已停止");
    } else {
        ESP_LOGW(TAG, "逻辑电路未在运行");
    }
}

// 逻辑电路配置处理函数
esp_err_t logic_circuit_handler(httpd_req_t *req)
{
    // 添加CORS头部，允许跨源请求
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    
    // 如果是OPTIONS请求（预检请求），直接返回
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    // 检查是否有test=connection参数
    size_t query_len = httpd_req_get_url_query_len(req);
    if (query_len > 0) {
        char* buf_ptr = (char*) malloc(query_len + 1);
        if(!buf_ptr){
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        
        if (httpd_req_get_url_query_str(req, buf_ptr, query_len + 1) == ESP_OK) {
            char param[32];
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
    
    // GET请求返回当前配置
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "处理GET请求，返回当前配置");
        
        cJSON *root = cJSON_CreateObject();
        cJSON *input_array = cJSON_CreateArray();
        cJSON *gate_array = cJSON_CreateArray();
        cJSON *output_array = cJSON_CreateArray();
        
        // 添加输入组
        for (int i = 0; i < input_group_count; i++) {
            cJSON *input_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(input_obj, "id", input_groups[i].id);
            cJSON_AddStringToObject(input_obj, "state", input_groups[i].state);
            cJSON_AddItemToArray(input_array, input_obj);
        }
        
        // 添加逻辑门
        for (int i = 0; i < logic_gate_count; i++) {
            cJSON *gate_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(gate_obj, "id", logic_gates[i].id);
            cJSON_AddStringToObject(gate_obj, "type", logic_gates[i].type);
            cJSON_AddStringToObject(gate_obj, "input1", logic_gates[i].inputs[0]);
            cJSON_AddStringToObject(gate_obj, "input2", logic_gates[i].inputs[1]);
            cJSON_AddStringToObject(gate_obj, "output", logic_gates[i].output);
            cJSON_AddItemToArray(gate_array, gate_obj);
        }
        
        // 添加输出组
        for (int i = 0; i < output_group_count; i++) {
            cJSON *output_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(output_obj, "id", output_groups[i].id);
            cJSON_AddStringToObject(output_obj, "state", output_groups[i].state);
            cJSON_AddItemToArray(output_array, output_obj);
        }
        
        cJSON_AddItemToObject(root, "inputGroups", input_array);
        cJSON_AddItemToObject(root, "logicGates", gate_array);
        cJSON_AddItemToObject(root, "outputGroups", output_array);
        
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
    
    // POST请求同步配置
    if (req->method == HTTP_POST) {
        ESP_LOGI(TAG, "处理POST请求，同步配置");
        
        cJSON *json_root = NULL;
        esp_err_t err = parse_json_request(req, &json_root);
        if (err != ESP_OK) {
            return err;
        }
        
        // 重置当前配置
        logic_circuit_reset_config();
        
        // 解析输入组
        cJSON *input_groups_json = cJSON_GetObjectItem(json_root, "inputGroups");
        if (input_groups_json && cJSON_IsArray(input_groups_json)) {
            input_group_count = cJSON_GetArraySize(input_groups_json);
            if (input_group_count > 8) input_group_count = 8;
            
            for (int i = 0; i < input_group_count; i++) {
                cJSON *input_obj = cJSON_GetArrayItem(input_groups_json, i);
                if (input_obj) {
                    cJSON *id_obj = cJSON_GetObjectItem(input_obj, "id");
                    cJSON *state_obj = cJSON_GetObjectItem(input_obj, "state");
                    
                    if (id_obj && cJSON_IsNumber(id_obj)) {
                        input_groups[i].id = id_obj->valueint;
                    }
                    
                    if (state_obj && cJSON_IsString(state_obj)) {
                        strncpy(input_groups[i].state, state_obj->valuestring, sizeof(input_groups[i].state) - 1);
                    }
                }
            }
        }
        
        // 解析逻辑门
        cJSON *logic_gates_json = cJSON_GetObjectItem(json_root, "logicGates");
        if (logic_gates_json && cJSON_IsArray(logic_gates_json)) {
            logic_gate_count = cJSON_GetArraySize(logic_gates_json);
            if (logic_gate_count > 50) logic_gate_count = 50;
            
            for (int i = 0; i < logic_gate_count; i++) {
                cJSON *gate_obj = cJSON_GetArrayItem(logic_gates_json, i);
                if (gate_obj) {
                    cJSON *id_obj = cJSON_GetObjectItem(gate_obj, "id");
                    cJSON *type_obj = cJSON_GetObjectItem(gate_obj, "type");
                    cJSON *inputs_array = cJSON_GetObjectItem(gate_obj, "inputs");
                    cJSON *output_obj = cJSON_GetObjectItem(gate_obj, "output");

                    // 从inputs数组获取input1和input2
                    cJSON *input1_obj = inputs_array ? cJSON_GetArrayItem(inputs_array, 0) : NULL;
                    cJSON *input2_obj = inputs_array ? cJSON_GetArrayItem(inputs_array, 1) : NULL;
                    
                    // 初始化逻辑门数据
                    memset(logic_gates[i].id, 0, sizeof(logic_gates[i].id));
                    memset(logic_gates[i].type, 0, sizeof(logic_gates[i].type));
                    memset(logic_gates[i].inputs[0], 0, sizeof(logic_gates[i].inputs[0]));
                    memset(logic_gates[i].inputs[1], 0, sizeof(logic_gates[i].inputs[1]));
                    memset(logic_gates[i].output, 0, sizeof(logic_gates[i].output));

                    if (id_obj && cJSON_IsString(id_obj)) {
                        strncpy(logic_gates[i].id, id_obj->valuestring, sizeof(logic_gates[i].id) - 1);
                    }
                    
                    if (type_obj && cJSON_IsString(type_obj)) {
                        strncpy(logic_gates[i].type, type_obj->valuestring, sizeof(logic_gates[i].type) - 1);
                    }
                    
                    if (input1_obj && cJSON_IsString(input1_obj)) {
                        strncpy(logic_gates[i].inputs[0], input1_obj->valuestring, sizeof(logic_gates[i].inputs[0]) - 1);
                    }
                    
                    if (input2_obj && cJSON_IsString(input2_obj)) {
                        strncpy(logic_gates[i].inputs[1], input2_obj->valuestring, sizeof(logic_gates[i].inputs[1]) - 1);
                    }
                    
                    if (output_obj && cJSON_IsString(output_obj)) {
                        strncpy(logic_gates[i].output, output_obj->valuestring, sizeof(logic_gates[i].output) - 1);
                    }

                    ESP_LOGI(TAG, "解析逻辑门 %d: ID=%s, 类型=%s", i, logic_gates[i].id, logic_gates[i].type);
                }
            }
        }
        
        // 解析输出组
        cJSON *output_groups_json = cJSON_GetObjectItem(json_root, "outputGroups");
        if (output_groups_json && cJSON_IsArray(output_groups_json)) {
            output_group_count = cJSON_GetArraySize(output_groups_json);
            if (output_group_count > 8) output_group_count = 8;
            
            for (int i = 0; i < output_group_count; i++) {
                cJSON *output_obj = cJSON_GetArrayItem(output_groups_json, i);
                if (output_obj) {
                    cJSON *id_obj = cJSON_GetObjectItem(output_obj, "id");
                    cJSON *state_obj = cJSON_GetObjectItem(output_obj, "state");
                    cJSON *mode_obj = cJSON_GetObjectItem(output_obj, "mode");
                    
                    if (id_obj && cJSON_IsNumber(id_obj)) {
                        output_groups[i].id = id_obj->valueint;
                    }
                    
                    if (state_obj && cJSON_IsString(state_obj)) {
                        strncpy(output_groups[i].state, state_obj->valuestring, sizeof(output_groups[i].state) - 1);
                    }
                    
                    // 从outputGroups中的mode属性设置GPIO组模式
                    if (id_obj && cJSON_IsNumber(id_obj) && mode_obj && cJSON_IsNumber(mode_obj)) {
                        int group_id = id_obj->valueint;
                        int mode = mode_obj->valueint;
                        
                        ESP_LOGI(TAG, "设置GPIO组%d模式为%s", group_id, mode == 0 ? "输出" : "输入");
                        esp_err_t ret = gpio_manager_set_group_mode(group_id, mode);
                        if (ret != ESP_OK) {
                            ESP_LOGE(TAG, "设置GPIO组%d模式失败: %s", group_id, esp_err_to_name(ret));
                        }
                    }
                }
            }
        }
        
        // 从inputGroups中的mode属性设置GPIO组模式
        for (int i = 0; i < input_group_count; i++) {
            cJSON *input_obj = cJSON_GetArrayItem(input_groups_json, i);
            if (input_obj) {
                cJSON *id_obj = cJSON_GetObjectItem(input_obj, "id");
                cJSON *mode_obj = cJSON_GetObjectItem(input_obj, "mode");
                
                if (id_obj && cJSON_IsNumber(id_obj) && mode_obj && cJSON_IsNumber(mode_obj)) {
                    int group_id = id_obj->valueint;
                    int mode = mode_obj->valueint;
                    
                    ESP_LOGI(TAG, "设置GPIO组%d模式为%s", group_id, mode == 0 ? "输出" : "输入");
                    esp_err_t ret = gpio_manager_set_group_mode(group_id, mode);
                    if (ret != ESP_OK) {
                        ESP_LOGE(TAG, "设置GPIO组%d模式失败: %s", group_id, esp_err_to_name(ret));
                    }
                }
            }
        }
        
        // 为了向后兼容，仍然支持单独的groupModes字段
        cJSON *group_modes_json = cJSON_GetObjectItem(json_root, "groupModes");
        if (group_modes_json && cJSON_IsObject(group_modes_json)) {
            cJSON *mode_item = NULL;
            cJSON_ArrayForEach(mode_item, group_modes_json) {
                if (cJSON_IsNumber(mode_item)) {
                    int group_id = atoi(mode_item->string);
                    int mode = mode_item->valueint;
                    
                    ESP_LOGI(TAG, "设置GPIO组%d模式为%s", group_id, mode == 0 ? "输出" : "输入");
                    esp_err_t ret = gpio_manager_set_group_mode(group_id, mode);
                    if (ret != ESP_OK) {
                        ESP_LOGE(TAG, "设置GPIO组%d模式失败: %s", group_id, esp_err_to_name(ret));
                    }
                }
            }
        }
        
        // groupModules 仅用于前端显示，后端不处理
        // 物理到逻辑的转换在前端 JavaScript 中完成
        
        cJSON_Delete(json_root);
        
        // 更新逻辑电路状态
        update_logic_circuit_states();
        
        // 启动逻辑电路监控任务（如果尚未运行）
        logic_circuit_monitor_start();
        
        // 返回成功响应
        const char resp[] = "{\"status\":\"success\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // PUT请求更新状态
    if (req->method == HTTP_PUT) {
        ESP_LOGI(TAG, "处理PUT请求，更新逻辑电路状态");
        update_logic_circuit_states();
        const char resp[] = "{\"status\":\"success\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
        return ESP_OK;
    }
    
    // DELETE请求重置配置
    if (req->method == HTTP_DELETE) {
        ESP_LOGI(TAG, "处理DELETE请求，重置配置");
        logic_circuit_reset_config();
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

// 更新逻辑电路状态（对外接口）
void logic_circuit_update_states(void)
{
    ::update_logic_circuit_states(); // 使用作用域解析运算符避免递归
}