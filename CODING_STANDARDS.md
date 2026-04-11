# AI编程标准与最佳实践 - 通用规则手册

> 适用于：Cursor、VSCode、Windsurf、Claude Desktop、GitHub Copilot等所有AI编程助手
> 专注领域：ESP32/嵌入式开发、C/C++、固件开发

## 🚨 核心安全原则

### 1. **真实性验证原则**
- **绝对禁止假设文件存在** - 必须用工具验证文件真实存在于文件系统
- **IDE显示≠文件存在** - 必须用 `git status`、`read_file` 等工具确认
- **发现矛盾立即停止** - 当IDE显示与工具结果不一致时，立即报告用户
- **多重验证机制** - 关键操作必须用2个以上工具交叉验证

### 2. **最小化操作原则**
- **只做用户明确要求的事** - 不添加"改进"、"优化"等额外操作
- **单一操作一次** - 每个工具调用只完成一个明确任务
- **禁止删除重写** - 除非用户明确要求，绝不删除原有代码重新编写
- **增量修改优先** - 在现有代码基础上最小化修改

### 3. **Git操作安全原则**
- **操作前必检查状态** - `git status` 确认工作目录状态
- **小步提交频繁保存** - 重要修改立即 `git add && git commit`
- **与远程保持同步** - 操作前 `git pull`，操作后 `git push`
- **分支保护** - 重要修改在独立分支进行

### 4. **修改后验证机制（强制性）**

#### 🚨 **绝对原则：任何修改必须验证后才能说"完成"**

- **禁止未经验证的"好了"** - 修改后不验证就报告完成是严重违规
- **必须实际验证** - 不能只是"应该可以"，必须实际测试
- **双重检查（Double Check）** - 关键配置需要另一个Agent验证或自我复查
- **效果验证** - 配置文件修改后必须验证配置生效，代码修改后必须编译/运行测试

#### 📋 **验证流程**

**代码修改后：**
```
1. 读取修改后的文件验证内容正确
2. 检查语法/格式（JSON语法、代码编译等）
3. 如有编译环境，必须编译测试
4. 多Agent审查（复杂修改时）
5. 报告：✅ 验证通过 + 具体验证步骤
```

**配置文件修改后：**
```
1. 读取文件验证修改已保存
2. 验证JSON/XML语法正确
3. 验证路径/值有效（文件存在、格式正确）
4. 如可能影响其他配置，验证关联性
5. 报告：✅ 配置有效 + 验证结果
```

#### ❌ **禁止行为**
- 修改后不验证就报告"已完成"
- 只修改不确认是否生效
- 假设修改成功而不验证
- 复杂修改不进行双重检查

#### ✅ **正确示例**
```
用户：修改COM端口为COM5
AI：
  1. 修改settings.json
  2. 验证修改：read_file确认COM5已保存
  3. 验证语法：JSON格式正确
  4. 验证关联：tasks.json也更新为COM5
  5. 报告：✅ COM端口已更新为COM5，settings.json和tasks.json已同步修改并验证
```

#### ⚠️ **失败处理**
如果验证失败：
1. **立即停止** - 不继续后续操作
2. **报告问题** - 明确说明验证失败的具体内容
3. **回滚修改** - 如有必要，恢复到修改前状态
4. **重新修改** - 修正问题后再次验证

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
```cpp
// ✅ 正确的日志使用
static const char *TAG = "component_name";

ESP_LOGI(TAG, "Information message");
ESP_LOGW(TAG, "Warning message");
ESP_LOGE(TAG, "Error message: %s", esp_err_to_name(ret));

// ❌ 错误：打印敏感信息
ESP_LOGI(TAG, "WiFi password: %s", password);  // 禁止
```

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

## 📋 强制检查清单

### 🔄 每次操作前必须执行
```bash
# 1. 检查Git状态
git status --porcelain

# 2. 检查文件真实存在
git ls-files | grep [文件名]

# 3. 检查分支状态
git branch -v

# 4. 检查远程同步状态
git remote -v
```

