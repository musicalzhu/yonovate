# AI案例检索索引 - 违规案例触发器

> **用途**：AI在执行操作前自动检索相关历史案例，避免重复犯错
> **机制**：关键词匹配 → 自动提示 → 必须查阅

---

## 🚨 强制执行机制

### 触发规则
```markdown
当用户输入匹配以下场景时，AI必须：
1. 先查阅本索引中的相关案例
2. 在回复中声明"已查阅相关案例"
3. 按案例教训执行操作
```

**禁止行为**：
- ❌ 不查阅案例直接凭记忆执行
- ❌ 查阅案例但不声明
- ❌ 明知有案例仍重复违规

---

## 📋 案例分类索引

### 场景1：ESP32编译/烧录
**触发关键词**：
- "设备接入"
- "COM端口"
- "烧录"
- "flash"
- "编译"
- "idf.py"

**强制查阅案例**：
- `CR_HISTORY.md` 第8-82行：AI违规案例记录-2026-04-14

**案例教训**：
1. 禁止询问，自动执行
2. 必须使用`cmd.exe /c`格式
3. 先阅读`esp32-build.md`

**AI必须声明**：
```markdown
已查阅CR_HISTORY.md案例-2026-04-14，按教训执行：
✅ 自动执行，不询问
✅ 使用cmd.exe /c格式
✅ 先阅读esp32-build.md
```

---

### 场景2：代码修改后验证
**触发关键词**：
- "修复完成"
- "修改好了"
- "代码已改"
- "修复了bug"

**强制查阅案例**：
- `CR_HISTORY.md` 第99-118行：AI反思与规则完善案例

**案例教训**：
1. 修改后必须立即编译
2. 禁止说"修复完成"后跳过编译

**AI必须声明**：
```markdown
已查阅编译验证案例，按规则执行修改后验证机制
```

---

### 场景3：Git操作
**触发关键词**：
- "提交"
- "commit"
- "git"
- "push"
- "分支"

**强制查阅案例**：
- `TROUBLESHOOTING.md` 第45-78行：编码问题与Git故障

**案例教训**：
1. 必须使用cmd.exe执行git
2. 提交后必须`git status`验证
3. 禁止说"已提交"但未验证

**AI必须声明**：
```markdown
已查阅Git操作案例，执行验证后声明
```

---

### 场景4：Web页面修改
**触发关键词**：
- "html"
- "前端"
- "web页面"
- "index.html"

**强制查阅案例**：
- `CORE_RULES.md` 第140-165行：Web页面版本号违规案例

**案例教训**：
1. 必须同时修改版本号
2. 格式：v1.x → v1.(x+1)

**AI必须声明**：
```markdown
已查阅Web页面版本号案例，同步更新版本号
```

---

## 🔍 AI自动检索流程

```markdown
用户输入 → 关键词匹配 → 检索相关案例 → 声明已查阅 → 按案例执行
```

### 检索步骤
```python
# AI内部逻辑
def auto_check_case(user_input):
    keywords = extract_keywords(user_input)
    
    for scenario in CASE_INDEX:
        if match_keywords(keywords, scenario.triggers):
            case_file = scenario.case_file
            case_location = scenario.location
            
            # 强制操作
            read_file(case_file, case_location)  # 必须查阅
            declare(f"已查阅{case_file}案例")      # 必须声明
            apply_lessons(scenario.lessons)        # 按教训执行
            
            return True
    
    return False
```

---

## ✅ 案例查阅检查清单

AI回复前必须自问：
```markdown
□ 本次操作是否有历史违规案例？（查阅CASE_INDEX.md）
□ 是否已阅读相关案例文件？
□ 是否在回复中声明"已查阅案例"？
□ 是否按案例教训执行？
□ 是否避免重复相同错误？
```

---

## 📊 案例库统计

| 案例编号 | 日期 | 场景 | 状态 | 规则强化 |
|---------|------|------|------|---------|
| CASE-2026-04-14 | 2026-04-14 | ESP32编译烧录 | ✅ 已修复 | CORE_RULES.md+3处 |
| CASE-2026-04-13 | 2026-04-13 | Web版本号 | ✅ 已修复 | CORE_RULES.md+WORKFLOW_GUIDE.md |

---

## 🎯 效果验证

**验证方式**：
1. 用户输入触发关键词
2. AI自动查阅案例并声明
3. 按正确流程执行
4. 不再重复历史错误

**成功标准**：
- AI主动声明"已查阅XX案例"
- 操作符合案例教训
- 用户无需纠正相同问题

---

*版本：1.0 | 创建日期：2026-04-14 | 作用：AI案例自动检索触发*
