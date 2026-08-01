# V3 状态机规范

## 概述

ESP32 有限状态机 V3.0，支持事件触发、状态效果和 GPIO 控制。

**设计目标**：为 STEM 教学提供简单、可理解的状态机架构，便于学生理解嵌入式系统核心概念。

**核心特性**：
- 表驱动：状态转换规则通过UI界面配置表定义，无需修改代码
- 多触发器组合：支持最多 3 个触发器的 AND/OR 逻辑
- 状态绑定效果：进入状态时自动执行绑定效果
- 集中式 GPIO 配置：边沿类型、输入/输出模块类型统一配置

---

## 架构决策

| 决策 | 说明 | 理由 |
|------|------|------|
| 单一全局实例 | `g_sm` 全局运行时状态 | 简化架构，明确与 V2 不兼容 |
| 配置与运行时分离 | `sm_config_t` 纯配置，`g_sm` 运行时状态 | 配置可静态分配，运行时动态管理 |
| 状态绑定效果 | 效果绑定到目标状态 | 相同状态可从多路径进入，效果统一执行 |
| 集中式 GPIO 配置 | GPIO边沿类型、输入/输出模块类型从 `sm_config_t` 获取 | 简化触发器和效果结构，统一配置入口 |
| 定时器统一状态 | SM_EFFECT_TIMER 启动，SM_TRIGGER_TIMER 检测 | 支持"进入状态后延时转换"场景 |

---

## 核心组件

### 触发系统

#### 触发器类型

**触发器类型枚举** (`sm_trigger_type_t`)：

| 枚举值 | 数值 | 说明 | 适用 source 格式 |
|--------|------|------|-----------------|
| `SM_TRIGGER_NONE` | 0 | 无自动触发 | - |
| `SM_TRIGGER_EDGE` | 1 | 边沿触发 - 检测 GPIO 电平变化 | "P0"-"P11" |
| `SM_TRIGGER_LEVEL` | 2 | 电平触发 - 传感器阈值比较 | "P0"-"P11" |
| `SM_TRIGGER_TIMER` | 3 | 定时器触发 - 延时到期检测 | "T0"-"T2" |
| `SM_TRIGGER_MANUAL` | 4 | 手动触发 - API/HTTP 调用触发 | "MANUAL" |


#### 边沿触发 (SM_TRIGGER_EDGE)

检测 GPIO 边沿变化，支持三种边沿类型：

**边沿类型枚举** (`sm_edge_type_t`)：

| 枚举值 | 数值 | 说明 | 检测条件 |
|--------|------|------|----------|
| `SM_EDGE_ANY` | 0 | 任何边沿 | 上升沿或下降沿均可触发 |
| `SM_EDGE_RISING` | 1 | 上升沿 | 电平从 0 变为 1 时触发 |
| `SM_EDGE_FALLING` | 2 | 下降沿 | 电平从 1 变为 0 时触发 |


**实现机制**：
1. 每次 `sm_v3_process()` 轮询时，读取当前 GPIO 状态
2. 与 `g_sm.gpio_prev_state` 历史状态比较
3. 检测到边沿时返回 true，触发状态转换
4. 根据配置的 `edge_types` 判断是否为有效触发


#### 电平触发 (SM_TRIGGER_LEVEL)

根据输入模块类型读取逻辑电平，支持比较运算符：

**比较运算符枚举** (`sm_operator_t`)：

| 枚举值 | 数值 | 说明 | 数学符号 |
|--------|------|------|----------|
| `SM_OP_EQ` | 0 | 等于 | == |
| `SM_OP_LT` | 1 | 小于 | < |
| `SM_OP_GT` | 2 | 大于 | > |
| `SM_OP_LE` | 3 | 小于等于 | <= |
| `SM_OP_GE` | 4 | 大于等于 | >= |
| `SM_OP_NE` | 5 | 不等于 | != |


**输入模块类型与电平转换**：

| 模块类型 | 物理电平 | 逻辑电平 | 适用场景 |
|----------|----------|----------|----------|
| `SM_INPUT_MODULE_NORMAL_BUTTON` | 0 (按下) | 1 | 普通按钮（低电平有效） |
| `SM_INPUT_MODULE_NORMAL_BUTTON` | 1 (释放) | 0 | 普通按钮（低电平有效） |
| `SM_INPUT_MODULE_LED_BUTTON` | 1 (按下) | 1 | 带灯按钮（高电平有效） |
| `SM_INPUT_MODULE_LED_BUTTON` | 0 (释放) | 0 | 带灯按钮（高电平有效） |
| `SM_INPUT_MODULE_TEMP_SENSOR` | ADC 值 | 温度值×10 | 温度传感器 (0.1°C精度) |


