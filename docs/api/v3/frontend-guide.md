# 状态机 V3.0 API 前端适配指南

**文档版本**: v1.1  
**更新日期**: 2026-04-29  
**适用范围**: 前端/客户端开发者  

---

## 1. 快速概览

V3 状态机采用**表驱动架构**，支持事件触发、状态效果和 GPIO 控制，专为 STEM 教学设计。

```json
{
  "initialState": "STOP",
  "groupModes": {"0": 1, "1": 1, "2": 0, "3": 0, "4": 1},
  "inputModuleTypes": {"0": 0, "1": 0, "4": 0},
  "edgeTypes": {"0": 1, "1": 0, "4": 0},
  "outputModuleTypes": {"2": 3},
  "transitions": [
    {
      "currentState": "STOP",
      "nextState": "ACTIVE",
      "triggerLogic": "AND",
      "triggers": [
        {"triggerType": 0, "triggerSource": "P0", "operator": 0, "triggerValue": 1},
        {"triggerType": 1, "triggerSource": "P4", "operator": 0, "triggerValue": 1}
      ]
    }
  ],
  "stateEffects": [
    {"stateName": "ACTIVE", "effectType": 1, "target": "P2", "value": 1}
  ]
}
```

---

## 2. 核心字段速查表

| JSON 字段 | 数据类型 | 必填 | 说明 |
|-----------|----------|------|------|
| `initialState` | string | 是 | 状态机启动时的初始状态名 |
| `groupModes` | object | 是 | GPIO 组模式配置，`0`=输出，`1`=输入 |
| `inputModuleTypes` | object | 条件 | 输入模块类型（输入组必填） |
| `edgeTypes` | object | 条件 | 边沿检测类型（输入组必填） |
| `outputModuleTypes` | object | 条件 | 输出模块类型（输出组必填） |
| `transitions` | array | 是 | 状态转换规则数组 |
| `stateEffects` | array | 否 | 状态进入时自动执行的效果 |

---

## 3. 触发器字段详解

### 3.1 triggerType（触发器类型）

| 值 | 枚举 | 说明 | 示例 |
|---|------|------|------|
| 0 | `SM_TRIGGER_EDGE` | 边沿触发 | 按钮按下/释放 |
| 1 | `SM_TRIGGER_LEVEL` | 电平触发 | 传感器持续高/低电平 |
| 2 | `SM_TRIGGER_TIMER` | 定时器触发 | 延时动作 |
| 3 | `SM_TRIGGER_MANUAL` | 手动触发 | API 调用 |

```json
// 边沿触发示例：按钮按下检测
{"triggerType": 0, "triggerSource": "P0", "operator": 0, "triggerValue": 1}

// 定时器触发示例：2秒后触发
{"triggerType": 2, "triggerSource": "T0", "operator": 0, "triggerValue": 2000}
```

### 3.2 triggerSource（触发源）

| 格式 | 含义 | 示例 |
|-----|------|------|
| `P0` ~ `P11` | GPIO 组 0-11（输入） | `"P0"` = GPIO 组 0 |
| `T0` ~ `T2` | 定时器 0-2 | `"T0"` = 定时器 0 |
| `M0` ~ `M1` | 电机组（用于效果） | `"M0"` = 电机 0 |

**注意**: V3 使用 `P0` 格式（带 P 前缀），不再支持纯数字 `"0"`。

### 3.3 operator（比较运算符）

| 值 | 枚举 | 符号 | 说明 |
|---|------|------|------|
| 0 | `SM_OP_EQ` | `==` | 等于 |
| 1 | `SM_OP_LT` | `<` | 小于 |
| 2 | `SM_OP_GT` | `>` | 大于 |
| 3 | `SM_OP_LE` | `<=` | 小于等于 |
| 4 | `SM_OP_GE` | `>=` | 大于等于 |
| 5 | `SM_OP_NE` | `!=` | 不等于 |

```json
// 电平触发示例：GPIO 组 1 高电平
{"triggerType": 1, "triggerSource": "P1", "operator": 0, "triggerValue": 1}

// 比较触发示例：电压低于阈值
{"triggerType": 1, "triggerSource": "P4", "operator": 1, "triggerValue": 500}
```

### 3.4 triggerValue（触发值）

