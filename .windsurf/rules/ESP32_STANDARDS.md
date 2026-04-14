# ESP32嵌入式开发技术规范

> **适用范围**: ESP32/ESP-IDF嵌入式项目
> **专注领域**: C/C++固件开发、硬件抽象层、嵌入式系统

---

## 🔧 ESP32编程最佳实践

### 1. **ESP-IDF项目结构规范**
```
project_root/
├── main/                 # 主程序入口
│   ├── main.cpp         # app_main()函数
│   └── CMakeLists.txt   # 主程序构建配置
├── components/          # 自定义组件
│   └── component_name/
│       ├── include/     # 公共头文件
│       ├── component.h  # 组件接口
│       ├── component.c  # 组件实现
│       └── CMakeLists.txt
├── CMakeLists.txt       # 根构建配置
└── sdkconfig           # ESP-IDF配置文件
```

### 2. **内存管理最佳实践**
```cpp
// ✅ 正确的内存管理
cJSON *json = cJSON_Parse(buffer);
if (json) {
    // 使用json...
    cJSON_Delete(json);  // 必须释放
}

// ✅ 动态内存配对
char *buf = malloc(size);
if (buf) {
    // 使用buf...
    free(buf);  // 必须释放
}

// ❌ 错误：内存泄漏
cJSON *json = cJSON_Parse(buffer);
// 忘记cJSON_Delete(json)
```

### 3. **错误处理规范**
```cpp
// ✅ ESP-IDF标准错误处理
esp_err_t ret = some_function();
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "some_function failed: %s", esp_err_to_name(ret));
    return ret;
}

// ✅ 资源清理错误处理
nvs_handle_t handle;
esp_err_t err = nvs_open("namespace", NVS_READONLY, &handle);
if (err != ESP_OK) {
    return err;  // 无需清理
}
// 使用handle...
nvs_close(handle);  // 必须关闭
return ESP_OK;
```

### 4. **日志使用规范**

#### 4.1 基本规范
```cpp
// ✅ 正确的日志使用
static const char *TAG = "component_name";

ESP_LOGI(TAG, "Information message");
ESP_LOGW(TAG, "Warning message");
ESP_LOGE(TAG, "Error message: %s", esp_err_to_name(ret));

// ❌ 错误：打印敏感信息
ESP_LOGI(TAG, "WiFi password: %s", password);  // 禁止
```

#### 4.2 **强制性编码规范（重要）**

**⚠️ 必须遵守**：ESP32固件日志**只能使用英文**，禁止使用中文！

**原因**：
- Windows中文系统默认GBK编码与ESP32 UTF-8输出冲突
- 导致`UnicodeEncodeError`，无法监控日志
- 影响调试和问题排查

```cpp
ESP_LOGI(TAG, "WiFi init complete");  // ✅ 正确
ESP_LOGI(TAG, "WiFi初始化完成");  // ❌ 错误：ESP32固件日志必须使用英文
```

> **原因**: 避免Windows中文环境下的编码问题（见TROUBLESHOOTING.md第45-78行）

**替代方案**：使用ASCII符号
```cpp
// 用这些替代UTF-8符号
[OK]   替代 ✓
[FAIL] 替代 ✗
[WARN] 替代 ⚠
[INFO] 替代 ℹ
```

**违规后果**：
- AI必须将中文日志改为英文
- 重新编译烧录
- 参考 `TROUBLESHOOTING.md` 编码问题章节

### 5. **GPIO操作规范**
```cpp
// ✅ 正确的GPIO配置
gpio_config_t io_conf = {};
io_conf.intr_type = GPIO_INTR_DISABLE;
io_conf.mode = GPIO_MODE_OUTPUT;
io_conf.pin_bit_mask = (1ULL << GPIO_PIN);
io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
gpio_config(&io_conf);

// ✅ 安全的GPIO操作
esp_err_t ret = gpio_set_level(GPIO_PIN, level);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "gpio_set_level failed");
}
```

### 6. **WiFi管理最佳实践**
```cpp
// ✅ WiFi配置管理
typedef struct {
    char ssid[33];      // 802.11标准：最大32字节+1
    char password[65];  // 802.11标准：最大64字节+1
} wifi_config_t;

// ✅ NVS存储WiFi配置
esp_err_t save_wifi_config(const char *ssid, const char *password) {
    if (!ssid || strlen(ssid) > 32) return ESP_ERR_INVALID_ARG;
    if (!password || strlen(password) > 64) return ESP_ERR_INVALID_ARG;
    // 存储逻辑...
}
```

