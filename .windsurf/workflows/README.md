# Windsurf Workflows - ESP32项目专用

> **目录用途**: Windsurf IDE专用的可执行workflow文件
> **与rules关系**: 本目录是`.windsurf/rules/`的具体执行实现，内容保持同步
> **适用范围**: ESP32/ESP-IDF嵌入式开发项目

---

## 📁 目录结构

```
.windsurf/workflows/
├── README.md              # 本文件 - 目录说明
├── esp32-build.md         # ESP32编译烧录workflow
└── log-monitoring.md      # 日志监控workflow
```

---

## 🔗 与ai-rules的关系

| 目录 | 用途 | 内容特点 |
|------|------|---------|
| `.windsurf/rules/` | 通用规则源 | 完整详细的规则文档（推荐主参考） |
| `.windsurf/workflows/` | IDE专用执行 | workflow格式的可执行步骤 |

**重要**: workflow和rules内容保持一致，修改时请同步更新！

---

## 📋 Workflow文件说明

### 1. esp32-build.md
**用途**: ESP32项目编译和烧录workflow  
**触发**: 代码修改后自动执行  
**关键标记**: `// turbo` - 表示可自动执行步骤

**核心规则**（来自`.windsurf/rules/CORE_RULES.md`）:
- ✅ AI自动编译（无需用户确认）
- ❌ 烧录需用户确认（硬件操作）

### 2. log-monitoring.md
**用途**: ESP32烧录测试日志监控workflow  
**触发**: 烧录完成后自动执行  
**检查点**: WiFi启动序列、连接结果、HTTP服务器

**核心规则**（来自`.windsurf/rules/ESP32_STANDARDS.md`）:
- 日志必须使用英文（避免编码问题）
- 关键检查点必须报告
- 错误日志立即上报

---

## ⚙️ 通用化占位符说明

本目录所有workflow使用占位符表示可变信息：

| 占位符 | 含义 | 示例 |
|--------|------|------|
| `[ESP-IDF路径]` | ESP-IDF安装目录 | `C:\Espressif\frameworks\esp-idf-v5.5.3` |
| `[项目路径]` | 项目根目录 | `c:\Downloads\code-v2\v2` |
| `[串口端口]` | 设备COM端口 | `COM5`, `/dev/ttyUSB0` |
| `[项目名]` | 项目二进制文件名 | `yonovate-v2` |

---

## 🔄 维护指南

### 修改workflow时:
1. 修改`.windsurf/workflows/*.md`
2. **同步更新** `.windsurf/rules/`中的对应规则
3. 确保占位符格式一致
4. 测试workflow可执行性

### 新增workflow时:
1. 创建新的`.md`文件（带YAML frontmatter）
2. 在本文档中添加说明
3. 同步更新`.windsurf/rules/`中的相关规则
4. 确保内容通用化（无具体路径/IP）

---

## 📚 相关文件

- **规则入口**: `../README.md`
- **核心规则**: `../rules/CORE_RULES.md`
- **技术规范**: `../rules/ESP32_STANDARDS.md`
- **操作手册**: `../rules/WORKFLOW_GUIDE.md`
- **问题排查**: `../rules/TROUBLESHOOTING.md`

---

*版本: 1.0 | 更新: 2026-04-13 | 适用于: Windsurf IDE + ESP32项目*
