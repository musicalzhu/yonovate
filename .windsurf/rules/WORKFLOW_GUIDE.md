# AI编程操作手册 - 工作流程与检查清单

> **适用范围**: 所有AI编程项目
> **用途**: 执行流程指导、检查清单、问题排查

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

// ✅ 版本号管理（强制规则）
// - 每次修改web页面后必须递增小版本号（v1.3 → v1.4）
// - ESP32内嵌WiFi配置页面：修改subtitle中的版本号
// - web_project多页面项目：同步更新所有相关页面版本号
// - 版本号格式：v主版本.次版本（如v1.4）
// - 目的：确保用户能确认页面已更新，避免缓存问题

// ✅ Web页面验证检查（烧录后必做）
// - 强制刷新页面 (Ctrl+F5) 清除浏览器缓存
// - 验证页面版本号是否更新（v1.3 → v1.4）
// - 如版本未更新，使用无痕模式验证
// - 检查设备是否重启并重新连接AP
// - 如仍异常，清除浏览器全部缓存后重试
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

# 4. 编译测试（AI自动执行，无需用户确认）
// turbo - 代码修改完成后AI立即自动编译，不等待用户指令
cmd.exe /c "C:\Espressif\frameworks\esp-idf-v5.5.3\export.bat && cd /d <project_path> && idf.py build"

**编译成功标准**：
- 输出包含 "Project build complete."
- 固件大小 < 分区大小（0xe12c0 < 0x100000）
- 剩余空间 > 10%

**编译后AI自动检查**：
- 分析编译日志，报告错误和警告
- 显示固件大小信息
- 准备烧录命令（但不执行）

**AI自我检查清单**（每次编译前必须自问）：
```markdown
□ 执行方式：是否使用cmd.exe /c？（禁用PowerShell直接执行）
□ 环境脚本：是否使用export.bat？（禁用export.ps1）
□ 执行模式：是否顺序执行Blocking=true？（禁用后台执行）
□ 输出处理：是否避免管道过滤？（禁用Select-String）
□ 结果验证：是否检查文件时间戳？（禁用日志解析）
□ 路径格式：是否使用/d参数切换盘符？（Windows必需）
□ 🆕 Web页面修改：是否更新版本号？（强制！v1.x → v1.(x+1)）
□ 🆕 修改确认：AI回复"已完成"前必须再次自查所有修改点
```

**AI回复"已完成/已修复"前的强制自查流程**：
```markdown
1. 回顾本次修改的所有文件
2. 检查是否涉及web页面修改
   └─► 是 → 立即检查版本号是否已递增
3. 对照.md规则逐项确认
4. 确认无误后才能回复"已完成"
5. 如有遗漏，立即修正并重新自查
```

**参考文档**：`.windsurf/rules/TROUBLESHOOTING.md` - "AI自动执行构建失败问题"章节

**反例**（AI常犯错误，必须避免）：
```powershell
# ❌ 错误方式（失败率70%）
powershell -NoProfile -Command ". C:\Espressif\frameworks\esp-idf-v5.5.3\export.ps1; idf.py build 2>&1 | Select-String -Pattern 'error|complete'"
# 问题：PS1环境不完整、后台执行、管道过滤

# ❌ 错误方式（失败率50%）
run_command -Blocking:false -Command "idf.py build"
# 问题：后台执行导致状态不同步，无法获取结果

# ❌ 错误方式（失败率30%）
powershell -Command "idf.py build"  # 缺少export环境
# 问题：未激活ESP-IDF环境，命令找不到
```

**正例**（AI必须采用，成功率100%）：
```powershell
# ✅ 正确方式（成功率100%）
cmd.exe /c "C:\Espressif\frameworks\esp-idf-v5.5.3\export.bat && cd /d C:\Downloads\code-v2\v2 && idf.py build"
# 验证：if (Test-Path 'build\yonovate-v2.bin') { "✅ Build successful" }
# 注意：顺序执行、CMD环境、文件验证
```

# 5. 烧录（需要用户确认）
❌ 不要自动执行 - 必须等待用户说"烧录"或"确认"
idf.py -p PORT flash
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

## Git提交前检查清单

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

#### ⚠️ Windows环境Git常见问题排查

**问题现象**: bash工具执行git命令只返回Windows版本信息，无法获取实际输出
```
Microsoft Windows [版本 10.0.xxxxx.xxxx]
(c) Microsoft Corporation。保留所有权利。

C:\Users\[用户名]\AppData\Local\Programs\[AI工具]>
```
> 注：以上为示例路径，实际路径取决于系统和AI工具安装位置

**根本原因**: `.git/config` 文件使用Unix换行符(LF)而非Windows换行符(CRLF)