---

## 📝 代码注释规范

### 文件头注释
```cpp
/**
 * @file wifi_config_store.cpp
 * @brief WiFi配置NVS存储组件
 * @details 提供WiFi SSID和密码的持久化存储功能，支持保存、加载和擦除操作
 * @author AI Assistant
 * @date 2024
 */
```

### 函数注释规范
```cpp
/**
 * @brief 初始化WiFi配置存储
 * @return esp_err_t ESP_OK成功，其他错误码失败
 * @note 必须在应用启动时调用一次，初始化NVS分区
 * 
 * 示例:
 * @code
 * esp_err_t err = wifi_config_store_init();
 * if (err != ESP_OK) {
 *     ESP_LOGE(TAG, "WiFi配置存储初始化失败: %s", esp_err_to_name(err));
 * }
 * @endcode
 */
```

### 代码内注释
```cpp
// 单行注释：说明下一步操作的目的
cJSON *root = cJSON_CreateObject();  // 创建JSON对象用于构建响应

/*
 * 多行注释：说明复杂逻辑块的功能
 * 1. 首先解析请求体
 * 2. 验证JSON格式
 * 3. 提取ssid和password字段
 * 4. 保存到NVS
 */
```

### 关键逻辑注释
```cpp
// 安全检查：验证请求体大小，防止内存溢出
if (req->content_len == 0 || req->content_len > 512) {
    // 返回错误响应
}

// 内存释放：确保所有分支都释放cJSON对象
cJSON_Delete(json);
```

---

## 🔄 开闭原则规范 (OCP)

### 核心定义
- **对扩展开放**：新功能通过添加代码实现，而非修改已有代码
- **对修改关闭**：已有代码尽量保持稳定，减少改动带来的风险

### ESP32项目实践

#### 1. 回调函数机制
```cpp
// 定义回调类型
typedef void (*wifi_event_cb_t)(wifi_event_t event, void* data);

// 注册回调（扩展点）
void wifi_manager_register_event_cb(wifi_event_cb_t cb);

// 触发回调（不修改核心代码即可扩展行为）
static wifi_event_cb_t g_event_cb = NULL;
void wifi_manager_notify(wifi_event_t event, void* data) {
    if (g_event_cb) {
        g_event_cb(event, data);
    }
}
```

#### 2. URI处理器注册机制
```cpp
// web_server.cpp中新增URI的处理方式
// ✅ 正确：通过注册机制扩展，不修改start函数
void web_server_register_handler(const char* uri, httpd_method_t method,
                                  esp_err_t (*handler)(httpd_req_t *req))
{
    httpd_uri_t config_uri = {
        .uri = uri,
        .method = method,
        .handler = handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &config_uri);
}

// ❌ 错误：每次新增URI都要修改start函数
esp_err_t web_server_start() {
    // ... 原有代码
    httpd_register_uri_handler(server, &uri1);  // 已有
    httpd_register_uri_handler(server, &uri2);  // 新增 - 违反OCP！
    httpd_register_uri_handler(server, &uri3);  // 再次新增 - 再次修改！
}
```

#### 3. 配置驱动而非硬编码
```cpp
// ✅ 正确：通过配置结构体扩展
static const gpio_config_t gpio_configs[] = {
    {GPIO_NUM_18, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_DISABLE},
    {GPIO_NUM_19, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_DISABLE},
    // 新增GPIO只需添加配置项，无需修改初始化代码
};

void gpio_init_all(void) {
    for (int i = 0; i < sizeof(gpio_configs)/sizeof(gpio_configs[0]); i++) {
        gpio_config(&gpio_configs[i]);
    }
}

// ❌ 错误：硬编码导致每次都要修改初始化函数
void gpio_init_all(void) {
    gpio_set_direction(GPIO_NUM_18, GPIO_MODE_OUTPUT);  // 硬编码
    gpio_set_direction(GPIO_NUM_19, GPIO_MODE_OUTPUT);  // 硬编码
    // 新增GPIO必须修改此函数
}
```