| 触发器类型 | 值范围 | 说明 |
|-----------|-------|------|
| `EDGE` | 0 或 1 | 边沿检测（通常设为 1） |
| `LEVEL` | 0 或 1 | 电平比较（0=低电平, 1=高电平） |
| `TIMER` | 毫秒 | 延时时间（如 2000 = 2秒） |
| `MANUAL` | 忽略 | 手动触发时此值无效 |

---

## 4. 多触发器与逻辑关系

### 4.1 AND 逻辑（默认）

所有触发器条件必须同时满足：

```json
{
  "currentState": "STOP",
  "nextState": "MOVING_UP",
  "triggerLogic": "AND",
  "triggers": [
    {"triggerType": 0, "triggerSource": "P0", "operator": 0, "triggerValue": 1},
    {"triggerType": 1, "triggerSource": "P4", "operator": 0, "triggerValue": 1}
  ]
}
// 解读：按钮按下 AND 门关闭 才能启动
```

### 4.2 OR 逻辑

任一触发器条件满足即可：

```json
{
  "currentState": "IDLE",
  "nextState": "ACTIVE",
  "triggerLogic": "OR",
  "triggers": [
    {"triggerType": 0, "triggerSource": "P1", "operator": 0, "triggerValue": 1},
    {"triggerType": 0, "triggerSource": "P2", "operator": 0, "triggerValue": 1}
  ]
}
// 解读：左手按钮 OR 右手按钮 按下都能激活
```

---

## 5. 集中式 GPIO 配置（V3 新增）

V3 将 GPIO 组模式、边沿类型、模块类型从分散配置改为集中配置。

### 5.1 groupModes（GPIO 组模式）

```json
{
  "groupModes": {
    "0": 1,  // 组 0: 输入模式
    "1": 1,  // 组 1: 输入模式
    "2": 0,  // 组 2: 输出模式
    "3": 0   // 组 3: 输出模式
  }
}
```

| 值 | 模式 |
|---|------|
| 0 | 输出模式 |
| 1 | 输入模式 |

### 5.2 inputModuleTypes（输入模块类型）

仅对输入模式组（groupModes=1）有效：

```json
{
  "inputModuleTypes": {
    "0": 0,  // 组 0: 普通按钮
    "1": 0   // 组 1: 普通按钮
  }
}
```

| 值 | 枚举 | 说明 |
|---|------|------|
| 0 | `SM_INPUT_MODULE_NORMAL_BUTTON` | 普通按钮 |
| 1 | `SM_INPUT_MODULE_REED_SWITCH` | 干簧管开关 |
| 2 | `SM_INPUT_MODULE_LIMIT_SWITCH` | 限位开关 |
| 3 | `SM_INPUT_MODULE_HUMIDITY_SENSOR` | 温湿度传感器 |
| 4 | `SM_INPUT_MODULE_VOLTAGE_MONITOR` | 电压监测 |

### 5.3 edgeTypes（边沿检测类型）

仅对输入模式组有效：

```json
{
  "edgeTypes": {
    "0": 1,  // 组 0: 上升沿
    "1": 2   // 组 1: 下降沿
  }
}
```

| 值 | 枚举 | 说明 |
|---|------|------|
| 0 | `SM_EDGE_ANY` | 任意边沿（上升或下降） |
| 1 | `SM_EDGE_RISING` | 上升沿 |
| 2 | `SM_EDGE_FALLING` | 下降沿 |

### 5.4 outputModuleTypes（输出模块类型）

仅对输出模式组（groupModes=0）有效：

```json
{
  "outputModuleTypes": {
    "2": 0,  // 组 2: LED
    "3": 3   // 组 3: 电机
  }
}
```

| 值 | 枚举 | 说明 |
|---|------|------|
| 0 | `SM_OUTPUT_MODULE_LED` | LED 灯 |
| 1 | `SM_OUTPUT_MODULE_RELAY` | 继电器 |
| 2 | `SM_OUTPUT_MODULE_BUZZER` | 蜂鸣器 |
| 3 | `SM_OUTPUT_MODULE_MOTOR` | 电机 |
| 4 | `SM_OUTPUT_MODULE_VALVE` | 电磁阀 |

---

## 6. 状态效果（stateEffects）

进入状态时自动执行的动作：

```json
{
  "stateEffects": [
    {
      "stateName": "ACTIVE",
      "effectType": 1,
      "target": "P2",
      "value": 1
    },
    {
      "stateName": "RUNNING",
      "effectType": 2,
      "target": "T0",
      "value": 5000
    },
    {
      "stateName": "MOVING_UP",
      "effectType": 3,
      "target": "M0",
      "value": 5
    }
  ]
}
```

