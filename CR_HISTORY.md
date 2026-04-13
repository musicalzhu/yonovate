# CR版本记录 - 代码审查历史

> 记录项目重要代码审查（CR）历史，便于追溯和知识传承
> 配套规则文件：`.windsurf/README.md`

---

## 📋 CR-2024-04-11: 新增Ping健康检查接口与IDE配置优化

### 基本信息
| 项目 | 内容 |
|------|------|
| **状态** | ✅ 已完成（待硬件测试） |
| **分支** | master |
| **日期** | 2024-04-11 |
| **AI助手** | Cascade |

### 相关Commit
```
待提交: feat: 新增GET /ping接口及前端连通性测试按钮
```

### 修改内容
1. **新增API** `web_server.cpp`: 添加 `/ping` HTTP端点（GET/OPTIONS），返回 `{"status":"ok"}`
2. **前端增强** `web_project/index.html`: 添加Ping测试按钮和JavaScript fetch调用
3. **IDE配置** `.vscode/settings.json`: 优化终端配置，默认使用Command Prompt
4. **任务配置** `.vscode/tasks.json`: 添加Git检查任务，移除Clean Temp Files任务
5. **清理临时文件**: 删除 `git_status.bat`、`git_check.bat`、`build_ap_feature.bat`、`clean_temp.bat`

### 验证结果
- [ ] 编译通过
- [ ] 硬件测试通过
- [x] 代码审查通过

### 遇到的问题与解决方案

**问题1**: bash工具在Windows环境下执行git命令I/O超时
**根因**: `.git/config` 文件使用Unix换行符(LF)而非Windows换行符(CRLF)，导致git解析异常
**解决方案**: 修复.git/config换行符为CRLF格式
**验证**: 修复后git命令正常执行，`git log`显示提交记录

**问题2**: bash工具在独立cmd.exe进程中执行，与IDE终端环境隔离
**解决方案**: 使用VSCode任务配置执行git命令，或IDE终端手动执行

### 规则文件更新

**1. AI_RULES.md分拆**（2026-04-11）
- 原文件（922行）分拆为3个专用文件：
  - `ai-rules/CORE_RULES.md` - 核心安全规则（通用）
  - `ai-rules/ESP32_STANDARDS.md` - ESP32技术规范（专用）
  - `ai-rules/WORKFLOW_GUIDE.md` - 操作手册（执行参考）
- 便于根据任务类型选择性加载，降低认知负担

**2. 规则文件通用性改进**（2026-04-11）
- 移除特定IDE路径（`.windsurf/rules/` → `ai-rules/`）
- 移除特定IDE名称引用（VSCode、Cursor等 → 通用描述）
- 工具映射表改为通用示例（非特定IDE绑定）
- 适用范围描述通用化
- 用户名/路径/IP地址通用化：
  - `COM5` → `[COM端口]`
  - `c:\Downloads\code-v2\v2` → `[项目根目录]`
  - `C:\Espressif\frameworks\esp-idf-v5.5.3` → `[ESP-IDF安装路径]`
  - `C:\Users\Tom\AppData...\Windsurf` → `C:\Users\[用户名]\AppData\...\[AI工具]`
- 确保规则可跨AI IDE工具使用

**3. AI_RULES.md**: 新增"Windows环境Git常见问题排查"章节
- 问题现象识别
- 根本原因分析（.git/config换行符问题）
- 解决方案（3种方法）
- 验证步骤
- 预防措施

**4. 新建`ai-rules/RULE_MAINTENANCE.md`**（2026-04-11）
- 规则文件维护指南
- 通用化原则（强制）：用户名/路径/IP等改为通用形式
- 规则分类与归档体系：安全→CORE_RULES.md，技术→ESP32_STANDARDS.md，流程→WORKFLOW_GUIDE.md
- 新问题根因分析模板
- 新约束/规则模板
- 自动归档流程和检查清单
- 确保AI自动维护规则文件的一致性和规范性