### 📝 ESP32代码修改检查
```cpp
// ✅ 内存管理检查
// - malloc/free配对
// - cJSON_Parse/cJSON_Delete配对
// - nvs_open/nvs_close配对

// ✅ 错误处理检查
// - 所有esp_err_t返回值都检查
// - 失败时正确清理资源
// - 使用ESP_LOGE记录错误

// ✅ 安全性检查
// - 不打印密码、密钥等敏感信息
// - 输入参数长度验证
// - 缓冲区边界检查
```

### 🌐 HTTP API开发检查
```cpp
// ✅ API设计检查
// - 路径与handler注册一致
// - JSON字段名与文档一致
// - 错误响应格式统一

// ✅ 安全检查
// - 输入长度限制
// - CORS头设置
// - 不返回敏感信息
```

---

## 🛠️ 标准工作流程

### 1. **任务分析阶段**
```bash
# 1. 理解用户需求
# 2. 检查当前项目状态
git status
git log --oneline -5

# 3. 定位相关文件
find . -name "*.cpp" -o -name "*.h" | grep -E "(wifi|gpio|server)"

# 4. 验证文件存在
read_file path/to/file.h
```

### 2. **代码修改阶段**
```bash
# 1. 创建分支（重要修改）
git checkout -b feature/new-feature

# 2. 小步修改
# 3. 每次修改后验证
git status
git diff

# 4. 编译测试（如果有ESP-IDF环境）
idf.py build
```

### 3. **验证确认阶段**
```bash
# 1. 多工具交叉验证
git status
git diff --stat
read_file modified_file.cpp

# 2. 检查代码质量
# - 内存管理
# - 错误处理
# - 日志规范

# 3. 提交代码
git add .
git commit -m "feat: 具体功能描述"
```

---

## ⚠️ 禁止行为清单

### ❌ 绝对禁止
- 假设文件存在而不验证
- 删除用户代码后"重新实现"
- 添加用户未要求的"改进"
- 在无ESP-IDF环境时提交代码
- 混淆IDE显示与文件系统状态
- 打印密码、密钥等敏感信息

### ⚠️ 高风险操作（需用户确认）
- `git reset --hard`
- `git clean -fd`
- 删除整个目录
- 修改核心配置文件
- 修改主要组件接口

---

## 🔍 自我验证机制

### 📊 代码质量自检清单
每次代码修改后必须检查：

#### 内存管理
- [ ] 所有malloc都有对应的free
- [ ] 所有cJSON_Parse都有cJSON_Delete
- [ ] 所有nvs_open都有nvs_close
- [ ] 没有内存泄漏风险

#### 错误处理
- [ ] 所有esp_err_t返回值都检查
- [ ] 失败路径正确清理资源
- [ ] 使用ESP_LOGE记录错误
- [ ] 错误信息有意义

#### 安全性
- [ ] 不打印密码、密钥等敏感信息
- [ ] 输入参数长度验证
- [ ] 缓冲区边界检查
- [ ] 字符串以null结尾

#### ESP32规范
- [ ] GPIO编号符合芯片数据手册
- [ ] WiFi SSID长度≤32字节，密码≤64字节
- [ ] 使用正确的ESP-IDF API
- [ ] 日志TAG与组件名一致

#### Git规范
- [ ] 工作目录干净
- [ ] 提交信息描述清晰
- [ ] 没有意外的大文件
- [ ] 分支状态正确

### 🚨 发现问题立即停止
如果任何检查项不符合：
1. 停止所有操作
2. 报告具体问题
3. 等待用户指示
4. 不继续后续操作

---

## 📚 工具使用规范

### 文件操作工具
- `read_file` - 验证文件内容和存在性
- `list_dir` - 检查目录结构
- `write_to_file` - 创建文件后立即验证
- `edit` - 修改文件后检查结果

### Git操作工具
- `git status` - 检查工作目录状态
- `git log` - 查看提交历史
- `git reflog` - 查看操作记录
- `git diff` - 查看具体修改

### 搜索工具
- `Grep` - 搜索代码内容
- `find_by_name` - 查找文件

---

## 🎯 多AI助手兼容性

### 通用指令格式
```markdown
# 适用于所有AI助手的指令格式
1. 首先检查当前状态
2. 明确操作目标
3. 小步执行验证
4. 报告完成情况
```