### 6.1 effectType（效果类型）

| 值 | 枚举 | 说明 | target 格式 | value 含义 |
|---|------|------|------------|-----------|
| 0 | `SM_EFFECT_NONE` | 无效果 | - | - |
| 1 | `SM_EFFECT_OUTPUT` | 输出控制 | `P0`~`P11` | 0=关, 1=开 |
| 2 | `SM_EFFECT_TIMER` | 启动定时器 | `T0`~`T2` | 毫秒数 |
| 3 | `SM_EFFECT_MOTOR` | 电机控制 | `M0`~`M1` | -9~9（档位） |
| 4 | `SM_EFFECT_NOTIFICATION` | 通知 | 忽略 | 忽略 |

### 6.2 常见效果示例

```json
// LED 点亮效果
{"stateName": "ACTIVE", "effectType": 1, "target": "P2", "value": 1}

// 启动 5 秒定时器
{"stateName": "DELAY", "effectType": 2, "target": "T0", "value": 5000}

// 电机正转 5 档
{"stateName": "MOVING_UP", "effectType": 3, "target": "M0", "value": 5}

// 电机停止
{"stateName": "STOP", "effectType": 3, "target": "M0", "value": 0}
```

---

## 7. 课件示例

### 7.1 课件1：双向安全控制（门关闭条件）

**教学目标**：
- AND 触发器逻辑（按钮 + 门关闭）
- 双向控制
- 限位器硬件保护

**状态转换表**：

| 当前状态 | 触发条件 | 下一状态 | 效果 |
|----------|----------|----------|------|
| STOP | p0上升沿 AND p4高电平 | MOVING_UP | M1正转 |
| STOP | p2上升沿 AND p4高电平 | MOVING_DOWN | M1反转 |
| MOVING_UP | p1高电平 | STOP | M1停止 |
| MOVING_DOWN | p3高电平 | STOP | M1停止 |

**完整配置**：

```json
{
  "initialState": "STOP",
  "groupModes": {"0": 1, "1": 1, "2": 1, "3": 1, "4": 1},
  "inputModuleTypes": {"0": 0, "1": 0, "2": 0, "3": 0, "4": 0},
  "edgeTypes": {"0": 1, "1": 0, "2": 2, "3": 0, "4": 0},
  "outputModuleTypes": {"5": 3},
  "transitions": [
    {
      "currentState": "STOP",
      "nextState": "MOVING_UP",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 0, "triggerSource": "P0", "operator": 0, "triggerValue": 1},
        {"triggerType": 1, "triggerSource": "P4", "operator": 0, "triggerValue": 1}
      ]
    },
    {
      "currentState": "STOP",
      "nextState": "MOVING_DOWN",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 0, "triggerSource": "P2", "operator": 0, "triggerValue": 1},
        {"triggerType": 1, "triggerSource": "P4", "operator": 0, "triggerValue": 1}
      ]
    },
    {
      "currentState": "MOVING_UP",
      "nextState": "STOP",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 1, "triggerSource": "P1", "operator": 0, "triggerValue": 1}
      ]
    },
    {
      "currentState": "MOVING_DOWN",
      "nextState": "STOP",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 1, "triggerSource": "P3", "operator": 0, "triggerValue": 1}
      ]
    }
  ],
  "stateEffects": [
    {"stateName": "MOVING_UP", "effectType": 3, "target": "M1", "value": 5},
    {"stateName": "MOVING_DOWN", "effectType": 3, "target": "M1", "value": -5},
    {"stateName": "STOP", "effectType": 3, "target": "M1", "value": 0}
  ]
}
```

**硬件连接**：

| 端口 | 类型 | 功能 |
|------|------|------|
| p0 | 输入-上升沿 | 上升请求按钮 |
| p2 | 输入-上升沿 | 下降请求按钮 |
| p1 | 输入-高电平 | 上升限位器（顶部） |
| p3 | 输入-高电平 | 下降限位器（底部） |
| p4 | 输入-高电平 | 门关传感器 |
| M1 | 输出-电机 | 电梯电机 |

---

### 7.2 课件2：延时等待控制（2秒等待 + 门检测 + 错误处理）

**教学目标**：
- 定时器AND条件（时间 + 门状态同时满足）
- 状态按方向拆分（UP_WAITING / DOWN_WAITING）
- 错误处理与自动恢复

**状态转换表**：

