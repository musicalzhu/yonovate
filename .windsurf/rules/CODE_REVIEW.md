# ESP-IDF C/C++ 代码审查最佳实践

> **适用范围**: ESP32/ESP-IDF嵌入式C/C++项目
> **用途**: AI代码审查检查清单、静态代码分析指南
> **性质**: 建议性规范，用于提高代码质量

---

## 🔍 代码审查检查清单

### 1. 内存安全审查

#### 1.1 动态内存管理
```cpp
// ✅ 正确：malloc/free 配对
void* ptr = malloc(size);
if (ptr == NULL) {
    ESP_LOGE(TAG, "Memory allocation failed");
    return ESP_ERR_NO_MEM;
}
// ... 使用ptr ...
free(ptr);  // 必须释放

// ❌ 错误：内存泄漏
void* ptr = malloc(size);
if (condition) {
    return;  // 错误：ptr未释放
}
free(ptr);
```

**审查检查点**:
- [ ] 所有malloc/calloc有对应的free
- [ ] 所有new有对应的delete
- [ ] 错误返回路径是否释放内存
- [ ] 循环中分配的内存在循环外释放

#### 1.2 ESP-IDF特定资源管理
```cpp
// ✅ 正确：句柄配对管理
nvs_handle_t handle;
esp_err_t err = nvs_open("namespace", NVS_READONLY, &handle);
if (err != ESP_OK) {
    return err;
}
// ... 使用handle ...
nvs_close(handle);  // 必须关闭

// ✅ 正确：cJSON内存管理
cJSON *root = cJSON_CreateObject();
if (root == NULL) {
    return ESP_ERR_NO_MEM;
}
// ... 添加数据 ...
cJSON_Delete(root);  // 必须删除
```

**审查检查点**:
- [ ] nvs_open → nvs_close 配对
- [ ] cJSON_Create → cJSON_Delete 配对
- [ ] httpd_start → httpd_stop 配对
- [ ] esp_wifi_start → esp_wifi_stop 配对
- [ ] 文件打开 → 文件关闭 配对

### 2. 并发安全审查

#### 2.1 FreeRTOS任务安全
```cpp
// ✅ 正确：使用互斥锁保护共享资源
static SemaphoreHandle_t s_mutex;
static int s_shared_counter = 0;

void init() {
    s_mutex = xSemaphoreCreateMutex();
}

void update_counter() {
    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_shared_counter++;
        xSemaphoreGive(s_mutex);
    }
}
```

**审查检查点**:
- [ ] 全局变量访问是否有互斥保护
- [ ] 共享资源是否有竞态条件
- [ ] 中断服务程序(ISR)是否使用ISR安全函数
- [ ] 死锁风险检查（锁的获取顺序）

#### 2.2 事件组与信号量
```cpp
// ✅ 正确：事件组使用
EventGroupHandle_t s_wifi_event_group;

void init() {
    s_wifi_event_group = xEventGroupCreate();
}

void wait_for_wifi() {
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        CONNECTED_BIT | FAIL_BIT,
        pdTRUE,  // 清除位
        pdFALSE, // 等待任意位
        portMAX_DELAY
    );
}
```

### 3. 错误处理审查

#### 3.1 ESP错误码完整处理
```cpp
// ✅ 正确：所有错误路径都处理
esp_err_t init_component() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || 
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // 特定错误处理
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);  // 其他错误直接检查
    
    ret = wifi_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
        cleanup_nvs();  // 错误时清理已初始化资源
        return ret;
    }
    
    return ESP_OK;
}
```

**审查检查点**:
- [ ] 所有esp_err_t返回值都检查
- [ ] 错误路径是否有资源清理
- [ ] ESP_ERROR_CHECK是否用于致命错误
- [ ] 错误日志是否包含有意义的信息

### 4. 边界条件审查

#### 4.1 输入参数验证
```cpp
// ✅ 正确：严格参数验证
esp_err_t save_wifi_config(const char *ssid, const char *password) {
    // 空指针检查
    if (ssid == NULL || password == NULL) {
        ESP_LOGE(TAG, "SSID or password is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    // 长度边界检查
    size_t ssid_len = strlen(ssid);
    if (ssid_len == 0 || ssid_len > 32) {
        ESP_LOGE(TAG, "SSID length invalid: %d (must be 1-32)", ssid_len);
        return ESP_ERR_INVALID_ARG;
    }
    
    size_t pass_len = strlen(password);
    if (pass_len < 8 || pass_len > 64) {
        ESP_LOGE(TAG, "Password length invalid: %d (must be 8-64)", pass_len);
        return ESP_ERR_INVALID_ARG;
    }
    
    // ... 保存逻辑 ...
    return ESP_OK;
}
```