#### 定时器触发 (SM_TRIGGER_TIMER)

检测定时器是否到期，用于延时触发场景。

**工作原理**：
1. 首次检查：记录启动时间，返回 false（未到期）
2. 后续检查：计算已过去时间，>= value 时返回 true


**定时器状态结构** (`sm_timer_state_t`)：

| 字段 | 类型 | 说明 |
|------|------|------|
| `start_ms` | uint32_t | 计时开始时间戳（毫秒） |
| `running` | bool | 是否正在计时，true=计时中，false=未启动或已到期 |
| `duration_ms` | uint32_t | 持续时间（毫秒），由 SM_EFFECT_TIMER 设置 |

**定时器管理器** (`sm_timer_manager_t`)：

| 字段 | 类型 | 说明 |
|------|------|------|
| `timers` | `sm_timer_state_t[SM_MAX_TIMERS]` | 定时器状态数组，索引 0/1/2 对应 T0/T1/T2 |

**重要说明**：SM_EFFECT_TIMER 启动定时器（设置 start_ms, running=true, duration_ms），SM_TRIGGER_TIMER 检测同一定时器的到期状态（比较 elapsed >= duration_ms）。

#### 手动触发 (SM_TRIGGER_MANUAL)

通过 API 或 HTTP 接口手动触发状态转换。

#### 多触发器组合

支持最多 3 个触发器的组合：

| 枚举值 | 数值 | 说明 |
|--------|------|------|
| `SM_LOGIC_AND` | 0 | 默认: 所有触发器必须满足 (AND逻辑) |
| `SM_LOGIC_OR` | 1 | 任一触发器满足即可 (OR逻辑) |

**触发器结构** (`sm_trigger_t`)：

| 字段 | 类型 | 说明 |
|------|------|------|
| `type` | sm_trigger_type_t | 触发器类型：EDGE/LEVEL/TIMER/MANUAL |
| `source` | char[32] | 触发源标识："P1"-"P12"(GPIO组)、"T0"-"T2"(定时器)、"MANUAL"(手动) |
| `op` | sm_operator_t | 比较运算符：EQ/LT/GT/LE/GE/NE（仅 LEVEL 类型使用） |
| `value` | int | 比较目标值或定时器毫秒数 |

**转换规则结构** (`sm_transition_t`)：

| 字段 | 类型 | 说明 |
|------|------|------|
| `current_state` | char[32] | 当前状态名，触发前的状态 |
| `next_state` | char[32] | 目标状态名，触发后转换到的状态 |
| `triggers[3]` | sm_trigger_t[3] | 触发器条件数组，支持最多3个触发器组合 |
| `trigger_count` | uint8_t | 实际使用的触发器数量，范围 1-3 |
| `trigger_logic` | sm_logic_t | 触发器组合逻辑：AND(默认) 或 OR |

**触发器数组索引说明**：
- `triggers[0]`: 第一个触发器（**必须**，始终存在）
- `triggers[1]`: 第二个触发器（可选）
- `triggers[2]`: 第三个触发器（可选）

**触发器 source 命名约定**：
- `P0`, `P1` ... `P11`: GPIO Group（支持 EDGE/LEVEL 触发类型）
- `T0`, `T1`, `T2`: Timer（支持 TIMER 触发类型）
- `MANUAL`: 手动触发（支持 MANUAL 触发类型）

**效果执行机制**：
- 效果通过目标状态名 (`next_state`) 在 `sm_config_t.effects[]` 中查找
- 进入状态时，遍历并执行所有 `state_name` 匹配的效果

---

### 状态效果

#### 效果类型

**效果类型枚举** (`sm_effect_type_t`)：

| 枚举值 | 数值 | 说明 | target 格式 | value 含义 |
|--------|------|------|-------------|------------|
| `SM_EFFECT_NONE` | 0 | 无效果 | - | - |
| `SM_EFFECT_OUTPUT` | 1 | GPIO 输出控制 | "P0"-"P11" | 输出电平值 |
| `SM_EFFECT_TIMER` | 2 | 启动效果定时器 | "T0"-"T2" | 持续时间（毫秒，如 2000 表示 2000 毫秒） |
| `SM_EFFECT_MOTOR` | 3 | 电机控制 | "M0", "M1"... | 档位 (-9~9) |
| `SM_EFFECT_NOTIFICATION` | 4 | 日志通知 | - | 未使用 |