| 当前状态 | 触发条件 | 下一状态 | 效果 |
|----------|----------|----------|------|
| STOP | p0上升沿 | UP_WAITING | T0启动(2秒) |
| STOP | p2上升沿 | DOWN_WAITING | T0启动(2秒) |
| **UP_WAITING** | **T0到期** AND **p4高电平** | **MOVING_UP** | **门关闭，立即启动** |
| **DOWN_WAITING** | **T0到期** AND **p4高电平** | **MOVING_DOWN** | **门关闭，立即启动** |
| **UP_WAITING** | **T0到期** AND **p4低电平** | **UP_ERROR** | **门未关闭，错误** |
| **DOWN_WAITING** | **T0到期** AND **p4低电平** | **DOWN_ERROR** | **门未关闭，错误** |
| **UP_ERROR** | **p4高电平** | **MOVING_UP** | **门关闭，立即启动** |
| **DOWN_ERROR** | **p4高电平** | **MOVING_DOWN** | **门关闭，立即启动** |
| **UP_ERROR** | **T1到期** | **STOP** | **超时，复位停止** |
| **DOWN_ERROR** | **T1到期** | **STOP** | **超时，复位停止** |
| MOVING_UP | p1高电平 | STOP | M1停止 |
| MOVING_DOWN | p3高电平 | STOP | M1停止 |

**完整配置**：

```json
{
  "initialState": "STOP",
  "groupModes": {"0": 1, "1": 1, "2": 1, "3": 1, "4": 1},
  "inputModuleTypes": {"0": 0, "1": 0, "2": 0, "3": 0, "4": 0},
  "edgeTypes": {"0": 1, "1": 0, "2": 2, "3": 0, "4": 0},
  "outputModuleTypes": {"5": 3},
  "transitions": [
    {
      "currentState": "STOP",
      "nextState": "UP_WAITING",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 0, "triggerSource": "P0", "operator": 0, "triggerValue": 1}
      ]
    },
    {
      "currentState": "STOP",
      "nextState": "DOWN_WAITING",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 0, "triggerSource": "P2", "operator": 0, "triggerValue": 1}
      ]
    },
    {
      "currentState": "UP_WAITING",
      "nextState": "MOVING_UP",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 2, "triggerSource": "T0", "operator": 0, "triggerValue": 0},
        {"triggerType": 1, "triggerSource": "P4", "operator": 0, "triggerValue": 1}
      ]
    },
    {
      "currentState": "DOWN_WAITING",
      "nextState": "MOVING_DOWN",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 2, "triggerSource": "T0", "operator": 0, "triggerValue": 0},
        {"triggerType": 1, "triggerSource": "P4", "operator": 0, "triggerValue": 1}
      ]
    },
    {
      "currentState": "UP_WAITING",
      "nextState": "UP_ERROR",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 2, "triggerSource": "T0", "operator": 0, "triggerValue": 0},
        {"triggerType": 1, "triggerSource": "P4", "operator": 5, "triggerValue": 1}
      ]
    },
    {
      "currentState": "DOWN_WAITING",
      "nextState": "DOWN_ERROR",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 2, "triggerSource": "T0", "operator": 0, "triggerValue": 0},
        {"triggerType": 1, "triggerSource": "P4", "operator": 5, "triggerValue": 1}
      ]
    },
    {
      "currentState": "UP_ERROR",
      "nextState": "MOVING_UP",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 1, "triggerSource": "P4", "operator": 0, "triggerValue": 1}
      ]
    },
    {
      "currentState": "DOWN_ERROR",
      "nextState": "MOVING_DOWN",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 1, "triggerSource": "P4", "operator": 0, "triggerValue": 1}
      ]
    },
    {
      "currentState": "UP_ERROR",
      "nextState": "STOP",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 2, "triggerSource": "T1", "operator": 0, "triggerValue": 0}
      ]
    },
    {
      "currentState": "DOWN_ERROR",
      "nextState": "STOP",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 2, "triggerSource": "T1", "operator": 0, "triggerValue": 0}
      ]
    },
    {
      "currentState": "MOVING_UP",
      "nextState": "STOP",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 1, "triggerSource": "P1", "operator": 0, "triggerValue": 1}
      ]
    },
    {
      "currentState": "MOVING_DOWN",
      "nextState": "STOP",
      "triggerLogic": 0,
      "triggers": [
        {"triggerType": 1, "triggerSource": "P3", "operator": 0, "triggerValue": 1}
      ]
    }
  ],
  "stateEffects": [
    {"stateName": "UP_WAITING", "effectType": 2, "target": "T0", "value": 2000},
    {"stateName": "DOWN_WAITING", "effectType": 2, "target": "T0", "value": 2000},
    {"stateName": "UP_ERROR", "effectType": 2, "target": "T1", "value": 5000},
    {"stateName": "DOWN_ERROR", "effectType": 2, "target": "T1", "value": 5000},
    {"stateName": "MOVING_UP", "effectType": 3, "target": "M1", "value": 5},
    {"stateName": "MOVING_DOWN", "effectType": 3, "target": "M1", "value": -5},
    {"stateName": "STOP", "effectType": 3, "target": "M1", "value": 0}
  ]
}
```

