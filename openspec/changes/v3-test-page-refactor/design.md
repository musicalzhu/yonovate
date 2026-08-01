# 设计：V3 测试页面重构


## 1. 完整格式（语义模型）

### 1.1 转换与触发器

每条转换包含：

- `currentState` / `nextState`
- `triggers[]`：长度 1–3
- `triggerCount`、`triggerLogic`（AND / OR）
- 每个触发器：`triggerType`、`triggerSource`、`operator`（0–5）、`triggerValue`

运算符与 UI 展示对应：`== / < / > / <= / >= / !=` → `0–5`（与固件约定一致）。

### 1.2 触发源字符串规范

| 类别 | JSON 示例 |
|------|----------|
| GPIO 组 | `"P0"` … `"P11"` |
| 定时器 | `"T0"`、`"T1"`、`"T2"` |
| 手动 | `"MANUAL"` |

### 1.3 模板与示例

`state_machine_v3_config.js` 中的 `buildConfig(category, templateName)` 产出完整 V3 结构；电梯/红绿灯/风扇/多按钮等模板演示不同触发器组合与逻辑。

## 2. 传输与命名规范

**编辑态 / UI 内存**：camelCase（与 JS 及模板脚本一致）

**HTTP 请求体**：由 `DataConverter.toBackend` 将内部模型转为设备端约定的键名（含顶层 snake_case、触发器内 `type`/`source`/`value` 等）；非直接 POST 前端对象

**HTTP 响应**：`GET /api/v3/state-machine` 返回的 `config` 经 `toFrontend` 转换后进入 UI

## 3. 模块化文件布局

```
web_project/pages/state_machine_v3.html    # 静态结构 + config + ES module 入口
web_project/js/state-machine-ui.js        # StateMachineUI：渲染、事件、模板加载
web_project/js/state-machine-api.js       # StateMachineAPI：resolveBaseURL、GET/POST
web_project/js/state-machine-data.js      # DataConverter、DataValidator
web_project/js/state_machine_v3_config.js # buildConfig、validateV3Config（全局）
web_project/js/config.js                  # ESP32_CONFIG（可选）
```

唯一入口为 `state_machine_v3.html`。

## 4. UI 信息架构

- **模板区**：一键加载 `buildConfig` 预设
- **转换列表**：每条展开为多个触发器卡片；支持 AND/OR 逻辑；触发器上限为 3
- **状态效果 / GPIO 区块**：与完整配置对象一致
- **JSON 预览**：展示内部 camelCase 视图（符合调试习惯）；保存时经过转换层

## 5. 校验策略与前后端边界

**页面不做的事情**：
- **不进行配置校验**：不在浏览器侧实现任何配置校验逻辑，配置的最终合法性、设备加载能力、运行时约束等全部以 HTTP API 与固件为准
- 不复制 `POST /api/v3/state-machine` 内部的判定逻辑
- 不做任何前端校验，直接提交配置给后端处理

**验证状态**：如「上次加载是否成功」、服务端状态文案等，须来自 `GET /api/v3/state-machine` 的现有字段（如 `lastLoadSuccess`、`lastLoadStatus`），由页面展示；不在前端另建与设备脱钩的验证状态机

**错误展示**：400 响应体说明优先于通用网络错误；用户可见提示以中文为主。详细条款见 `specs/v3-test-page/spec.md`。

## 6. 测试建议

| 层级 | 内容 |
|------|------|
| 逻辑 | 转换往返、混合键名样例 |
| API | Mock `fetch`、超时、400 响应 |
| 设备 | POST 加载、GET 回填、多触发器场景、监控区刷新 |

## 7. Vue 3最佳实践架构设计

### 7.1 技术栈选择（现代化最佳实践）