#### 状态绑定机制

效果绑定到**目标状态名**，而非转换索引：

**状态效果结构** (`sm_state_effect_t`)：

| 字段 | 类型 | 说明 |
|------|------|------|
| `state_name` | char[32] | 绑定到的状态名，进入该状态时执行此效果 |
| `effect_type` | sm_effect_type_t | 效果类型：OUTPUT/TIMER/MOTOR/NOTIFICATION |
| `target` | char[32] | 效果目标：GPIO组ID("P0")、定时器ID("T0")、电机ID("M0") |
| `value` | int | 效果值：输出电平值、定时器毫秒数、电机档位(-9~9) |


**执行时机**：
1. 状态转换成功时，新状态名确定
2. 遍历所有 effects，查找 `state_name` 匹配项
3. 对每个匹配的调用效果


#### GPIO组输出效果 (SM_EFFECT_OUTPUT)

根据GPIO组的输出模块类型解释 value 值

#### 定时器效果 (SM_EFFECT_TIMER)

启动效果定时器（供后续 SM_TRIGGER_TIMER 检测到期）

**设计说明**：
- 效果定时器启动时设置持续时间
- 后续 SM_TRIGGER_TIMER 检测同一定时器的到期状态


#### 电机效果 (SM_EFFECT_MOTOR)

控制电机速度和方向：

| value | 含义 |
|-------|------|
| 0 | 停止 |
| 1-9 | 正转（档位 1-9） |
| -1~-9 | 反转（档位 1-9） |


---

### GPIO

#### 输入模块类型

**输入模块类型枚举** (`sm_input_module_type_t`)：

| 枚举值 | 数值 | 说明 | 物理电平→逻辑电平转换 |
|--------|------|------|----------------------|
| `SM_INPUT_MODULE_NORMAL_BUTTON` | 0 | 普通按钮（低电平有效） | 0→1, 1→0 |
| `SM_INPUT_MODULE_LED_BUTTON` | 1 | 带灯按钮（高电平有效） | 1→1, 0→0 |
| `SM_INPUT_MODULE_TEMP_SENSOR` | 2 | 温度传感器 | ADC值→温度值×10 |
| `SM_INPUT_MODULE_HUMIDITY_SENSOR` | 3 | 湿度传感器（预留） | - |
| `SM_INPUT_MODULE_VOLTAGE_MONITOR` | 4 | 电压监测（预留） | - |


#### 输出模块类型

**输出模块类型枚举** (`sm_output_module_type_t`)：

| 枚举值 | 数值 | 说明 | value 值含义 |
|--------|------|------|-------------|
| `SM_OUTPUT_LED` | 0 | LED灯 | 0=关闭, 1=点亮 |
| `SM_OUTPUT_TRAFFIC_LIGHT` | 1 | 交通灯 | 0=灭, 1=红, 2=黄, 3=绿 |
| `SM_OUTPUT_FAN` | 2 | 风扇 | 0=关闭, 非0=开启 |


#### 集中式 GPIO 配置

所有 GPIO 相关配置集中在 `sm_config_t`：

**状态机配置结构** (`sm_config_t`)：

| 字段 | 类型 | 说明 |
|------|------|------|
| `initial_state` | char[32] | 启动时的初始状态名 |
| `transitions` | sm_transition_t[32] | 状态转换规则数组 |
| `transition_count` | uint32_t | 实际配置的转换规则数量 |
| `effects` | sm_state_effect_t[32] | 状态效果绑定数组 |
| `effect_count` | uint32_t | 实际配置的效果数量 |
| `group_modes` | uint8_t[12] | GPIO 组模式：0=输出模式, 1=输入模式 |
| `input_module_types` | sm_input_module_type_t[12] | 输入模块类型数组 |
| `edge_types` | sm_edge_type_t[12] | 边沿检测类型数组 |
| `output_module_types` | sm_output_module_type_t[12] | 输出模块类型数组 |

---

### 硬件接口定义

ESP32 STEM 课件硬件引脚约定：

#### 原生 GPIO 组 (0-3)

| 组 ID | GPIO 引脚 | 用途 |
|-------|-----------|------|
| Group 0 | GPIO13, GPIO12 | 通用 I/O |
| Group 1 | GPIO15, GPIO2 | 通用 I/O |
| Group 2 | GPIO26, GPIO25 | 通用 I/O |
| Group 3 | GPIO27, GPIO14 | 通用 I/O |