**设计说明**：
- **2秒等待**：按钮按下后必须等待2秒（防误触）
- **门检测**：2秒到期时检查门是否关闭
- **容错恢复**：ERROR状态期间若门关闭，可立即启动（无需重新等待）
- **超时保护**：ERROR状态持续5秒后自动复位到STOP
- **方向拆分**：UP_WAITING/DOWN_WAITING 分别对应上升/下降请求

**硬件连接**：

| 端口 | 类型 | 功能 |
|------|------|------|
| p0 | 输入-上升沿 | 上升请求按钮 |
| p2 | 输入-下降沿 | 下降请求按钮 |
| p1 | 输入-高电平 | 上升限位器（顶部） |
| p3 | 输入-高电平 | 下降限位器（底部） |
| p4 | 输入-高电平 | 门关传感器 |
| M1 | 输出-电机 | 电梯电机 |
| LED | 输出-LED | 状态指示灯（UP慢闪/DOWN慢闪/ERROR常亮） |

---

## 8. HTTP API 端点

### 8.1 加载配置

```http
POST /api/v3/state-machine
Content-Type: application/json

{ /* 完整配置 JSON */ }
```

**响应**:
```json
{
  "success": true,
  "message": "配置加载成功",
  "currentState": "STOP",
  "version": "3.0"
}
```

### 8.2 获取当前配置

```http
GET /api/v3/state-machine
```

**响应**: 当前完整配置 JSON

### 8.3 停止状态机

```http
DELETE /api/v3/state-machine
```

---

## 9. 常见问题（FAQ）

### Q1: 触发器字段名有哪些？

**A**: V3 使用以下字段名（必须使用完整格式）：

```json
{"triggerType": 1, "triggerSource": "P0", "operator": 0, "triggerValue": 1}
```

- `triggerType`: 触发器类型（0=边沿, 1=电平, 2=定时器, 3=手动）
- `triggerSource`: 触发源（`P0`~`P11`, `T0`~`T2` 等）
- `operator`: 比较运算符（0=`==`, 1=`<`, 2=`>`, 3=`<=`, 4=`>=`, 5=`!=`）
- `triggerValue`: 触发比较值（边沿/电平用 0/1，定时器用毫秒）

### Q2: 为什么配置加载后状态机没反应？

**A**: 检查以下几点：
1. `triggerSource` 是否使用 `P0` 格式（不是纯数字）
2. `groupModes` 是否正确配置（输入=1, 输出=0）
3. `edgeTypes` 是否与触发器类型匹配

### Q3: 定时器触发器如何工作？

**A**: 需要两步：
1. 在 `stateEffects` 中使用 `effectType: 2` 启动定时器
2. 在 `transitions` 中使用 `triggerType: 2` 检测定时器到期

```json
// 状态效果：启动 T0 定时器（3秒）
{"stateName": "WAITING", "effectType": 2, "target": "T0", "value": 3000}

// 转换触发器：T0 到期
{"triggerType": 2, "triggerSource": "T0", "operator": 0, "triggerValue": 3000}
```

### Q4: 如何调试配置问题？

**A**: 查看设备日志（ESP32 串口输出）：
- `SM_V3_JSON` 标签：配置解析日志
- `SM_CORE` 标签：状态转换日志
- `SM_GPIO` 标签：GPIO 触发检测日志

---

## 10. 版本历史

| 版本 | 日期 | 说明 |
|------|------|------|
| v1.1 | 2026-04-29 | 激进清理，移除 V2 兼容内容，仅保留课件 1 和课件 2 示例 |
| v1.0 | 2026-04-29 | 初始版本，适配 V3 固件字段名 |

---

**文档维护者**: V3 开发团队  
**问题反馈**: 请提交至项目 Issue 页面
