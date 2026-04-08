#include "esp_err.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include "string.h"
#include "gpio_manager.h"
#include "motor_control.h"

// 定义MIN宏
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static const char *TAG = "motor_control";

// 电机控制处理函数
esp_err_t motor_control_handler(httpd_req_t *req)
{
    // 设置CORS头部
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    
    // 处理OPTIONS请求
    if (req->method == HTTP_OPTIONS) {
        httpd_resp_set_status(req, "204 No Content");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    // 只处理POST请求
    if (req->method != HTTP_POST) {
        httpd_resp_set_status(req, "405 Method Not Allowed");
        httpd_resp_send(req, NULL, 0);
        return ESP_OK;
    }
    
    // 读取请求体
    char content[100] = {0};
    size_t recv_size = MIN(req->content_len, sizeof(content) - 1);
    
    int ret = httpd_req_recv(req, content, recv_size);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    
    // 解析JSON数据
    cJSON *root = cJSON_Parse(content);
    if (!root) {
        ESP_LOGE(TAG, "无法解析JSON数据");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    // 获取电机ID和状态
    cJSON *motor_id_json = cJSON_GetObjectItem(root, "motorId");
    cJSON *state_json = cJSON_GetObjectItem(root, "state");
    
    if (!motor_id_json || !state_json || !cJSON_IsNumber(motor_id_json) || !cJSON_IsNumber(state_json)) {
        ESP_LOGE(TAG, "JSON数据格式错误");
        httpd_resp_send_500(req);
        cJSON_Delete(root);
        return ESP_FAIL;
    }
    
    int motor_id = motor_id_json->valueint;
    int state = state_json->valueint;
    
    ESP_LOGI(TAG, "接收到电机控制请求: motorId=%d, state=%d", motor_id, state);
    
    // 控制电机
    esp_err_t err = gpio_manager_set_motor(motor_id, state);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "电机控制失败: %s", esp_err_to_name(err));
        httpd_resp_send_500(req);
        cJSON_Delete(root);
        return ESP_FAIL;
    }
    
    // 返回成功响应
    const char resp[] = "{\"status\":\"success\"}";
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    
    cJSON_Delete(root);
    return ESP_OK;
}