```
┌─────────────────────────────────────────────────┐
│              Vue 3最佳实践技术栈        │
├─────────────────────────────────────────────────┤
│                                                 │
│  📦 核心框架：                                   │
│  ├── Vue 3.5+ (Composition API + <script setup> + Reactivity Transform)
│  ├── TypeScript 5.3+ (严格模式 + Reactivity Transform)
│  ├── Vite 5.0+ (HMR + 优化构建)
│  ├── Vue Router 4.2+ (路由懒加载)
│  └── Pinia 2.1+ (Setup语法)                     │
│                                                 │
│  🎨 UI组件库：                                   │
│  ├── Element Plus 2.5+ (完整TS支持)              │
│  ├── @element-plus/icons-vue 2.3+               │
│  ├── VueUse 10.0+ (组合式函数工具库)             │
│  └── TailwindCSS 3.4+ (实用优先)                 │
│                                                 │
│  🛠️ 开发工具：                                   │
│  ├── ESLint 8.56+ (Vue 3规则)                   │
│  ├── Prettier 3.1+ (代码格式化)                  │
│  ├── Vue DevTools (浏览器调试)                   │
│  └── TypeScript编译器 (严格类型检查)              │
│                                                 │
│  🧪 测试工具：                                   │
│  ├── Vitest 1.1+ (Vue专用)                      │
│  ├── Vue Test Utils 2.4+ (组件测试)              │
│  ├── Playwright 1.40+ (E2E测试)                 │
│  └── MSW 2.0+ (API Mock)                        │
│                                                 │
└─────────────────────────────────────────────────┘
```

### 7.2 组件化架构（分层设计）

```
┌─────────────────────────────────────────────────┐
│              Vue最佳实践组件架构        │
├─────────────────────────────────────────────────┤
│                                                 │
│  📁 应用层 (App Layer)                          │
│  ├── App.vue                                    │
│  ├── Layout/                                     │
│  │   ├── AppHeader.vue                          │
│  │   ├── AppSidebar.vue                         │
│  │   └── AppFooter.vue                          │
│                                                 │
│  📁 页面层 (Pages Layer)                        │
│  ├── pages/                                     │
│  │   ├── Dashboard.vue                          │
│  │   ├── StateMachine/                          │
│  │   │   ├── index.vue                          │
│  │   │   ├── Editor.vue                         │
│  │   │   └── Monitor.vue                        │
│  │   ├── DeviceControl/                         │
│  │   │   ├── GPIO.vue                           │
│  │   │   ├── LogicCircuit.vue                   │
│  │   │   └── WiFi.vue                           │
│  │   └── Settings.vue                           │
│                                                 │
│  📁 组件层 (Components Layer)                    │
│  ├── components/                                 │
│  │   ├── common/                                │
│  │   │   ├── BaseButton.vue                     │
│  │   │   ├── BaseInput.vue                      │
│  │   │   ├── BaseCard.vue                       │
│  │   │   └── LoadingSpinner.vue                 │
│  │   ├── forms/                                 │
│  │   │   ├── FormField.vue                      │
│  │   │   ├── FormGroup.vue                      │
│  │   │   └── FormValidation.vue                 │
│  │   ├── charts/                                │
│  │   │   ├── StateDiagram.vue                   │
│  │   │   └── LogicDiagram.vue                   │
│  │   └── device/                                │
│  │       ├── GPIOStatus.vue                     │
│  │       ├── ConnectionStatus.vue               │
│  │       └── DeviceInfo.vue                    │
│                                                 │
│  📁 组合式函数层 (Composables Layer)             │
│  ├── composables/                               │
│  │   ├── useDeviceConnection.ts                 │
│  │   ├── useStateMachine.ts                     │
│  │   ├── useGPIO.ts                            │
│  │   ├── useValidation.ts                      │
│  │   └── useNotifications.ts                   │
│                                                 │
│  📁 服务层 (Services Layer)                     │
│  ├── services/                                  │
│  │   ├── api.ts                                │
│  │   ├── device.ts                             │
│  │   ├── stateMachine.ts                       │
│  │   └── validation.ts                        │
│                                                 │
│  📁 状态管理层 (Stores Layer)                    │
│  ├── stores/                                    │
│  │   ├── app.ts                                │
│  │   ├── device.ts                             │
│  │   ├── stateMachine.ts                       │
│  │   └── ui.ts                                 │
│                                                 │
│  📁 类型定义层 (Types Layer)                     │
│  ├── types/                                     │
│  │   ├── api.ts                                │
│  │   ├── device.ts                             │
│  │   ├── stateMachine.ts                       │
│  │   └── common.ts                             │
│                                                 │
└─────────────────────────────────────────────────┘
```