### 工具映射表
| 功能 | Cursor | VSCode | Windsurf | Claude | Copilot |
|------|--------|--------|----------|--------|---------|
| 文件读取 | @file | #file | /file | 文件读取 | 文件内容 |
| 文件编辑 | @edit | #edit | /edit | 文件编辑 | 编辑文件 |
| 终端命令 | @terminal | #terminal | /terminal | 终端命令 | 命令执行 |

---

## 📝 沟通规范

### ✅ 成功操作报告
```
✅ 已完成：[具体完成的操作]
📊 当前状态：[git status结果]
🔍 验证结果：[自检清单结果]
📋 下一步：[建议的后续操作]
```

### ❌ 错误报告
```
❌ 操作失败：[具体错误]
🔍 原因分析：[错误原因]
🛠️ 解决方案：[具体解决步骤]
📋 预防措施：[避免重复的方法]
```

### ⚠️ 矛盾报告
```
⚠️ 发现矛盾：[IDE显示 vs 工具结果]
📊 实际状态：[工具验证结果]
🔍 预期状态：[用户描述状态]
📋 建议操作：[具体解决步骤]
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

### WiFi配置PR示例
```cpp
// ✅ 符合OCP的设计：
// web_server.cpp 通过注册方式添加新URI
// wifi_config_store 独立组件，不影响其他存储功能
// main.cpp 仅添加初始化调用，不修改现有初始化流程
```

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

## � Git提交前检查清单

### 强制流程
**任何代码在commit前必须完成以下检查，否则禁止提交！**

### 1. 代码审查 (Code Review)
```bash
# 查看所有变更
git diff

# 检查变更范围
git diff --stat
```
- [ ] 确认只修改了必要的文件
- [ ] 确认没有意外修改（如调试代码、测试文件）
- [ ] 确认符合最小化修改原则
- [ ] 确认没有删除或修改他人代码

### 2. 规则合规检查
- [ ] 所有新增代码符合.md规则
- [ ] 所有日志使用中文
- [ ] 所有注释使用中文
- [ ] 函数头注释完整（Doxygen格式）
- [ ] 符合开闭原则(OCP)
- [ ] 内存管理正确（无内存泄漏）
- [ ] 错误处理完善

### 3. 编译验证
```bash
# 清理并重新编译
idf.py clean
idf.py build
```
- [ ] 无编译错误
- [ ] 无编译警告（或警告已评估并接受）
- [ ] 链接成功

### 4. 单元测试/功能验证
- [ ] 新增功能已本地测试
- [ ] 现有功能未被破坏
- [ ] 边界条件已测试

#### ESP32嵌入式项目测试策略

由于ESP-IDF嵌入式项目通常无单元测试框架，采用以下验证方式：

**Level 1: 编译时验证（强制）**
```bash
idf.py build
```
- 无编译错误/警告
- 链接成功，生成固件

**Level 2: 静态代码检查（强制）**
- 代码边界检查：`if (len > MAX_SIZE)`
- 空指针检查：`if (ptr == NULL)`
- 内存管理：malloc/free配对，句柄关闭
- 错误处理：所有分支返回正确错误码

**Level 3: 功能自检（代码内实现）**
```cpp
// 参数验证
if (!ssid || !password) return ESP_ERR_INVALID_ARG;

// 范围验证
if (req->content_len > 512) return ESP_ERR_INVALID_SIZE;

// 状态验证
if (!s_wifi_initialized) return ESP_ERR_INVALID_STATE;
```

**Level 4: 硬件测试（有条件时执行）**
- 烧录固件到ESP32开发板
- 功能API测试（HTTP请求）
- NVS存储持久化验证
- WiFi连接重连验证

**本次WiFi配置PR验证清单：**
- ✅ 编译通过：`yonovate-v2.bin` 生成成功
- ✅ 静态检查：所有函数有参数/边界检查
- ✅ 内存管理：`cJSON_Delete`/`nvs_close`正确配对
- ✅ 错误处理：HTTP状态码/NVS错误码正确返回
- ⏳ 硬件测试：待实际烧录验证（需开发板）

### 5. Git状态确认
```bash
# 检查git状态
git status