**识别方法**:
```bash
# 检查.git/config换行符格式
type .git\config | findstr /R "\r"
# 如无输出，说明是Unix格式(LF)，需要修复
```

**解决方案**:
```bash
# 方法1: 使用dos2unix/unix2dos工具
unix2dos .git\config

# 方法2: 使用IDE重新保存文件
# 在IDE中打开.git/config，右下角选择"CRLF"，保存

# 方法3: 使用PowerShell
(Get-Content .git\config) -join "`r`n" | Set-Content .git\config -NoNewline
```

**验证修复**:
```bash
# 修复后git命令应正常执行
git status
git log --oneline -3
```

**预防措施**:
- Windows项目提交.git/config前确保使用CRLF格式
- 跨平台协作时统一换行符规范，建议使用`.gitattributes`配置

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

### 📋 执行要点（IDE环境）

#### 环境要求
- **终端类型**：使用 `ESP-IDF CMD` 或 `ESP-IDF PowerShell`
- **工作目录**：必须在项目根目录 `[项目根目录]`
- **环境初始化**：编译前必须先执行 `export.bat`（CMD）或 `Initialize-Idf.ps1`（PowerShell）

#### 标准执行流程

**步骤1：Git变更检查**
```bash
cd [项目根目录]
git status --short
git diff --stat
```

**步骤2：编译验证（AI自动执行，无需用户确认）**
```bash
// turbo - 代码修改完成后AI立即自动编译，不等待用户说"编译"
cmd.exe /c "C:\Espressif\frameworks\esp-idf-v5.5.3\export.bat && cd /d [项目根目录] && idf.py build"
```

**编译成功标准**：
- 输出包含 `Project build complete.`
- 固件大小 < 分区大小（如 0xe12c0 < 0x100000）
- 剩余空间 > 10%

**编码规范检查（编译前自动执行）**：
```bash
# AI必须在编译前检查：ESP32固件日志必须使用英文
# 禁止中文日志和UTF-8特殊符号（✓✗⚠）
# 原因：Windows GBK编码与ESP32 UTF-8冲突，导致监控失败
```

**AI行为要求**：
1. 修改`.cpp/.c/.h`文件后立即自动编译（SafeToAutoRun: true）
2. 编译完成后立即报告结果（成功/失败）
3. 失败后停止，不继续后续步骤
4. 成功后准备烧录命令，但不自动执行（烧录需要用户确认）

**步骤3：烧录与日志监控（需要用户确认）**

**3.1 烧录命令（用户确认后执行）**
```bash
# ❌ 不要自动执行 - 必须等待用户确认硬件连接

# 正确命令（带编码设置，避免中文乱码）
cmd.exe /c "chcp 65001 && set PYTHONIOENCODING=utf-8 && C:\Espressif\frameworks\esp-idf-v5.5.3\export.bat && cd /d [项目根目录] && idf.py -p [PORT] flash monitor"

# 备用：使用PuTTY（更稳定）
# 配置：Serial, [PORT], 115200 baud, UTF-8编码
```

**3.2 日志监控检查点（AI必须执行）**

烧录后必须监控以下日志序列：

| 顺序 | 必须看到的日志 | 含义 | 如果没出现 |
|-----|--------------|------|-----------|
| 1 | `WiFi init` / `Smart Dual Mode` | WiFi初始化开始 | ⚠️ 固件未启动 |
| 2 | `[1/7] Initializing NVS` | 存储初始化 | ⚠️ NVS可能损坏 |
| 3 | `APSTA dual mode started` | AP已启动 | ⚠️ WiFi启动失败 |
| 4 | `OK STA connected` 或 `FAIL Timeout` | 连接结果 | 等待30秒 |
| 5 | `HTTP Server started` | Web服务器启动 | ⚠️ 无法访问配网页 |

**3.3 错误日志立即报告**
```
CRITICAL错误（停止并报告）：
- "NVS init failed" - 存储损坏
- "WiFi init failed" - WiFi启动失败
- "HTTP server start failed" - 服务器失败

WARNING警告（记录但继续）：
- "No WiFi config in NVS" - 使用默认配置
- "STA connection timeout" - 进入AP配网模式
```

**3.4 日志监控报告格式**
```
📊 烧录监控报告：

启动检查:
  [1/5] ✓ WiFi初始化
  [2/5] ✓ NVS存储
  [3/5] ✓ AP启动 (SSID: Yonovate-Setup)
  [4/5] ✓ STA连接结果 (OK/FAIL)
  [5/5] ✓ Web服务器

连接结果:
  → STA模式: IP=xxx.xxx.xxx.xxx (OK)
  → 或 AP模式: 配网中 (FAIL)

配网信息:
  SSID: Yonovate-Setup
  Pass: 12345678
  URL: http://192.168.4.1
```
预期输出：`Project build complete.`

