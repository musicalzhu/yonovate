# ESP32 设备控制平台 - Vue 3 版本

基于 Vue 3.5+ + TypeScript 5.3+ + Vite 5.0+ 的现代化 ESP32 设备控制平台，提供完整的状态机配置编辑和设备管理功能。

## 概述

本项目是 ESP32 设备控制平台的 Vue 3 重构版本，采用现代化的前端技术栈，提供更好的开发体验和用户界面。已完全迁移到纯 Vue 3 生态系统，移除了所有旧版 JavaScript 代码和 HTML 页面。

## 🚀 技术栈

### 核心框架
- **Vue 3.5+** - 使用 Composition API + Reactivity Transform
- **TypeScript 5.3+** - 严格类型安全
- **Vite 5.0+** - 极速构建工具
- **Vue Router 4.2+** - 路由管理
- **Pinia 2.1+** - 状态管理（Setup 语法）

### UI 组件库
- **Element Plus 2.5+** - UI 组件库
- **@element-plus/icons-vue 2.3+** - 图标库
- **VueUse 10.0+** - 组合式函数工具库
- **TailwindCSS 3.4+** - CSS 框架

### 开发工具
- **ESLint 8.56+** - 代码检查
- **Prettier 3.1+** - 代码格式化
- **Vitest 1.1+** - 单元测试
- **Vue Test Utils 2.4+** - 组件测试
- **Playwright 1.40+** - E2E 测试

## 🚀 功能特性

### 核心功能
- ✅ **状态机配置编辑器** - 支持完整 V3 格式配置
- ✅ **多触发器支持** - 支持 1-3 个触发器配置
- ✅ **逻辑运算** - AND/OR/XOR 逻辑选择
- ✅ **运算符支持** - ==, <, >, <=, >=, !=
- ✅ **实时 JSON 预览** - 实时显示配置 JSON
- ✅ **模板管理** - 内置电梯、红绿灯、风扇等模板
- ✅ **设备监控** - 实时设备状态监控
- ✅ **响应式设计** - 完整的移动端适配
- ✅ **暗黑模式** - 支持主题切换

## 📁 项目结构

```
web_project/
├── src/
│   ├── components/          # Vue 组件
│   │   ├── base/            # 基础组件
│   │   │   ├── BaseButton.vue
│   │   │   ├── BaseCard.vue
│   │   │   └── BaseInput.vue
│   │   ├── state-machine/   # 状态机组件
│   │   │   └── StateMachineEditor.vue
│   │   └── ui/              # UI组件
│   │       ├── LoadingSpinner.vue
│   │       └── NotificationToast.vue
│   ├── composables/         # VueUse风格组合式函数
│   │   ├── useDeviceConnection.ts
│   │   ├── useStateMachine.ts
│   │   ├── useGPIO.ts
│   │   ├── useValidation.ts
│   │   ├── useNotifications.ts
│   │   ├── useTheme.ts
│   │   └── useResponsive.ts
│   ├── services/            # API服务层
│   │   ├── api/             # 后端API服务
│   │   │   ├── api.ts          # Axios客户端
│   │   │   ├── stateMachine.ts  # 状态机服务
│   │   │   └── device.ts       # 设备服务
│   │   ├── notification/    # 通知服务
│   │   ├── storage/         # 存储服务
│   │   └── validation/      # 验证服务
│   ├── stores/              # Pinia状态管理
│   │   ├── app.ts
│   │   ├── device.ts
│   │   ├── stateMachine.ts
│   │   └── ui.ts
│   ├── types/               # TypeScript类型定义
│   │   ├── api.ts
│   │   ├── stateMachine.ts
│   │   └── device.ts
│   ├── utils/               # 工具函数
│   ├── views/               # 页面视图
│   │   ├── StateMachineV3Page.vue
│   │   └── DeviceControlPage.vue
│   ├── router/              # 路由配置
│   ├── App.vue             # 根组件
│   └── main.ts              # 入口文件
├── tests/                   # 单元测试
│   ├── unit/               # 单元测试
│   └── integration/        # 集成测试
├── index.html              # Vue 3 入口页面
├── vite.config.ts          # Vite配置
├── tsconfig.json           # TypeScript配置
└── package.json
```

## 🛠️ 开发指南

### 组件开发规范

#### 组件命名
- 组件文件名使用 PascalCase：`StateMachineV3Page.vue`
- 组件注册名使用 kebab-case：`state-machine-v3-page`

#### 组件结构
```vue
<template>
  <!-- 模板内容 -->
</template>

<script setup lang="ts">
  import { ref, computed, onMounted } from 'vue'

  interface Props {
    // props 定义
  }

  const props = defineProps<Props>()
  const emit = defineEmits<{
    (e: 'event-name', payload: any): void
  }>()

  const state = ref()
  const computedValue = computed(() => {})
  const method = () => {}

  onMounted(() => {})
</script>

<style scoped>
  /* 组件样式 */
</style>
```

### 状态管理规范

使用 Pinia Setup 语法：
```typescript
import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export const useExampleStore = defineStore('example', () => {
  const state = ref()
  const computedValue = computed(() => {})
  const updateState = (newValue: any) => {
    state.value = newValue
  }
  return { state, computedValue, updateState }
})
```

### 环境要求
- Node.js >= 20.0.0
- npm >= 10.0.0

### 安装依赖

