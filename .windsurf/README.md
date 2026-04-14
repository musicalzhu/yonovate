# AI编程标准与最佳实践 - Windsurf IDE规则入口

> **适用于**：Windsurf IDE + 各类AI编程助手
> **专注领域**：ESP32/嵌入式开发、C/C++、固件开发
> **更新时间**：2026-04-13
> **目录位置**：`.windsurf/`（Windsurf IDE自动加载）

---

## 📚 规则文件结构

本项目的AI编程规则已整合到`.windsurf/`目录，便于Windsurf IDE自动识别和加载：

```
.windsurf/
├── README.md              # 本文件 - 规则总索引入口
├── CASE_INDEX.md          # 🆕 AI案例检索索引（违规案例触发器）
├── rules/                 # 通用规则目录（Windsurf自动加载）
│   ├── CORE_RULES.md      # 核心安全规则、禁止行为、沟通规范
│   ├── ESP32_STANDARDS.md # ESP-IDF最佳实践、技术规范、AI自动执行规范
│   ├── CODE_REVIEW.md     # C/C++代码审查检查清单、内存安全、并发安全
│   ├── TESTING.md         # 单元测试策略、CI/CD集成、测试覆盖率
│   ├── WORKFLOW_GUIDE.md  # 工作流程、检查清单、问题排查
│   ├── RULE_MAINTENANCE.md# 规则维护指南、归档流程
│   ├── TROUBLESHOOTING.md # 编码问题、日志监控、AI执行失败排查
│   └── README.md          # rules目录说明
└── workflows/             # 可执行workflow文件（Windsurf自动加载）
    ├── README.md          # workflows目录说明
    ├── esp32-build.md     # ESP32编译烧录workflow
    └── log-monitoring.md  # 日志监控workflow
```

---

## 📖 规则文件索引

| 文件 | 路径 | 内容 | 适用场景 |
|------|------|------|----------|
| **案例检索索引** | `CASE_INDEX.md` | AI违规案例触发器、场景关键词匹配、查阅强制约束 | **执行前必查** |
| **核心安全规则** | `rules/CORE_RULES.md` | 安全原则、禁止行为、沟通规范、自动编译规则 | 所有项目必看 |
| **ESP32技术规范** | `rules/ESP32_STANDARDS.md` | ESP-IDF最佳实践、代码注释、OCP原则、AI自动执行技术规范 | ESP32项目专用 |
| **代码审查指南** | `rules/CODE_REVIEW.md` | C/C++代码审查检查清单、内存安全、并发安全、静态分析 | 代码审查参考 |
| **测试与CI/CD** | `rules/TESTING.md` | 单元测试策略、CI/CD集成、测试覆盖率、GitHub Actions | 测试流程参考 |
| **操作手册** | `rules/WORKFLOW_GUIDE.md` | 工作流程、检查清单、Git提交指南、问题排查、AI自我检查清单 | 执行参考 |
| **规则维护指南** | `rules/RULE_MAINTENANCE.md` | 如何维护规则文件、新问题归档流程、通用化原则 | AI自动维护参考 |
| **问题排查** | `rules/TROUBLESHOOTING.md` | 编码问题、日志监控、AI自动执行构建失败问题、成功案例 | 故障诊断 |
| **编译workflow** | `workflows/esp32-build.md` | ESP32编译烧录步骤、AI执行最佳实践、学习验证记录 | 自动执行 |
| **监控workflow** | `workflows/log-monitoring.md` | 烧录测试日志监控规范、检查点、错误报告 | 自动监控 |

---

## 🚀 快速开始

### 开始新项目时
1. 阅读 `rules/CORE_RULES.md` 了解安全原则
2. 查看 `rules/WORKFLOW_GUIDE.md` 的标准工作流程

### ESP32开发时
1. **执行前查阅** `CASE_INDEX.md` 匹配场景（强制！防止重复犯错）
2. 查阅 `rules/ESP32_STANDARDS.md` 获取技术规范
3. 参考 `workflows/esp32-build.md` 了解编译步骤