**步骤4：提交代码（用户确认后执行）**
```bash
cd [项目根目录]
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

**方式1：ESP-IDF CMD（推荐）**
```batch
[ESP-IDF安装路径]>cd [项目根目录]
[项目根目录]>idf.py build
```
输出：
```
Bootloader binary size 0x6630 bytes. 0x9d0 bytes (9%) free.
[项目名].bin binary size 0xdc870 bytes. 0x23790 bytes (14%) free.
Project build complete.
```

**方式2：IDE终端（ESP-IDF CMD配置）**
- 默认终端：配置为ESP-IDF CMD
- 直接执行：`idf.py build`

**方式3：批处理脚本（解决引号转义问题）**
```batch
@echo off
cd /d [项目根目录]
git add .
git commit -m "feat: description"
```

#### 常见问题处理

**问题1：git命令执行超时**
- 解决：使用 `cmd /c "cd 项目路径 && git命令"` 方式执行

**问题2：idf.py命令不识别**
- 原因：未执行export.bat初始化环境
- 解决：先执行 `[ESP-IDF安装路径]\export.bat`

**问题3：中文提交信息乱码**
- 解决：使用英文提交信息，或在批处理脚本中提交

**问题4：文件存在性矛盾（AI幻觉高危区）**
- **现象**：IDE显示文件存在，但`read_file`或`git ls-files`验证不存在
- **原因**：IDE缓存与实际文件系统不一致
- **解决**：
  1. 以工具验证结果为准，不以IDE显示为准
  2. 使用`git status`确认文件真实状态
  3. 如确实不存在，报告用户而非假设存在
- **参考**：`CORE_RULES.md` 真实性验证原则

**问题5：编译错误排查**
- **现象**：`idf.py build`报错，无法生成固件
- **排查步骤**：
  1. 检查错误类型：语法错误/链接错误/配置错误
  2. 语法错误：检查最近修改的文件
  3. 链接错误：检查函数声明与实现是否一致
  4. 配置错误：检查`sdkconfig`和`CMakeLists.txt`
  5. 清理后重试：`idf.py clean && idf.py build`
- **解决**：根据具体错误信息修复对应问题

**问题6：Git冲突处理**
- **现象**：`git pull`或`git merge`提示冲突
- **解决**：
  1. 暂停操作，不强制合并
  2. 使用`git status`查看冲突文件列表
  3. 读取冲突文件，分析冲突标记`<<<<<<< HEAD`
  4. 与用户确认保留哪个版本
  5. 解决后`git add . && git commit -m "解决冲突"`
- **禁止**：未经用户确认擅自删除他人代码

**问题7：工具执行超时/失败**
- **现象**：`run_command`或`grep_search`返回超时或失败
- **原因**：命令复杂、输出过大、环境限制
- **解决**：
  1. 简化命令，分步执行
  2. 限制输出字符数（如`OutputCharacterCount: 500`）
  3. 使用`Blocking: false`非阻塞执行长命令
  4. 检查命令语法，避免管道/重定向问题
  5. 改用其他工具替代（如用`read_file`替代`grep`小文件）

**问题8：配置文件格式错误**
- **现象**：修改`settings.json`或`tasks.json`后格式错误
- **识别**：`read_file`显示红色错误或JSON解析失败
- **解决**：
  1. 使用JSON验证工具检查语法
  2. 常见问题：逗号多余、引号不匹配、括号未闭合
  3. 修复后再次`read_file`确认
  4. 参考`CORE_RULES.md`修改后验证机制

**问题9：串口端口占用（PermissionError: 拒绝访问）**
- **现象**：`idf.py -p COM5 flash`报错：`PermissionError(13, '拒绝访问', None, 5)`或`Could not open COM5, the port is busy`
- **原因**：
  - 之前的`idf.py monitor`未正确关闭
  - `python.exe`或`esptool.exe`进程仍在占用串口
  - 多个AI会话同时尝试访问同一串口
- **AI自动解决方案（执行以下步骤）**：
  1. 清理占用进程（自动执行）：
     ```cmd
     taskkill /F /IM python.exe 2>nul
     taskkill /F /IM esptool.exe 2>nul
     timeout 2
     ```
  2. 等待2秒让系统释放端口
  3. 重新执行烧录命令
  4. 如仍失败，提示用户手动关闭所有终端窗口后重试
- **预防措施**：
  - 每次烧录前自动执行进程清理
  - 串口操作完成后确保进程正确退出
  - 避免同时运行多个串口监视器

---

**重要：这些操作流程是强制性的，违反会导致操作错误或项目风险。每次执行前必须回顾相关流程，并进行自我检查。**

---

*版本：1.0 | 更新日期：2026-04-11 | 适用：所有AI编程项目*