```bash
cd web_project
npm install
```

### 开发模式

```bash
npm run vue:dev
```

访问 http://localhost:5173 查看应用。

### 构建生产版本

```bash
npm run vue:build
```

### 预览生产版本

```bash
npm run vue:preview
```

### 运行测试

```bash
# 运行所有测试
npm test

# 测试覆盖率
npm run test:coverage

# 测试监视模式
npm run test:watch
```

## 🔌 API 接口

### 状态机接口

| 接口 | 方法 | 描述 |
|------|------|------|
| `/api/v3/state-machine` | GET | 获取状态机配置 |
| `/api/v3/state-machine` | POST | 保存状态机配置 |
| `/api/v3/state-machine/status` | GET | 获取状态机状态 |
| `/api/v3/state-machine/trigger` | POST | 手动触发状态机 |
| `/api/v3/state-machine/reset` | POST | 重置状态机 |
| `/api/v3/state-machine/validate` | POST | 验证状态机配置 |

### 设备接口

| 接口 | 方法 | 描述 |
|------|------|------|
| `/api/v3/device/info` | GET | 获取设备信息 |
| `/api/v3/device/gpio` | GET | 获取GPIO状态 |
| `/api/v3/device/gpio` | POST | 设置GPIO输出 |
| `/api/v3/device/wifi` | GET | 获取WiFi配置 |
| `/api/v3/device/wifi` | POST | 设置WiFi配置 |
| `/api/v3/device/reboot` | POST | 重启设备 |

## 📋 数据格式

### 状态机配置格式

```json
{
  "initialState": "IDLE",
  "transitions": [
    {
      "currentState": "IDLE",
      "nextState": "RUNNING",
      "triggers": [
        {
          "triggerType": 1,
          "triggerSource": "P0",
          "operator": 0,
          "triggerValue": 1
        }
      ],
      "triggerCount": 1,
      "triggerLogic": 0
    }
  ],
  "stateEffects": []
}
```

### 触发源规范

| 类别 | 格式 | 示例 |
|------|------|------|
| GPIO | P0-P11 | "P0", "P5", "P11" |
| 定时器 | T0-T2 | "T0", "T1", "T2" |
| 手动 | MANUAL | "MANUAL" |

### 运算符映射

| 运算符 | 值 |
|--------|-----|
| == | 0 |
| < | 1 |
| > | 2 |
| <= | 3 |
| >= | 4 |
| != | 5 |

### 逻辑类型

| 逻辑 | 值 |
|------|-----|
| AND | 0 |
| OR | 1 |
| XOR | 2 |

## 🧪 测试覆盖

### 单元测试
- ✅ StateMachineEditor 组件测试
- ✅ useTheme 组合式函数测试
- ✅ StateMachineService 服务测试
- ✅ DeviceService 服务测试

### 测试命令

```bash
npm test                    # 运行所有测试
npm run test:watch          # 监视模式
npm run test:coverage       # 覆盖率报告
```

## 🎨 主题系统

支持三种主题模式：
- **light** - 亮色主题
- **dark** - 暗色主题  
- **auto** - 跟随系统主题

## 📝 开发规范

### 代码风格
- 使用 TypeScript 严格模式
- 使用 Composition API (`<script setup>`)
- 使用 Pinia Setup 语法
- 组件命名使用 PascalCase
- 文件命名使用 kebab-case

### 目录结构
- `components/` - 可复用组件
- `composables/` - 组合式函数
- `services/` - API服务
- `stores/` - 状态管理
- `types/` - 类型定义
- `views/` - 页面视图

## 📄 页面功能

### 1. 状态机 V3 (StateMachineV3Page)
- 完整格式状态机配置
- 多触发器支持（1-3个）
- AND/OR 逻辑选择
- 实时 JSON 预览
- 运行日志
- 模板管理（电梯、红绿灯、风扇等）

### 2. 设备控制 (DeviceControlPage)
- GPIO 组模式配置
- 输入/输出模块配置
- 实时状态监控
- WiFi 配置管理
- 设备信息显示
- 系统日志查看

## 🏗️ 服务层架构

### API 服务层
Vue 版本使用全新的服务层架构：

```typescript
import { StateMachineService } from '@/services/api/stateMachine'

const service = new StateMachineService()
const config = await service.getConfig()
```

### 数据验证服务
使用 ValidationService 进行配置验证：

```typescript
import { ValidationService } from '@/services/validation'

const validationService = new ValidationService()
const result = validationService.validateConfig(config)
```

### 数据转换服务
使用 DataConverter 进行格式转换：

```typescript
import { DataConverter } from '@/services/validation/dataConverter'

const converter = new DataConverter()
const backendConfig = converter.toBackend(config)
const frontendConfig = converter.toFrontend(response)
```

## 🔧 故障排除

### 常见问题

1. **依赖安装失败**
   ```bash
   npm install --legacy-peer-deps
   ```

2. **TypeScript 错误**
   ```bash
   npm run vue:build
   ```

3. **开发服务器启动失败**
   ```bash
   npm run vue:dev --host
   ```

### 调试技巧

1. 使用 Vue DevTools 调试组件状态
2. 查看浏览器控制台日志
3. 使用 Network 面板检查 API 调用

## 📄 许可证

MIT License

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

---

**Version**: 3.0.0  
**Build**: Vue 3.5 + TypeScript 5.3 + Vite 5.0