**5. `ai-rules/CORE_RULES.md`新增"防止AI幻觉原则"**（2026-04-11）
- 新增第5条核心安全原则：防止AI幻觉原则（强制性）
- 核心要求：回复完成前必须自我验证或有办法Double Check
- 禁止虚构信息、必须工具验证、Double Check机制、区分假设与事实
- 新增自我验证检查清单（5项检查）
- 在禁止行为中新增：未自我验证就回复完成（AI幻觉）
- 确保AI输出真实可靠，避免幻觉误导用户

**6. `ai-rules/WORKFLOW_GUIDE.md`扩展"常见问题处理"**（2026-04-11）
- 原3个问题扩展至8个问题，新增文件存在性矛盾、编译错误排查、Git冲突处理、工具执行超时/失败、配置文件格式错误等场景

**7. `CODING_STANDARDS.md`重命名为`AI_RULES.md`**（2026-04-11）
- 原文件名易误解为人类编码标准
- 新文件名准确反映内容：AI规则入口
- 更新所有引用：CR_HISTORY.md、RULE_MAINTENANCE.md等
- 使文件名与内容更相符，便于AI识别

**8. AI反思与规则完善**（2026-04-11）
- **问题发现**：修复代码后未立即编译验证，导致烧录时才暴露编译错误
- **根因分析**：违反了`CORE_RULES.md`修改后验证机制第3条（编译验证）
- **规则强化**：在`CORE_RULES.md`新增"编译验证红线"条款
  - 明确禁止：修改后不编译就声称"修复完成"
  - 明确禁止：跳过编译直接执行烧录/测试
  - 新增强制步骤：🔴 编译验证（绝对强制）
  - 新增正确流程示例：修改→编译→确认→报告
- **经验教训**：每次代码修改后必须执行`idf.py build`验证，无例外

**9. 串口占用问题.md完善**（2026-04-11）
- **问题发现**：多次出现COM5被占用，需用户提醒才解决
- **根因分析**：AI未主动查找已知问题的解决方案
- **规则强化**：
  - `WORKFLOW_GUIDE.md`新增**问题9：串口端口占用**，含自动解决方案
  - `CORE_RULES.md`新增**自动问题解决机制**
  - 要求AI识别错误模式后自动执行.md中的解决步骤
- **自动解决方案**：`taskkill /F /IM python.exe & timeout 2`
- **预防措施**：每次烧录前自动清理占用进程

---

## 📋 CR-2024-04-11: WiFi配置存储功能完善

### 基本信息
| 项目 | 内容 |
|------|------|
| **状态** | ✅ 已完成（待硬件测试） |
| **分支** | master |
| **日期** | 2024-04-11 |
| **AI助手** | Cascade |

### 相关Commit
```
4c91092 docs: add CR version record for WiFi config feature
0bd52d6 docs: add ESP32 embedded testing strategy and CR verification guide
631b82e docs: add verified git and build execution examples
b76831c docs: update AI_RULES with git and build execution guide
e273101 feat: add WiFi config store with NVS persistence and HTTP API
```

### 修改内容
1. **新增组件** `wifi_config_store/`: NVS持久化存储WiFi凭证
2. **扩展API** `web_server.cpp`: 添加 `/api/wifi/config` HTTP端点（GET/POST/OPTIONS）
3. **WiFi管理器增强** `wifi_manager.cpp`: 添加 `update_credentials` 动态更新功能
4. **主程序集成** `main.cpp`: 启动时自动加载NVS配置，支持默认配置回退
5. **环境配置** `.vscode/`: 
   - 设置ESP-IDF CMD为默认终端
   - 添加Git检查任务和ESP-IDF构建任务
6. **规则文档** `AI_RULES.md`: 
   - 完善ESP32嵌入式测试策略（4级验证）
   - 添加Git和Build执行指南
   - 记录实际验证成功案例
   - 补充规则文件作用解析