#### AW9523 扩展组 (4-11)

| 组 ID | 扩展标识 | 控制方式 | 用途 |
|-------|----------|----------|------|
| Group 4 | E1 | I2C (AW9523) | 扩展 I/O |
| Group 5 | E2 | I2C (AW9523) | 扩展 I/O |
| Group 6 | E3 | I2C (AW9523) | 扩展 I/O |
| Group 7 | E4 | I2C (AW9523) | 扩展 I/O |
| Group 8 | E5 | I2C (AW9523) | 扩展 I/O |
| Group 9 | E6 | I2C (AW9523) | 扩展 I/O |
| Group 10 | E7 | I2C (AW9523) | 扩展 I/O |
| Group 11 | E8 | I2C (AW9523) | 扩展 I/O |

**AW9523 说明**：通过 I2C 总线控制的 GPIO 扩展芯片，支持 8 组扩展引脚。

#### 电机控制接口

| 电机 ID | DIR 引脚 | PWM 引脚 | 控制函数 |
|---------|----------|----------|----------|
| Motor 0 | GPIO18 | GPIO19 | `sm_gpio_set_motor("M0", speed)` |
| Motor 1 | GPIO23 | GPIO16 | `sm_gpio_set_motor("M1", speed)` |

**电机档位映射**：
- `-9 ~ -1`: 反转（9档最高速）
- `0`: 停止
- `1 ~ 9`: 正转（9档最高速）

#### 系统按键

| 按键 | GPIO 引脚 | 用途 |
|------|-----------|------|
| Button A | GPIO34 | 系统按键 A（仅输入） |
| Button B | GPIO35 | 系统按键 B（仅输入） |

#### 电压检测

| 功能 | GPIO 引脚 | ADC 通道 | 用途 |
|------|-----------|----------|------|
| 电压检测 | GPIO36 | ADC1_CH0 | 电池/系统电压监测 |

**ADC 说明**：使用 ESP32 ADC1 通道 0，可通过 `adc1_get_raw()` 读取原始值并转换为电压。

---

### 定时器管理

#### 定时器统一状态设计

每个定时器 ID (T0, T1, T2) 有统一的状态，支持"效果启动，触发器检测"模式：

- SM_EFFECT_TIMER 启动定时器（设置 start_ms, running=true, duration_ms）
- SM_TRIGGER_TIMER 检测到期（比较 elapsed >= duration_ms）

#### 运行时状态

**全局运行时状态** (`g_sm`)：

| 字段 | 类型 | 说明 |
|------|------|------|
| `config` | sm_config_t | 配置快照，初始化时从传入配置复制 |
| `current_state` | char[32] | 当前状态名，状态机运行时的当前状态 |
| `is_running` | bool | 运行标志，true=运行中，false=已停止 |
| `timer_mgr` | sm_timer_manager_t | 定时器状态管理器 |
| `active_timer_count` | uint8_t | 活跃定时器计数，用于监控和调试 |
| `gpio_prev_state` | uint8_t[12] | GPIO 历史状态，用于边沿检测比较 |

---

### 核心引擎

#### 生命周期 API

| 函数 | 参数 | 返回值 | 说明 |
|------|------|--------|------|
| `sm_v3_init` | `config`: 状态机配置指针 | `ESP_OK`: 成功<br>`ESP_ERR_INVALID_ARG`: 配置为空 | 初始化状态机，复制配置到内部存储 |
| `sm_v3_start` | 无 | `ESP_OK`: 成功<br>`ESP_ERR_INVALID_STATE`: 未初始化 | 启动状态机，设置运行标志，开始处理状态转换 |
| `sm_v3_stop` | 无 | 无 | 暂停运行（`is_running = false`），保留当前状态，可继续 |
| `sm_v3_process` | 无 | 无 | 处理一次状态检查（在主循环或定时器回调中调用） |
| `sm_v3_reset` | 无 | 无 | 停止运行 + 重置到初始状态 + 清空所有运行时状态（定时器、GPIO历史、边沿检测标志） |


#### 主循环流程

`sm_v3_process()` 工作流程：

1. **检查运行状态** - 未运行则直接返回
2. **匹配规则** - 遍历转换规则，寻找 `current_state` 匹配项
3. **检查触发器** - 调用 `check_triggers()` 验证条件
4. **执行转换** - 若触发满足：
   - 更新状态到 `next_state`
   - 执行新状态的绑定效果
   - 记录转换日志