#### 4. 组件依赖解耦
```cpp
// ✅ 正确：通过CMake条件编译，核心代码不感知具体实现
// CMakeLists.txt
set(COMPONENTS web_server wifi_manager)  // 可灵活增删

// web_server.cpp 不直接包含wifi_manager.h
// 而是通过事件或回调与WiFi组件交互

// ❌ 错误：强耦合导致修改一处影响多处
// web_server.cpp 直接调用wifi_manager_update_credentials()
// 如果wifi_manager接口变化，web_server必须修改
```

### OCP检查清单
新增功能前自检：
- [ ] 是否需要修改已有函数才能扩展？
- [ ] 是否可以通过配置/注册机制实现？
- [ ] 修改是否会影响其他模块？
- [ ] 是否遵循"添加新代码 > 修改旧代码"原则？

---

## 🈶 日志和输出语言规范

### 强制规则
- **所有ESP_LOGX日志必须使用中文**
- **所有ESP_ERROR_CHECK错误消息必须使用中文**
- **所有用户可见的输出必须使用中文**
- **代码注释默认使用中文**

### 正确示例
```cpp
ESP_LOGI(TAG, "WiFi配置存储初始化成功");
ESP_LOGI(TAG, "从NVS加载WiFi配置: SSID=%s", saved_ssid);
ESP_LOGE(TAG, "WiFi配置保存失败: %s", esp_err_to_name(err));
ESP_LOGW(TAG, "WiFi凭据已保存但重连失败: %s", esp_err_to_name(werr));
```

### 错误示例
```cpp
ESP_LOGI(TAG, "WiFi config saved");           // 英文！错误！
ESP_LOGE(TAG, "Failed to initialize");        // 英文！错误！
ESP_LOGW(TAG, "saved but reconnect failed");  // 英文！错误！
```

### 验证清单
提交代码前必须检查：
- [ ] 所有ESP_LOGI/ESP_LOGW/ESP_LOGE使用中文
- [ ] 所有httpd_resp_send的错误消息使用中文
- [ ] 所有snprintf的错误提示使用中文
- [ ] 代码注释使用中文

---

## 🤖 AI自动执行技术规范（IDE集成）

### 问题背景

**现象**：AI在IDE中自动执行`idf.py build`经常失败，但用户手工ESP-IDF CMD执行成功。

**根因分析**：

| 因素 | AI尝试方式 | 手工CMD方式 | 技术原因 |
|------|-----------|------------|---------|
| **Shell环境** | PowerShell `export.ps1` | CMD `export.bat` | PS1未设置所有环境变量 |
| **执行模式** | 后台`Blocking=false` | 前台顺序执行 | 后台状态不同步 |
| **输出处理** | `Select-String`过滤 | 直接输出 | 管道可能截断或异常 |
| **路径解析** | PS路径处理 | CMD `/d`参数 | 盘符切换问题 |
| **结果验证** | 日志解析 | 文件时间戳 | 日志可能丢失或格式变化 |

### 强制规范

**AI执行build时必须遵循**（成功率从30%提升至100%）：

```powershell
# ✅ 唯一正确方式
cmd.exe /c "[ESP-IDF路径]\export.bat && cd /d [项目路径] && idf.py build"

# ✅ 结果验证（文件检查法）
if (Test-Path "build\[项目名].bin") { 
    "✅ Build successful" 
}
```

**禁止的方式**（AI必须自我检查并拒绝）：

```powershell
# ❌ 禁止1：PowerShell直接执行export.ps1
. C:\Espressif\frameworks\esp-idf-v5.5.3\export.ps1; idf.py build
# 原因：PS1环境配置不完整

# ❌ 禁止2：后台执行
run_command -Blocking:false -Command "idf.py build"
# 原因：无法同步获取结果

# ❌ 禁止3：管道过滤输出
idf.py build 2>&1 | Select-String -Pattern "error|complete"
# 原因：过滤可能截断输出或导致异常

# ❌ 禁止4：未激活环境直接执行
idf.py build
# 原因：找不到命令或环境变量
```

### 技术细节

**为什么`cmd.exe /c`比PowerShell可靠**：