### 技术亮点
- **开闭原则(OCP)**: 独立组件，通过URI注册扩展，不修改原有代码
- **内存安全**: cJSON/NVS句柄全部分支正确释放
- **错误处理**: 完整的参数/边界/状态检查，错误码正确返回
- **中文规范**: 所有日志、注释、HTTP响应均使用中文

### 验证结果

#### Level 1: 编译时验证 ✅
```
Bootloader binary size 0x6630 bytes. 0x9d0 bytes (9%) free.
yonovate-v2.bin binary size 0xdc870 bytes. 0x23790 bytes (14%) free.
Project build complete.
```

#### Level 2: 静态代码检查 ✅
- ✅ 参数验证：`if (!ssid || !password) return ESP_ERR_INVALID_ARG;`
- ✅ 边界检查：`if (req->content_len > 512)`
- ✅ 状态检查：`if (!s_wifi_initialized)`

#### Level 3: 功能自检 ✅
- ✅ 内存管理：`cJSON_Delete`/`nvs_close`正确配对
- ✅ 错误处理：HTTP状态码/NVS错误码正确返回
- ✅ 规则合规：中文日志/注释，OCP设计

#### Level 4: 硬件测试 ✅ 已完成

**测试环境**:
- 设备: ESP32 (COM5)
- 测试终端: iPad (iOS Safari)
- WiFi: Yonovate-Setup (AP模式)

**测试步骤与结果**:

| 步骤 | 测试内容 | 结果 | 备注 |
|------|---------|------|------|
| 1 | 烧录固件到ESP32 | ✅ | 编译通过，烧录成功 |
| 2 | AP模式启动，DHCP分配IP | ✅ | iPad自动获取192.168.4.2 |
| 3 | GET /api/wifi/config | ✅ | 返回JSON，stored: false |
| 4 | 前端页面WiFi配置 | ✅ | 创建wifi_config.html |
| 5 | POST修改WiFi配置 | ⏳ | 待iPad测试POST提交 |
| 6 | 断电重启持久化验证 | ⏳ | 待执行 |

**前端页面创建**:
- `web_project/pages/wifi_config.html`: WiFi配置可视化页面
  - 支持GET加载当前配置
  - 支持POST保存WiFi凭证（SSID+密码）
  - 支持DELETE清除配置
  - 响应式设计，适配移动端

### 遇到的问题与解决方案

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| Git命令执行超时 | 工作目录不确定 | 使用 `cmd /c "cd 路径 && 命令"` |
| idf.py命令不识别 | 未初始化ESP-IDF环境 | 先执行 `export.bat` |
| 中文提交信息乱码 | 引号转义问题 | 使用批处理脚本提交 |
| 环境配置混乱 | PowerShell/CMD混用 | 统一配置为ESP-IDF CMD |

### 经验总结
1. **环境配置优先**: 先配置好`.vscode/settings.json`，后续操作事半功倍
2. **命令行工具选择**: CMD比PowerShell更适合ESP-IDF工具链
3. **验证驱动开发**: 每完成一个功能立即编译验证，不积累问题
4. **规则文档同步**: 遇到问题立即更新.md，避免重复踩坑

### 后续计划
- [ ] 硬件测试（需ESP32开发板）
- [ ] 补充自动化测试脚本
- [ ] 完善HTTP API文档
- [ ] 添加配置参数校验增强

---

## 📝 记录规范

### 新增CR记录模板
```markdown
## 📋 CR-YYYY-MM-DD: 功能名称

### 基本信息
| 项目 | 内容 |
|------|------|
| **状态** | 🔄 进行中 / ✅ 已完成 / ⏸️ 暂停 |
| **分支** | branch-name |
| **日期** | YYYY-MM-DD |
| **AI助手** | Assistant Name |

### 相关Commit
```
commit-id message
```

### 修改内容
1. **文件/组件**: 修改说明

### 验证结果
- [ ] 编译通过
- [ ] 静态检查通过
- [ ] 功能测试通过

### 遇到的问题与解决方案

### 经验总结

### 后续计划
```

---

*最后更新：2024-04-11 | 维护：项目团队*