# 查看准备提交的文件列表
git diff --cached --name-only
```
- [ ] 确认提交的文件都是预期的
- [ ] 确认没有遗漏的变更
- [ ] 确认提交信息已准备

### 6. 提交信息规范
```bash
git commit -m "类型: 简洁描述"
```
- [ ] 提交信息使用中文
- [ ] 描述清晰准确
- [ ] 包含关联的issue/PR编号（如有）

### 7. 最终确认清单
```bash
# 提交前最后检查
git diff --cached
```
- [ ] 已再次审查所有变更
- [ ] 确认符合所有.md规则
- [ ] 已本地编译验证通过
- [ ] 已功能测试通过
- [ ] 准备提交

---

### 📋 执行要点（Windsurf/VSCode环境）

#### 环境要求
- **终端类型**：使用 `ESP-IDF CMD` 或 `ESP-IDF PowerShell`
- **工作目录**：必须在项目根目录 `c:\Downloads\code-v2\v2`
- **环境初始化**：编译前必须先执行 `export.bat`（CMD）或 `Initialize-Idf.ps1`（PowerShell）

#### 标准执行流程

**步骤1：Git变更检查**
```bash
cd c:\Downloads\code-v2\v2
git status --short
git diff --stat
```

**步骤2：编译验证（CMD方式）**
```bash
cd c:\Downloads\code-v2\v2
C:\Espressif\frameworks\esp-idf-v5.5.3\export.bat
idf.py build
```
预期输出：`Project build complete.`

**步骤3：提交代码**
```bash
cd c:\Downloads\code-v2\v2
git add .
git commit -m "feat: 功能描述"
```
预期输出：`[master abc1234] feat: xxx`

#### 验证成功标志

| 命令 | 成功输出示例 |
|------|-------------|
| `git status --short` | `M main/main.cpp` / `?? components/new/` |
| `idf.py build` | `Project build complete.` |
| `git commit` | `[master e273101] feat: xxx` |

#### 实际验证成功案例（2024-04-11）

**方式1：ESP-IDF 5.5 CMD（推荐）**
```batch
C:\Espressif\frameworks\esp-idf-v5.5.3>cd C:\Downloads\code-v2\v2
C:\Downloads\code-v2\v2>idf.py build
```
输出：
```
Bootloader binary size 0x6630 bytes. 0x9d0 bytes (9%) free.
yonovate-v2.bin binary size 0xdc870 bytes. 0x23790 bytes (14%) free.
Project build complete.
```

**方式2：VSCode终端（ESP-IDF CMD配置）**
- 默认终端：`"terminal.integrated.defaultProfile.windows": "ESP-IDF CMD"`
- 直接执行：`idf.py build`

**方式3：批处理脚本（解决引号转义问题）**
```batch
@echo off
cd /d c:\Downloads\code-v2\v2
git add .
git commit -m "feat: description"
```

#### 常见问题处理

**问题1：git命令执行超时**
- 解决：使用 `cmd /c "cd 项目路径 && git命令"` 方式执行

**问题2：idf.py命令不识别**
- 原因：未执行export.bat初始化环境
- 解决：先执行 `C:\Espressif\frameworks\esp-idf-v5.5.3\export.bat`

**问题3：中文提交信息乱码**
- 解决：使用英文提交信息，或在批处理脚本中提交

---

## 🚨 紧急停止条件

遇到以下情况立即停止所有操作：

1. 文件存在性矛盾 - IDE显示存在但工具验证不存在
2. Git状态异常 - 无法解释的git status结果
3. 用户指出错误 - 用户认为操作有误
4. 工具结果冲突 - 不同工具返回矛盾结果
5. 任何不确定的情况 - 无法100%确认操作正确性

---

## 📈 持续改进

### 规则更新机制
- 每次遇到新问题时更新规则
- 定期回顾和完善检查清单
- 根据项目特点调整最佳实践
- 保持与ESP-IDF版本同步

### 学习机制
- 记录常见错误和解决方案
- 分析失败案例，改进规则
- 收集用户反馈，优化流程
- 跟踪ESP-IDF最新最佳实践

---

**重要：这些规则是强制性的，违反规则会导致严重后果。每次操作前必须回顾相关规则，并进行自我验证。**

*版本：1.0 | 更新日期：2026-04-11 | 适用：所有ESP32/嵌入式AI编程项目*