**审查检查点**:
- [ ] 指针参数NULL检查
- [ ] 字符串长度边界检查
- [ ] 数组索引边界检查
- [ ] 数值范围验证

### 5. 资源限制审查

#### 5.1 栈大小估算
```cpp
// ✅ 正确：合理的栈大小配置
#define TASK_STACK_SIZE (4096)  // 根据任务复杂度估算

void create_tasks() {
    xTaskCreatePinnedToCore(
        http_server_task,
        "http_server",
        TASK_STACK_SIZE,  // 足够但不过度
        NULL,
        5,
        NULL,
        0
    );
}
```

**审查检查点**:
- [ ] 任务栈大小是否合理（不过大导致内存浪费）
- [ ] 递归调用深度限制
- [ ] 大数组避免在栈上分配
- [ ] 堆内存分配失败处理

### 6. 安全性审查

#### 6.1 敏感信息保护
```cpp
// ❌ 错误：打印敏感信息
ESP_LOGI(TAG, "WiFi password: %s", password);

// ✅ 正确：不记录敏感信息
ESP_LOGI(TAG, "WiFi config saved for SSID: %s", ssid);
// 密码不打印
```

**审查检查点**:
- [ ] 密码/密钥是否出现在日志中
- [ ] 个人信息是否泄露
- [ ] 调试代码是否移除（如硬编码密码）

#### 6.2 缓冲区溢出防护
```cpp
// ✅ 正确：安全的字符串操作
char buffer[64];
snprintf(buffer, sizeof(buffer), "%s", input);  // 带长度限制

// ❌ 错误：可能溢出
strcpy(buffer, input);  // 如果input > 64，溢出！
```

### 7. 可维护性审查

#### 7.1 魔术数字消除
```cpp
// ❌ 错误：魔术数字
if (len > 32) {  // 32是什么？
    return ERROR;
}

// ✅ 正确：使用有意义的常量
#define MAX_SSID_LEN 32
#define MIN_WIFI_PASSWORD_LEN 8
#define MAX_WIFI_PASSWORD_LEN 64

if (len > MAX_SSID_LEN) {
    ESP_LOGE(TAG, "SSID exceeds max length: %d", MAX_SSID_LEN);
    return ESP_ERR_INVALID_ARG;
}
```

**审查检查点**:
- [ ] 无魔术数字
- [ ] 常量命名有意义
- [ ] 配置参数集中管理

#### 7.2 函数单一职责
```cpp
// ❌ 错误：函数做太多事
void init_and_connect_wifi(const char* ssid, const char* pass) {
    // 初始化WiFi
    // 配置STA
    // 尝试连接
    // 启动HTTP服务器
    // 太多了！
}

// ✅ 正确：拆分职责
esp_err_t wifi_manager_init(void);
esp_err_t wifi_manager_configure_sta(const char* ssid, const char* pass);
esp_err_t wifi_manager_connect(void);
esp_err_t web_server_start(void);
```

---

## 📋 代码审查流程

### AI代码审查步骤

```markdown
1. **读取修改的文件**
   ↓
2. **应用检查清单**（逐项检查）
   - 内存安全检查
   - 并发安全检查
   - 错误处理检查
   - 边界条件检查
   - 安全性检查
   ↓
3. **标记问题级别**
   - 🔴 严重：内存泄漏、缓冲区溢出、空指针解引用
   - 🟠 警告：魔术数字、缺少错误处理
   - 🟡 建议：代码风格、注释完善
   ↓
4. **生成审查报告**
   ↓
5. **修复后复查**
```

---

## 🔧 静态分析工具集成

### 推荐工具

| 工具 | 用途 | 集成方式 |
|------|------|---------|
| cppcheck | 静态代码分析 | `idf.py cppcheck` |
| clang-tidy | C++代码检查 | 自定义target |
| valgrind | 内存泄漏检测 | 模拟器运行 |

### ESP-IDF特定检查

```bash
# 检查内存使用
idf.py size

# 检查栈使用
idf.py size-files

# 运行静态分析
idf.py cppcheck
```

---

**重要：此代码审查清单用于AI辅助代码审查，提高代码质量。严重问题必须修复，建议性问题酌情处理。**

---

*版本：1.0 | 创建日期：2026-04-14 | 适用：ESP32/ESP-IDF C/C++项目*
