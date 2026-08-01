/*
 * 状态机 V3.0 - HTTP 处理器
 */

#include "sm_core.h"
#include "sm_json.h"
#include "sm_http.h"
#include <string.h>
#include "esp_log.h"
#include "esp_http_server.h"

static const char *TAG = "SM_V3_HTTP";

/* 辅助函数: 设置 CORS 头 */
static void set_cors_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_set_hdr(req, "Access-Control-Max-Age", "86400");
}

/* 辅助函数: 发送 JSON 响应 */
static esp_err_t send_json_response(httpd_req_t *req, const char *json_str)
{
    httpd_resp_set_type(req, "application/json");
    set_cors_headers(req);
    httpd_resp_send(req, json_str, strlen(json_str));
    return ESP_OK;
}

/* 辅助函数: 发送错误响应 */
static esp_err_t send_error_response(httpd_req_t *req, const char *message)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"success\":false,\"message\":\"%s\"}", message);
    httpd_resp_set_status(req, "400 Bad Request");
    send_json_response(req, buf);
    return ESP_OK;
}

/* GET 处理器 - 获取当前状态和配置 */
esp_err_t sm_v3_http_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET 请求 /api/v3/state-machine");

    char json_buf[4096];
    esp_err_t err = sm_v3_get_config_json(json_buf, sizeof(json_buf));

    if (err != ESP_OK) {
        send_error_response(req, "获取配置失败");
        return err;
    }

    // 获取最后一次加载状态
    const char* load_status = sm_v3_get_last_load_status();
    bool load_success = sm_v3_get_last_load_success();

    // 获取当前状态
    char current_state[SM_MAX_STATE_NAME_LEN];
    sm_v3_get_current_state(current_state, sizeof(current_state));

    // 构建增强的响应
    char response[8192];
    snprintf(response, sizeof(response),
             "{\"success\":true,\"config\":%s,\"currentState\":\"%s\","
             "\"lastLoadSuccess\":%s,\"lastLoadStatus\":\"%s\",\"version\":\"3.0\"}",
             json_buf, current_state, load_success ? "true" : "false", load_status);

    return send_json_response(req, response);
}

/* POST 处理器 - 加载状态机配置
 *
 * 处理流程:
 * 1. 分块读取 HTTP 请求体（支持大配置）
 * 2. 停止当前运行中的状态机（如果正在运行）
 * 3. 解析并加载 JSON 配置
 * 4. 启动新配置的状态机
 * 5. 返回当前状态
 *
 * 安全性: 原子操作保证不会出现"半加载"状态
 */
esp_err_t sm_v3_http_post_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "POST 请求 /api/v3/state-machine");

    /* 【缓冲区设置】
     * buf[4096]: 静态缓冲区，限制最大配置 4KB（足够所有场景）
     * received: 已接收字节计数器
     * remaining: 剩余待接收字节（req->content_len = HTTP Content-Length）
     */
    char buf[4096];
    int received = 0;
    int remaining = req->content_len;

    /* 【大小检查】预留1字节给字符串终止符 '\0' */
    if (remaining > sizeof(buf) - 1) {
        send_error_response(req, "请求过大");
        return ESP_ERR_INVALID_SIZE;
    }

    /* 【分块接收循环】
     *
     * 为什么需要循环?
     *   TCP 数据可能分多次到达，httpd_req_recv 每次返回实际接收量
     *   例如: 4KB 配置可能分 3 次到达 (1500 + 1500 + 1020)
     *
     * buf + received:
     *   指针运算，指向缓冲区中下一个写入位置
     *   第一次: buf + 0 = &buf[0]
     *   第二次: buf + 1500 = &buf[1500]
     *   第三次: buf + 3000 = &buf[3000]
     */
    while (remaining > 0) {
        int ret = httpd_req_recv(req, buf + received, remaining);

        /* 错误处理: ret <= 0 表示连接断开或超时 */
        if (ret <= 0) {
            send_error_response(req, "接收请求失败");
            return ESP_ERR_INVALID_STATE;
        }

        received += ret;   /* 累加: "已写入到哪里" */
        remaining -= ret;   /* 递减: "还剩多少要收" */
    }

    /* 【字符串终止】C 字符串需要 '\0' 结尾，用于后续 JSON 解析 */
    buf[received] = '\0';

    /* 【原子更新流程】
     * 1. 停止当前状态机（如果运行中）- 防止状态混乱
     * 2. 加载新配置 - 失败则保持停止状态
     * 3. 启动新状态机 - 从 initialState 开始
     *
     * 失败处理:
     *   - 加载失败: 状态机保持停止，返回 400
     *   - 启动失败: 配置已加载但状态机停止，返回 500
     */
    sm_v3_stop();

    /* 加载配置 */
    esp_err_t err = sm_v3_load_config(buf);
    if (err != ESP_OK) {
        send_error_response(req, "配置 JSON 无效");
        return err;
    }

    /* 启动状态机 */
    err = sm_v3_start();
    if (err != ESP_OK) {
        send_error_response(req, "启动状态机失败");
        return err;
    }

    /* 成功响应 */
    char response[512];
    char current_state[SM_MAX_STATE_NAME_LEN];
    sm_v3_get_current_state(current_state, sizeof(current_state));

    snprintf(response, sizeof(response),
             "{\"success\":true,\"message\":\"配置加载成功\","
             "\"currentState\":\"%s\",\"version\":\"3.0\"}",
             current_state);

    return send_json_response(req, response);
}

/* DELETE 处理器 - 停止和重置 */
esp_err_t sm_v3_http_delete_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "DELETE 请求 /api/v3/state-machine");

    sm_v3_stop();
    sm_v3_reset();

    const char *response = "{\"success\":true,\"message\":\"状态机已停止\"}";
    return send_json_response(req, response);
}

/* OPTIONS 处理器 - CORS 预检请求 */
esp_err_t sm_v3_http_options_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "OPTIONS 请求 /api/v3/state-machine");

    set_cors_headers(req);
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

/* 注册 V3 端点到 HTTP 服务器 */
esp_err_t sm_v3_register_endpoints(httpd_handle_t server)
{
    httpd_uri_t get_uri = {
        .uri = "/api/v3/state-machine",
        .method = HTTP_GET,
        .handler = sm_v3_http_get_handler,
        .user_ctx = NULL
    };

    httpd_uri_t post_uri = {
        .uri = "/api/v3/state-machine",
        .method = HTTP_POST,
        .handler = sm_v3_http_post_handler,
        .user_ctx = NULL
    };

    httpd_uri_t delete_uri = {
        .uri = "/api/v3/state-machine",
        .method = HTTP_DELETE,
        .handler = sm_v3_http_delete_handler,
        .user_ctx = NULL
    };

    httpd_uri_t options_uri = {
        .uri = "/api/v3/state-machine",
        .method = HTTP_OPTIONS,
        .handler = sm_v3_http_options_handler,
        .user_ctx = NULL
    };

    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &get_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &post_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &delete_uri));
    ESP_ERROR_CHECK(httpd_register_uri_handler(server, &options_uri));

    return ESP_OK;
}