5. **单周期单转换** - 每个周期只处理一个转换（按配置顺序优先）

---

## 实现规范

### 编码规范

**文件编码**: UTF-8 (无 BOM)

| 元素 | 格式 | 示例 |
|------|------|------|
| 文件头部注释 | 中文 | `/* 状态机 V3.0 - 核心引擎 */` |
| 代码注释 | 中文 | `/* 初始化状态机 */` |
| 日志 TAG | 英文缩写 | `static const char *TAG = "SM_CORE";` |
| 日志内容 | 中文 | `ESP_LOGI(TAG, "状态机初始化完成");` |
| 技术术语 | 保留英文 | JSON, GPIO, HTTP, API |

### 日志输出规范

**日志级别与场景**：

| 级别 | TAG 示例 | 使用场景 | 内容示例 |
|------|----------|----------|----------|
| ERROR | `SM_CORE` | 初始化失败、配置错误 | `ESP_LOGE(TAG, "配置指针为空")` |
| WARN | `SM_GPIO` | 边沿检测异常、定时器溢出 | `ESP_LOGW(TAG, "GPIO组ID超出范围")` |
| INFO | `SM_CORE` | 状态转换、效果执行 | `ESP_LOGI(TAG, "状态转换: %s -> %s", old, new)` |
| DEBUG | `SM_TIMER` | 定时器启动、检查细节 | `ESP_LOGD(TAG, "定时器T0启动，%d秒", seconds)` |


## 附录：电梯教学场景

### 场景1：双向安全控制（门关闭条件）

**教学目标**：
- AND 触发器逻辑（按钮 + 门关闭）
- 双向控制
- 限位器硬件保护

**状态流转**：

| 当前状态 | 触发条件 | 下一状态 | 执行效果 |
|---------|---------|---------|---------|
| STOP | p0上升沿 AND p4高电平 | MOVING_UP | M1正转5档（上升）|
| STOP | p2下降沿 AND p4高电平 | MOVING_DOWN | M1反转5档（下降）|
| MOVING_UP | p1高电平 | STOP | M1停止 |
| MOVING_DOWN | p3高电平 | STOP | M1停止 |

**硬件连接**：

| 端口 | 类型 | 功能 |
|------|------|------|
| p0 | 输入-上升沿 | 上升请求按钮 |
| p2 | 输入-下降沿 | 下降请求按钮 |
| p1 | 输入-高电平 | 上升限位器（顶部） |
| p3 | 输入-高电平 | 下降限位器（底部） |
| p4 | 输入-高电平 | 门关传感器（安全条件） |
| M1 | 输出-电机 | 电梯电机（正转=上升，反转=下降） |

**完整配置**：