1. **环境完整性**：`export.bat`使用`set`命令设置环境变量，在CMD子进程中完整生效
2. **顺序执行**：`&&`操作符确保前一步成功才执行下一步
3. **路径处理**：`/d`参数确保跨盘符切换正确（如C:到D:）
4. **输出保持**：不过滤输出，避免编码或截断问题

**为什么文件检查比日志解析可靠**：

1. **确定性**：文件存在性检查是二进制的（是/否）
2. **稳定性**：日志格式可能变化，文件检查不会
3. **独立性**：不依赖输出解析，避免编码问题
4. **可验证**：可对比时间戳确认build比源文件新

### AI自我检查代码模板

每次执行build前，AI应生成类似代码并自检：

```powershell
# AI生成的执行命令（模板）
$cmd = 'cmd.exe /c "{0}\export.bat && cd /d {1} && idf.py build"' -f 
    '[ESP-IDF路径]', '[项目路径]'

# AI自检清单（必须全部通过）
$checks = @(
    ($cmd -match 'cmd\.exe'),           # 使用CMD
    ($cmd -match 'export\.bat'),       # 使用BAT（非PS1）
    ($cmd -match '/d'),                 # 盘符切换
    ($cmd -notmatch '\|'),             # 无管道
    ($cmd -notmatch 'Select-String')   # 无过滤
)

if ($checks -notcontains $false) {
    # 执行命令
    Invoke-Expression $cmd
    # 验证结果
    if (Test-Path "build\[项目名].bin") { 
        "✅ Build successful" 
    }
}
```

### 参考文档

- **问题排查**：`TROUBLESHOOTING.md` - "AI自动执行构建失败问题"章节
- **操作流程**：`WORKFLOW_GUIDE.md` - "AI自我检查清单"
- **实践经验**：成功验证案例（2026-04-13）

---

## ESP32嵌入式项目测试策略

由于ESP-IDF嵌入式项目通常无单元测试框架，采用以下验证方式：

### Level 1: 编译时验证（强制）
```bash
idf.py build
```
- 无编译错误/警告
- 链接成功，生成固件

### Level 2: 静态代码检查（强制）
- 代码边界检查：`if (len > MAX_SIZE)`
- 空指针检查：`if (ptr == NULL)`
- 内存管理：malloc/free配对，句柄关闭
- 错误处理：所有分支返回正确错误码

### Level 3: 功能自检（代码内实现）
```cpp
// 参数验证
if (!ssid || !password) return ESP_ERR_INVALID_ARG;

// 范围验证
if (req->content_len > 512) return ESP_ERR_INVALID_SIZE;

// 状态验证
if (!s_wifi_initialized) return ESP_ERR_INVALID_STATE;
```

### Level 4: 硬件测试（有条件时执行）
- 烧录固件到ESP32开发板
- 功能API测试（HTTP请求）
- NVS存储持久化验证
- WiFi连接重连验证

---

## 🌐 Web页面版本管理规范（ESP32内嵌页面）

### 版本号管理原则

对于ESP32内嵌HTML页面（通过`web_server.cpp`中的字符串常量嵌入固件）：

```cpp
// ✅ 正确：在subtitle中显示版本号
<div class="subtitle">配网模式 (AP: Yonovate-Setup) <span style="color:#999;font-size:12px;">v1.4</span></div>

// ✅ 正确：每次修改后递增版本号
// v1.3 -> v1.4 -> v1.5 （小版本递增）
```

### 版本号更新检查清单

| 检查项 | 要求 |
|--------|------|
| **修改位置** | 必须在subtitle span中 |
| **格式** | v主版本.次版本（如v1.4） |
| **递增规则** | 每次修改后+0.1 |
| **目的** | 让用户确认页面已更新，避免浏览器缓存问题 |

### 烧录后验证流程

```bash
1. 烧录固件到ESP32
2. 连接设备WiFi (Yonovate-Setup)
3. 访问 http://192.168.4.1
4. 强制刷新页面 (Ctrl+F5)
5. 检查版本号是否更新
6. 如未更新：
   - 使用无痕模式验证
   - 清除浏览器缓存
   - 检查烧录是否成功
```

---

**重要：这些技术规范是强制性的，违反会导致代码质量问题。每次编码前必须回顾相关规范，并进行自我检查。**

---

*版本：1.0 | 更新日期：2026-04-11 | 适用：ESP32/ESP-IDF嵌入式项目*