### 7.3 Vue最佳实践迁移策略（完全重构）

**阶段1：纯Vue基础设施搭建**
- 创建全新的Vue 3 + TypeScript + Vite项目
- 配置严格的TypeScript和ESLint规则
- 设置Vue Router 4.2+和Pinia 2.1+
- 配置Element Plus和VueUse生态
- 建立完整的开发工具链

**阶段2：服务层和类型系统重构**
- 创建纯Vue原生的API服务层
- 建立完整的TypeScript类型系统
- 实现数据验证和转换服务
- 创建错误处理和通知系统
- 建立本地存储和会话管理

**阶段3：状态管理和组合式函数**
- 设计Pinia Store架构（Setup语法）
- 创建VueUse风格的组合式函数
- 实现响应式数据流
- 建立统一的错误处理机制
- 创建可复用的业务逻辑

**阶段4：组件库和页面重构**
- 创建分层组件架构
- 实现通用组件库
- 重构状态机V3页面
- 重构设备控制页面
- 实现响应式设计和主题系统

**阶段5：测试和优化**
- 建立完整的测试体系
- 性能优化和代码分割
- 文档和开发指南
- 部署和CI/CD配置

### 7.4 数据流架构（单向数据流）

```
┌─────────────────────────────────────────────────┐
│              Vue最佳实践数据流          │
├─────────────────────────────────────────────────┤
│                                                 │
│  用户操作 → 组件事件 → Composable → Store Action │
│     ↓                                           │
│  Store State → 响应式更新 → 组件重新渲染        │
│     ↓                                           │
│  Service API → 后端通信 → 数据转换 → Store     │
│     ↓                                           │
│  错误处理 → 通知系统 → 用户反馈                 │
│                                                 │
│  🔄 单向数据流：                                 │
│  ┌─────────┐    ┌──────────┐    ┌──────────┐   │
│  │Component│───▶│Composable │───▶│  Store   │   │
│  └─────────┘    └──────────┘    └──────────┘   │
│       ▲              │              │         │
│       │              │              ▼         │
│       │              │         ┌──────────┐   │
│       │              │         │ Service  │   │
│       │              │         └──────────┘   │
│       │              │              │         │
│       │              │              ▼         │
│       │              │         ┌──────────┐   │
│       │              └─────────│   API    │   │
│       │                        └──────────┘   │
│       │                                         │
│       └─────────────────────────────────────────┘
│                                                 │
└─────────────────────────────────────────────────┘
```

### 7.5 纯Vue生态架构（无历史包袱）

#### 完全重新设计的技术栈
- **API层**：使用Axios + 拦截器，完全Vue原生
- **数据层**：TypeScript类型系统 + Zod验证
- **状态管理**：Pinia Setup语法 + 响应式设计
- **组件层**：Composition API + `<script setup>`
- **工具层**：VueUse + 自定义组合式函数

#### 架构原则
1. **类型安全优先** - 所有API都有完整的TypeScript类型
2. **组合优于继承** - 使用组合式函数而不是混入
3. **单一数据源** - Pinia作为唯一的状态管理
4. **服务层抽象** - 完全的API服务层设计
5. **组件化设计** - 高度可复用的组件架构

#### 与原有系统的关系
- **API接口兼容** - 保持与ESP32后端的通信协议
- **数据格式兼容** - 保持状态机配置的数据结构
- **功能完全重写** - 前端完全使用Vue最佳实践
- **无历史依赖** - 不依赖任何原有JavaScript模块