### 代码审查时
1. **执行前查阅** `CASE_INDEX.md` 代码审查相关案例
2. 遵循 `rules/CODE_REVIEW.md` 的检查清单逐项审查
3. 重点关注内存安全、并发安全、错误处理

### 测试与CI/CD
1. 查看 `rules/TESTING.md` 的单元测试策略
2. 集成GitHub Actions工作流

### 执行具体操作时
1. 按 `rules/WORKFLOW_GUIDE.md` 的检查清单执行
2. 遇到问题时查看 `rules/TROUBLESHOOTING.md`

### 发现新问题时
1. 遵循 `rules/RULE_MAINTENANCE.md` 归档到合适文件
2. 保持规则通用化（无具体路径/IP）

---

## 🎯 AI自动执行最佳实践（重点）

**核心发现**：AI在IDE中自动执行`idf.py build`需遵循特定方式

| 执行方式 | 成功率 | 说明 |
|---------|-------|------|
| `cmd.exe /c "export.bat && idf.py build"` | ✅ **100%** | **AI必须使用此方式** |
| PowerShell+后台+管道 | ❌ 30% | 经常失败，已验证 |

**关键规则**（来自`rules/WORKFLOW_GUIDE.md`）：
- ✅ 使用`cmd.exe /c`执行（非PowerShell直接执行）
- ✅ 使用`export.bat`（非export.ps1）
- ✅ 顺序执行`Blocking=true`（非后台）
- ✅ 避免管道过滤（无`Select-String`）
- ✅ 文件时间戳验证结果（非日志解析）

**验证记录**：2026-04-13 AI学习成功案例（见`workflows/esp32-build.md`）

---

## 🈶 规则文件作用与价值

### 为什么要使用规则文件？

本规则文件集合是**AI编程助手的行为准则**，旨在：

| 作用层级 | 具体价值 | 示例 |
|---------|---------|------|
| **约束AI行为** | 强制规范AI操作边界，防止越界 | 禁止假设文件存在，必须工具验证 |
| **保障代码质量** | 检查清单确保每次修改符合标准 | 编译→静态检查→功能自检→硬件测试 |
| **降低沟通成本** | 标准化流程，减少反复确认 | 提交前7步强制检查清单 |
| **知识沉淀** | 积累项目特定最佳实践 | ESP32测试策略、AI自动执行最佳实践 |
| **自我保护** | 明确"紧急停止条件" | 发现矛盾立即停止操作 |

### 核心价值

> **让AI从"随心编程"变成"规范编程"**

通过明确的规则，确保：
- ✅ 每次代码修改都经过验证
- ✅ 所有提交符合项目标准
- ✅ 团队协作有一致的行为准则
- ✅ 项目风险可控，可追溯

---

## 🚨 快速参考

### AI必须遵守的强制规则

1. **真实性验证原则** - 必须用工具验证文件存在
2. **最小化操作原则** - 只做用户明确要求的事
3. **自动编译原则** - 代码修改后立即自动编译（turbo）
4. **英文日志原则** - 所有ESP_LOG必须使用英文
5. **通用化原则** - 规则文件无具体路径/IP

### AI执行build前自检清单

```markdown
□ 使用cmd.exe /c？（禁用PowerShell直接执行）
□ 使用export.bat？（禁用export.ps1）
□ 顺序执行Blocking=true？（禁用后台）
□ 避免管道过滤？（禁用Select-String）
□ 检查文件时间戳？（禁用日志解析）
□ 使用/d参数切换盘符？（Windows必需）
```

---

## 🔗 外部参考

- **项目CR历史**: `../CR_HISTORY.md`
- **项目README**: `../web_project/README.md`
- **ESP-IDF文档**: https://docs.espressif.com/projects/esp-idf/

---

*版本：2.0 | 更新：2026-04-13 | 迁移：ai-rules/ → .windsurf/rules/ | 适用于：Windsurf IDE + ESP32项目*