```json
{
  "initialState": "STOP",
  "groupModes": {"0": 1, "1": 1, "2": 1, "3": 1, "4": 1},
  "inputModuleTypes": {"0": 0, "1": 0, "2": 0, "3": 0, "4": 0},
  "edgeTypes": {"0": 1, "1": 0, "2": 2, "3": 0, "4": 0},
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

### 场景2：延时等待控制（2秒等待 + 门检测 + 错误处理）

**教学目标**：
- 定时器AND条件（时间 + 门状态同时满足）
- 状态按方向拆分（UP_WAITING / DOWN_WAITING）
- 错误处理与自动恢复

**状态流转**：

| 当前状态 | 触发条件 | 下一状态 | 执行效果 |
|---------|---------|---------|---------|
| STOP | p0上升沿 | UP_WAITING | 启动T0（2秒），LED慢闪 |
| STOP | p2下降沿 | DOWN_WAITING | 启动T0（2秒），LED慢闪 |
| UP_WAITING | T0到期 AND p4高电平 | MOVING_UP | M1正转5档 |
| DOWN_WAITING | T0到期 AND p4高电平 | MOVING_DOWN | M1反转5档 |
| UP_WAITING | T0到期 AND p4=0 | **UP_ERROR** | 报警灯亮，启动T1（5秒）|
| DOWN_WAITING | T0到期 AND p4=0 | **DOWN_ERROR** | 报警灯亮，启动T1（5秒）|
| **UP_ERROR** | **p4高电平** | **MOVING_UP** | **门关闭，立即启动** |
| **DOWN_ERROR** | **p4高电平** | **MOVING_DOWN** | **门关闭，立即启动** |
| **UP_ERROR** | **T1到期** | **STOP** | **超时，复位停止** |
| **DOWN_ERROR** | **T1到期** | **STOP** | **超时，复位停止** |
| MOVING_UP | p1高电平 | STOP | M1停止 |
| MOVING_DOWN | p3高电平 | STOP | M1停止 |

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

**完整配置**：

```json
{
  "initialState": "STOP",
  "groupModes": {"0": 1, "1": 1, "2": 1, "3": 1, "4": 1},
  "inputModuleTypes": {"0": 0, "1": 0, "2": 0, "3": 0, "4": 0},
  "edgeTypes": {"0": 1, "1": 0, "2": 2, "3": 0, "4": 0},
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
    }
  ],
  "stateEffects": [
    {"stateName": "UP_WAITING", "effectType": 2, "target": "T0", "value": 2},
    {"stateName": "UP_WAITING", "effectType": 1, "target": "P6", "value": 1},
    {"stateName": "DOWN_WAITING", "effectType": 2, "target": "T0", "value": 2},
    {"stateName": "DOWN_WAITING", "effectType": 1, "target": "P6", "value": 2},
    {"stateName": "MOVING_UP", "effectType": 3, "target": "M1", "value": 5},
    {"stateName": "MOVING_UP", "effectType": 1, "target": "P6", "value": 0},
    {"stateName": "MOVING_DOWN", "effectType": 3, "target": "M1", "value": -5},
    {"stateName": "MOVING_DOWN", "effectType": 1, "target": "P6", "value": 0},
    {"stateName": "STOP", "effectType": 3, "target": "M1", "value": 0},
    {"stateName": "STOP", "effectType": 1, "target": "P6", "value": 0},
    {"stateName": "UP_ERROR", "effectType": 1, "target": "P6", "value": 3},
    {"stateName": "UP_ERROR", "effectType": 2, "target": "T1", "value": 5},
    {"stateName": "DOWN_ERROR", "effectType": 1, "target": "P6", "value": 3},
    {"stateName": "DOWN_ERROR", "effectType": 2, "target": "T1", "value": 5}
  ]
}
```

---

## 测试规范

### Unity 测试框架

V3 状态机使用 ESP-IDF 集成的 Unity 测试框架进行单元测试和硬件在环(HIL)测试。

#### 测试组件结构

```
components/state_machine_v3/test/
├── test_state_machine_v3.cpp    # 主测试文件
├── CMakeLists.txt               # 测试构建配置
└── README.md                    # 测试说明
```

#### 测试用例分类

**1. 基础功能测试**
- 状态机启动和停止
- 手动触发状态转换
- JSON 配置加载

**2. 定时器触发测试**
- 定时器启动和到期检测
- 延时状态转换验证
- 时序精度测试

**3. 硬件输出效果测试**
- GPIO 输出控制测试
- 电机 PWM 控制测试
- HIL 前后状态对比验证

**4. 课件场景测试**
- 双向安全控制场景
- 电梯控制场景

#### 测试构建和运行

**构建命令**：
```bash
# 构建测试固件
idf.py -DIDF_BUILD_TESTS=1 -DTEST_COMPONENTS="state_machine_v3" build

# 烧录和监控
idf.py -DIDF_BUILD_TESTS=1 -DTEST_COMPONENTS="state_machine_v3" flash monitor -p COM5

# 一键构建、烧录、监控
idf.py -DIDF_BUILD_TESTS=1 -DTEST_COMPONENTS="state_machine_v3" build flash monitor -p COM5
```

**测试执行环境**：
- Unity 测试框架自动运行所有测试用例
- 测试结果通过串口输出详细日志
- 支持 DEBUG 级别日志用于问题排查


#### 测试配置

**硬件初始化**：
```cpp
extern "C" void setUp(void) {
    ensure_hardware_initialized();
    sm_v3_reset();
}

extern "C" void tearDown(void) {
    sm_v3_stop();
    sm_v3_reset();
}
```

**看门狗处理**：
```cpp
// 测试入口禁用看门狗防止超时
esp_task_wdt_deinit();
unity_run_all_tests();
```
---

## 相关文档

| 文档 | 路径 | 说明 |
|------|------|------|
| 前端适配指南 | [docs/api/v3/frontend-guide.md](../../../docs/api/v3/frontend-guide.md) | JSON 配置格式、HTTP API 端点、课件示例 |

---

## 变更记录

本规范的演进历史记录在 [openspec/changes/](../../changes/) 目录下。
