## 重构需求

### 需求：重构范围与前后端职责边界（总纲）

V3 Web 测试页（`state_machine_v3.html`）重构需遵守以下边界：

**页面负责**：完整 V3 结构的可视化编辑、JSON 预览、模板加载、模块化前端架构、内部 camelCase 模型、通过 `DataConverter` 的键名转换。

**页面不负责**（不重复实现后端已有能力）：

- **不进行配置校验**：不在前端复制与固件等价的「状态机配置语义校验引擎」。配置的最终合法性、设备加载能力、运行时约束等全部以 HTTP API 与固件为准。
- 验证状态机配置的权威结论（如加载是否成功、服务端验证文案）来自 `GET /api/v3/state-machine`（及现有响应字段，如 `lastLoadSuccess`、`lastLoadStatus` 等），页面仅展示与转发错误信息，不自行推导判定逻辑。
- POST 提交后的业务失败优先展示响应体中的服务端说明。

#### 场景：语义级校验以设备与 API 为准

- **触发条件**：需要判断配置是否被设备接受或存在仅固件知晓的冲突
- **预期行为**：以 POST/GET 的 HTTP 状态与响应正文为权威；前端不做任何校验

#### 场景：加载/验证状态来自后端响应

- **触发条件**：用户查看「验证状态」「上次加载是否成功」等与设备相关的状态
- **预期行为**：数据来自现有 GET 接口返回字段，页面不单独维护与固件脱钩的持久判定状态

#### 场景：后端返回 400 时优先展示服务端原因

- **触发条件**：后端对配置返回 HTTP 400 或带错误说明的响应体
- **预期行为**：页面尽可能展示服务端返回的具体原因，而非统一替换为通用「请求失败」

---

### 需求：前端内部统一使用 camelCase，传输与后端约定对齐

前端在内存与 UI 状态中使用 camelCase；通过 `DataConverter` 在保存/加载时与设备端约定的 snake_case 顶层字段及触发器内 `type`/`source`/`value` 等互转。

#### 场景：编辑态与预览

- **触发条件**：用户编辑配置或查看 JSON 预览
- **预期行为**：内存对象为 camelCase，而非直接以后端 snake_case 作为主编辑模型

#### 场景：保存与加载

- **触发条件**：用户保存或从设备加载配置
- **预期行为**：POST 请求体经 `toBackend`；GET 响应中的 `config` 经 `toFrontend` 后再渲染

---

### 需求：UI、API 与数据模块分层；入口无大块内联业务脚本

前端应将 DOM 与交互（`state-machine-ui.js`）、HTTP（`state-machine-api.js`）、转换与轻量校验（`state-machine-data.js`）分离；入口 HTML 仅含静态结构、`config.js`、`state_machine_v3_config.js`、ES 模块 `import '../js/...'` 及少量 `window` 胶水。

#### 场景：打开测试页

- **触发条件**：用户打开 `web_project/pages/state_machine_v3.html`
- **预期行为**：业务逻辑位于 `web_project/js/state-machine-*.js`；模块脚本使用 `../js/` 路径

#### 场景：仅改 HTTP 封装

- **触发条件**：只需调整 Base URL、超时或某一 API 封装
- **预期行为**：主要修改集中在 `state-machine-api.js`，无需牵动 UI 与转换层无关代码

#### 场景：模板来源

- **触发条件**：用户加载示例模板
- **预期行为**：由 `state_machine_v3_config.js` 的 `buildConfig`（或等价全局）提供；UI 负责触发与渲染

---

### 需求：完整格式编辑器能力（多触发器、逻辑、运算符、标准源）

测试页应支持的触发器、AND/OR 逻辑等参考数据类型模型与接口定义。


#### 场景：多场景模板

- **触发条件**：用户点击示例模板按钮
- **预期行为**：加载完整 V3 结构（含多触发器与逻辑），非简化占位

#### 场景：JSON 预览随编辑更新

- **触发条件**：用户修改触发器相关字段
- **预期行为**：JSON 预览实时反映当前内存配置

---

## 数据模型与接口定义

### 前端数据模型

基于 V3 状态机后端规格，前端使用 camelCase 命名约定，通过 `DataConverter` 与后端 snake_case 互转。

#### 核心类型枚举

```typescript
// 触发器类型
enum TriggerType { NONE = 0, EDGE = 1, LEVEL = 2, TIMER = 3, MANUAL = 4 }

// 边沿类型
enum EdgeType { ANY = 0, RISING = 1, FALLING = 2 }

// 比较运算符：EQ(等于)、LT(小于)、GT(大于)、LE(小于等于)、GE(大于等于)、NE(不等于)
enum Operator { EQ = 0, LT = 1, GT = 2, LE = 3, GE = 4, NE = 5 }

// 逻辑运算
enum LogicType { AND = 0, OR = 1 }

// 效果类型：NONE、OUTPUT、TIMER、MOTOR、NOTIFICATION
enum EffectType { NONE = 0, OUTPUT = 1, TIMER = 2, MOTOR = 3, NOTIFICATION = 4 }

// 输入模块类型：NORMAL_BUTTON、LED_BUTTON、TEMP_SENSOR、HUMIDITY_SENSOR、VOLTAGE_MONITOR
enum InputModuleType { NORMAL_BUTTON = 0, LED_BUTTON = 1, TEMP_SENSOR = 2, HUMIDITY_SENSOR = 3, VOLTAGE_MONITOR = 4 }

// 输出模块类型：LED、TRAFFIC_LIGHT、FAN
enum OutputModuleType { LED = 0, TRAFFIC_LIGHT = 1, FAN = 2 }
```

#### 数据结构接口

```typescript
// 单个触发器
interface Trigger {
  triggerType: TriggerType;
  triggerSource: string;  // "P0"-"P11", "T0"-"T2", "MANUAL"
  operator: Operator;
  triggerValue: number;
}

// 状态转换规则
interface Transition {
  currentState: string;
  nextState: string;
  triggers: Trigger[];     // 1-3 个触发器
  triggerCount: number;    // 1-3
  triggerLogic: LogicType; // AND/OR
}

// 状态效果
interface StateEffect {
  stateName: string;
  effectType: EffectType;
  target: string;  // "P0"-"P11", "T0"-"T2", "M0"-"M1"
  value: number;
}

// V3 状态机配置 (前端 camelCase)
interface StateMachineConfig {
  initialState: string;
  transitions: Transition[];
  transitionCount: number;
  stateEffects: StateEffect[];
  effectCount: number;
  groupModes: number[];           // 0=输出, 1=输入, 长度12
  inputModuleTypes: InputModuleType[];
  edgeTypes: EdgeType[];
  outputModuleTypes: OutputModuleType[];
}

// API 响应通用格式
interface ApiResponse<T> {
  success: boolean;
  data?: T;
  error?: string;
  lastLoadSuccess?: boolean;
  lastLoadStatus?: string;
}

// 状态机状态
interface StateMachineStatus {
  currentState: string;
  isRunning: boolean;
  config: StateMachineConfig;
}
```


### API 接口定义

| 方法 | 端点 | 请求体 | 响应体 | 说明 |
|------|------|--------|--------|------|
| GET | `/api/v3/state-machine` | - | `ApiResponse<StateMachineStatus>` | 获取状态机配置和当前状态 |
| POST | `/api/v3/state-machine` | `StateMachineConfig` | `ApiResponse<void>` | 更新状态机配置 |
| POST | `/api/v3/state-machine/trigger` | `{ triggerSource: string }` | `ApiResponse<void>` | 手动触发状态转换 |
| POST | `/api/v3/state-machine/reset` | - | `ApiResponse<void>` | 重置状态机 |


### 约束与验证规则

| 约束项 | 规则 |
|--------|------|
| 状态名称 | 1-32 字符，无特殊字符 |
| 触发器数量 | 每转换 1-3 个 |
| GPIO 组 | P0-P11（12 组） |
| 定时器 | T0-T2（3 个） |
| 电机 | M0-M1（2 个） |
| 运算符 | 0-5 |
| 电机档位 | -9 到 9 |
| 定时器值 | 正整数毫秒 |

以上定义确保前端与后端数据模型一致，支持完整的结构校验和类型安全。

---

## Vue 3最佳实践技术规范

### 前端技术栈规范

#### 核心框架
- **Vue 3.5+**：使用Composition API + `<script setup>`语法糖 + Reactivity Transform
- **TypeScript 5.3+**：严格模式，所有API必须有类型定义
- **Vite 5.0+**：作为构建工具，支持HMR和优化构建
- **Vue Router 4.2+**：用于页面路由管理，支持路由懒加载
- **Pinia 2.1+**：作为状态管理库，完全TypeScript支持

#### UI组件库
- **Element Plus 2.5+**：基于Vue 3的组件库，完整TypeScript支持
- **@element-plus/icons-vue 2.3+**：图标库
- **VueUse 10.0+**：组合式函数工具库，提升开发效率
- **TailwindCSS 3.4+**：实用优先的CSS框架，用于快速样式开发

#### 开发工具
- **ESLint 8.56+**：代码检查，Vue 3专用规则
- **Prettier 3.1+**：代码格式化
- **Vue DevTools**：浏览器调试工具
- **TypeScript编译器**：严格类型检查 + Reactivity Transform

#### 测试工具（完整测试体系）
- **Vitest 1.1+**：Vue专用的单元测试框架
- **Vue Test Utils 2.4+**：Vue组件测试工具库
- **@vue/test-utils-extras**：Vue 3.5+测试增强工具
- **Playwright 1.40+**：E2E测试框架
- **MSW 2.0+**：API Mock库，用于测试API调用
- **@testing-library/vue**：组件测试库，更接近用户行为
- **@testing-library/jest-dom**：DOM测试匹配器

### Vue组件规范

#### 组件命名
- 组件文件名使用PascalCase：`StateMachineV3.vue`
- 组件注册名使用kebab-case：`state-machine-v3`
- 组件内部使用PascalCase引用其他组件

#### 组件结构（Vue 3.5+最佳实践）
```vue
<template>
  <!-- 模板内容 -->
  <div class="component-name">
    <!-- 使用语义化HTML标签 -->
    <header class="component-header">
      <h2>{{ title }}</h2>
    </header>
    
    <main class="component-body">
      <!-- 条件渲染使用v-if，列表渲染使用v-for -->
      <div v-if="loading" class="loading-state">
        <el-loading-spinner />
      </div>
      
      <div v-else class="content">
        <!-- 事件处理使用@前缀 -->
        <el-button @click="handleClick" :disabled="!canClick">
          {{ buttonText }}
        </el-button>
      </div>
    </main>
  </div>
</template>

<script setup lang="ts">
  // Vue 3.5+ Reactivity Transform - 启用$ref和$computed
  // 导入 - 按需导入，避免全局导入
  import { $ref, $computed, onMounted, watch } from 'vue'
  import { useRouter } from 'vue-router'
  import { ElMessage } from 'element-plus'
  
  // 类型定义 - 使用interface定义复杂类型
  interface Props {
    title: string
    readonly?: boolean
    initialData?: StateMachineConfig
  }
  
  interface Emits {
    save: [config: StateMachineConfig]
    cancel: []
    update: [field: string, value: any]
  }

  // Vue 3.5+ defineModel - 简化v-model实现
  const modelValue = defineModel<StateMachineConfig>({ default: () => getDefaultConfig() })
  
  // Props和Emits - 使用defineProps和defineEmits
  const props = withDefaults(defineProps<Props>(), {
    readonly: false
  })
  
  const emit = defineEmits<Emits>()

  // 组合式函数 - 优先使用VueUse和自定义composables
  const router = useRouter()
  const { config, isLoading, error } = useStateMachine()
  const { validate, errors } = useValidation()
  const { notify } = useNotifications()

  // Vue 3.5+ Reactivity Transform - 使用$ref和$computed
  const isDirty = $ref(false)
  const localConfig = $ref(props.initialData || getDefaultConfig())

  // 计算属性 - 使用$computed，避免在模板中写复杂逻辑
  const canSave = $computed(() => {
    return !props.readonly && isDirty && validate(localConfig).valid
  })
  
  const buttonText = $computed(() => {
    return props.readonly ? '只读模式' : (isLoading ? '保存中...' : '保存')
  })

  // Props解构 - Vue 3.5+新特性
  const { title, readonly } = toRefs(props)

  // 方法 - 使用函数式编程，避免副作用
  const handleClick = async () => {
    if (!canSave) return
    
    try {
      isLoading.value = true
      await saveConfig(localConfig)
      emit('save', localConfig)
      notify('success', '配置保存成功')
      isDirty.value = false
    } catch (err) {
      notify('error', '配置保存失败')
      console.error('Save failed:', err)
    } finally {
      isLoading.value = false
    }
  }

  // 监听器 - 使用watch监听响应式数据变化
  watch(localConfig, () => {
    isDirty.value = true
  }, { deep: true })

  // 生命周期 - 只在必要时使用
  onMounted(() => {
    if (props.initialData) {
      localConfig.value = { ...props.initialData }
    }
  })
</script>

<style scoped>
  /* 使用scoped样式避免样式污染 */
  .component-name {
    /* 使用CSS变量便于主题定制 */
    --primary-color: #409eff;
    --text-color: #303133;
    
    /* 使用语义化类名 */
    padding: 1rem;
    border: 1px solid var(--primary-color);
    border-radius: 8px;
  }

  .component-header {
    margin-bottom: 1rem;
  }

  .component-body {
    min-height: 200px;
  }

  .loading-state {
    display: flex;
    justify-content: center;
    align-items: center;
    height: 100px;
  }

  .content {
    /* 使用flexbox布局 */
    display: flex;
    flex-direction: column;
    gap: 1rem;
  }

  /* 响应式设计 */
  @media (max-width: 768px) {
    .component-name {
      padding: 0.5rem;
    }
  }
</style>
```

#### 响应式数据规范（Vue最佳实践）
- **基本类型**：使用`ref<T>()`，明确指定泛型类型
- **复杂对象**：使用`reactive<T>()`，或`ref<T>()`配合解构
- **计算属性**：使用`computed<T>()`，避免在模板中写复杂逻辑
- **只读数据**：使用`readonly()`或`computed(() => ...)`创建
- **避免直接修改props**：使用emit通知父组件或使用v-model

### 状态管理规范（Pinia最佳实践）

#### Store结构（Setup语法）
```typescript
import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { StateMachineConfig, Transition } from '@/types/stateMachine'

export const useStateMachineStore = defineStore('stateMachine', () => {
  // State - 使用ref和reactive
  const config = ref<StateMachineConfig>({
    initialState: 'IDLE',
    transitions: [],
    stateEffects: []
  })
  
  const isLoading = ref(false)
  const error = ref<string | null>(null)
  const lastSaved = ref<Date | null>(null)

  // Getters - 使用computed，支持缓存
  const transitionCount = computed(() => config.value.transitions.length)
  const effectCount = computed(() => config.value.stateEffects.length)
  const isValid = computed(() => validateConfig(config.value))
  const canSave = computed(() => !isLoading.value && isValid.value)

  // Actions - 使用async/await，错误处理
  const updateConfig = (newConfig: StateMachineConfig) => {
    config.value = { ...newConfig }
  }

  const addTransition = (transition: Transition) => {
    config.value.transitions.push({ ...transition })
  }

  const removeTransition = (index: number) => {
    config.value.transitions.splice(index, 1)
  }

  const saveConfig = async () => {
    try {
      isLoading.value = true
      error.value = null
      
      // 调用API服务
      await stateMachineService.saveConfig(config.value)
      lastSaved.value = new Date()
      
      return true
    } catch (err) {
      error.value = err instanceof Error ? err.message : '保存失败'
      throw err
    } finally {
      isLoading.value = false
    }
  }

  const reset = () => {
    config.value = {
      initialState: 'IDLE',
      transitions: [],
      stateEffects: []
    }
    error.value = null
    lastSaved.value = null
  }

  return {
    // State
    config: readonly(config),
    isLoading: readonly(isLoading),
    error: readonly(error),
    lastSaved: readonly(lastSaved),
    
    // Getters
    transitionCount,
    effectCount,
    isValid,
    canSave,
    
    // Actions
    updateConfig,
    addTransition,
    removeTransition,
    saveConfig,
    reset
  }
})
```

#### Store命名和结构
- **文件命名**：使用camelCase，如`stateMachine.ts`
- **Store函数**：使用use前缀，如`useStateMachine()`
- **State暴露**：使用`readonly()`保护状态
- **Actions**：返回Promise，支持async/await
- **错误处理**：统一的错误状态管理

### 组合式函数规范（Composables最佳实践）

#### Composables结构（VueUse风格）
```typescript
import { ref, computed, watch, onUnmounted } from 'vue'
import { useIntervalFn, useWebSocket } from '@vueuse/core'
import type { StateMachineConfig } from '@/types/stateMachine'

export function useStateMachine(deviceId?: string) {
  // 响应式状态
  const config = ref<StateMachineConfig | null>(null)
  const isLoading = ref(false)
  const error = ref<string | null>(null)
  const isConnected = ref(false)

  // 计算属性
  const isValid = computed(() => config.value ? validateConfig(config.value) : false)
  const transitionCount = computed(() => config.value?.transitions.length ?? 0)

  // 方法
  const loadConfig = async () => {
    if (!deviceId) throw new Error('Device ID is required')
    
    try {
      isLoading.value = true
      error.value = null
      
      const response = await stateMachineService.getConfig(deviceId)
      config.value = response.data
      isConnected.value = true
      
      return response.data
    } catch (err) {
      error.value = err instanceof Error ? err.message : '加载失败'
      isConnected.value = false
      throw err
    } finally {
      isLoading.value = false
    }
  }

  const saveConfig = async (newConfig: StateMachineConfig) => {
    if (!deviceId) throw new Error('Device ID is required')
    
    try {
      isLoading.value = true
      error.value = null
      
      const response = await stateMachineService.saveConfig(deviceId, newConfig)
      config.value = newConfig
      
      return response
    } catch (err) {
      error.value = err instanceof Error ? err.message : '保存失败'
      throw err
    } finally {
      isLoading.value = false
    }
  }

  // 自动刷新
  const { pause, resume } = useIntervalFn(() => {
    if (isConnected.value && !isLoading.value) {
      loadConfig()
    }
  }, 5000)

  // 监听连接状态
  watch(isConnected, (connected) => {
    if (connected) {
      resume()
    } else {
      pause()
    }
  })

  // 清理
  onUnmounted(() => {
    pause()
  })

  return {
    // State
    config: readonly(config),
    isLoading: readonly(isLoading),
    error: readonly(error),
    isConnected: readonly(isConnected),
    
    // Getters
    isValid,
    transitionCount,
    
    // Methods
    loadConfig,
    saveConfig,
    
    // 控制
    refresh: loadConfig,
    pauseAutoRefresh: pause,
    resumeAutoRefresh: resume
  }
}
```

#### Composables命名和规范
- **文件命名**：使用use前缀，如`useStateMachine.ts`
- **函数命名**：使用use前缀，如`useStateMachine()`
- **返回值**：使用`readonly()`保护内部状态
- **类型安全**：完整的TypeScript类型定义
- **生命周期**：正确处理组件卸载时的清理
- **错误处理**：统一的错误处理机制

### 类型定义规范（TypeScript最佳实践）

#### 类型定义位置和结构
```
types/
├── api.ts              # API相关类型
├── stateMachine.ts     # 状态机相关类型
├── device.ts          # 设备相关类型
├── common.ts          # 通用类型
└── index.ts           # 类型导出
```

#### 类型定义示例（严格类型安全）
```typescript
// types/stateMachine.ts
export enum TriggerType {
  NONE = 0,
  EDGE = 1,
  LEVEL = 2,
  TIMER = 3,
  MANUAL = 4
}

export enum LogicType {
  AND = 0,
  OR = 1
}

export interface Trigger {
  readonly triggerType: TriggerType
  readonly triggerSource: string // 格式：P0-P11, T0-T2, MANUAL
  readonly operator: number // 0-5: ==, <, >, <=, >=, !=
  readonly triggerValue: number
}

export interface Transition {
  readonly currentState: string
  readonly nextState: string
  readonly triggers: readonly Trigger[]
  readonly triggerCount: number // 1-3
  readonly triggerLogic: LogicType
}

export interface StateEffect {
  readonly stateName: string
  readonly effectType: number
  readonly target: string
  readonly value: number
  readonly duration?: number
}

export interface StateMachineConfig {
  readonly initialState: string
  readonly transitions: readonly Transition[]
  readonly stateEffects: readonly StateEffect[]
  readonly groupModes?: readonly number[]
  readonly inputModuleTypes?: readonly number[]
  readonly edgeTypes?: readonly number[]
  readonly outputModuleTypes?: readonly number[]
}

// 类型守卫
export function isValidTriggerType(value: unknown): value is TriggerType {
  return Object.values(TriggerType).includes(value as TriggerType)
}

export function isValidTriggerSource(value: unknown): value is string {
  if (typeof value !== 'string') return false
  return /^(P[0-9]|T[0-2]|MANUAL)$/.test(value)
}
```

### 服务层架构规范（纯Vue生态）

#### 服务层结构
```
services/
├── api/
│   ├── client.ts         # HTTP客户端
│   ├── device.ts         # 设备API
│   └── stateMachine.ts   # 状态机API
├── validation/
│   ├── stateMachine.ts   # 状态机验证
│   └── device.ts         # 设备验证
└── storage/
    ├── localStorage.ts    # 本地存储
    └── session.ts         # 会话存储
```

#### API服务示例（完全Vue原生）
```typescript
// services/api/stateMachine.ts
import type { StateMachineConfig, StateMachineStatus } from '@/types/stateMachine'
import { apiClient } from './client'

export class StateMachineService {
  private readonly basePath = '/api/v3/state-machine'

  async getConfig(): Promise<StateMachineConfig> {
    const response = await apiClient.get<StateMachineConfig>(this.basePath)
    return response.data
  }

  async saveConfig(config: StateMachineConfig): Promise<void> {
    await apiClient.post(this.basePath, config)
  }

  async getStatus(): Promise<StateMachineStatus> {
    const response = await apiClient.get<StateMachineStatus>(`${this.basePath}/status`)
    return response.data
  }

  async trigger(source: string): Promise<void> {
    await apiClient.post(`${this.basePath}/trigger`, { triggerSource: source })
  }

  async reset(): Promise<void> {
    await apiClient.post(`${this.basePath}/reset`)
  }
}

// 单例导出
export const stateMachineService = new StateMachineService()
```

#### HTTP客户端示例
```typescript
// services/api/client.ts
import type { AxiosResponse, AxiosError } from 'axios'
import axios from 'axios'
import { ElMessage } from 'element-plus'
import { useApiStore } from '@/stores/api'

interface ApiResponse<T> {
  data: T
  success: boolean
  message?: string
}

class ApiClient {
  private client = axios.create({
    timeout: 10000,
    headers: {
      'Content-Type': 'application/json'
    }
  })

  constructor() {
    // 请求拦截器
    this.client.interceptors.request.use(
      (config) => {
        const apiStore = useApiStore()
        config.baseURL = apiStore.baseURL
        return config
      },
      (error) => Promise.reject(error)
    )

    // 响应拦截器
    this.client.interceptors.response.use(
      (response) => response,
      (error: AxiosError) => {
        const message = error.response?.data?.message || error.message || '请求失败'
        ElMessage.error(message)
        return Promise.reject(error)
      }
    )
  }

  async get<T>(url: string): Promise<ApiResponse<T>> {
    const response = await this.client.get<ApiResponse<T>>(url)
    return response.data
  }

  async post<T>(url: string, data?: any): Promise<ApiResponse<T>> {
    const response = await this.client.post<ApiResponse<T>>(url, data)
    return response.data
  }

  async put<T>(url: string, data?: any): Promise<ApiResponse<T>> {
    const response = await this.client.put<ApiResponse<T>>(url, data)
    return response.data
  }

  async delete<T>(url: string): Promise<ApiResponse<T>> {
    const response = await this.client.delete<ApiResponse<T>>(url)
    return response.data
  }
}

export const apiClient = new ApiClient()
```

### 组件架构规范（分层设计）

#### 组件分层架构
```
src/components/
├── layout/              # 布局组件
│   ├── AppHeader.vue
│   ├── AppSidebar.vue
│   └── AppFooter.vue
├── common/              # 通用组件
│   ├── BaseButton.vue
│   ├── BaseInput.vue
│   ├── BaseCard.vue
│   └── LoadingSpinner.vue
├── forms/               # 表单组件
│   ├── FormField.vue
│   ├── FormGroup.vue
│   └── FormValidation.vue
├── device/              # 设备相关组件
│   ├── ConnectionStatus.vue
│   ├── GPIOStatus.vue
│   └── DeviceInfo.vue
└── stateMachine/        # 状态机组件
    ├── StateDiagram.vue
    ├── TransitionEditor.vue
    ├── TriggerEditor.vue
    └── EffectEditor.vue
```

#### 组件设计原则
1. **单一职责** - 每个组件只负责一个功能
2. **组合优于继承** - 使用组合式函数和插槽
3. **Props向下，Events向上** - 单向数据流
4. **可测试性** - 组件逻辑与UI分离
5. **可复用性** - 通过props和插槽实现定制

### 数据流架构规范（单向数据流）

#### 数据流设计
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

### 测试规范（Vue Testing Library风格）

#### 测试结构
```
tests/
├── unit/                # 单元测试
│   ├── components/      # 组件测试
│   ├── composables/     # 组合式函数测试
│   └── stores/          # Store测试
├── integration/         # 集成测试
└── e2e/                # E2E测试
```

#### 组件测试示例
```typescript
// tests/unit/components/StateMachineEditor.spec.ts
import { describe, it, expect, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia } from 'pinia'
import StateMachineEditor from '@/components/StateMachineEditor.vue'
import type { StateMachineConfig } from '@/types/stateMachine'

describe('StateMachineEditor', () => {
  it('renders correctly with initial config', () => {
    const wrapper = mount(StateMachineEditor, {
      global: {
        plugins: [createPinia()]
      },
      props: {
        initialConfig: {
          initialState: 'IDLE',
          transitions: [],
          stateEffects: []
        }
      }
    })

    expect(wrapper.find('.state-machine-editor').exists()).toBe(true)
    expect(wrapper.text()).toContain('IDLE')
  })

  it('emits save event when save button is clicked', async () => {
    const wrapper = mount(StateMachineEditor, {
      global: {
        plugins: [createPinia()]
      }
    })

    await wrapper.find('[data-testid="save-button"]').trigger('click')
    
    expect(wrapper.emitted('save')).toBeTruthy()
  })
})
```

### 完整测试架构规范（分层测试体系）

#### 测试分层架构
```
tests/
├── unit/                    # 单元测试
│   ├── composables/         # 组合式函数测试
│   │   ├── useStateMachine.spec.ts
│   │   ├── useAPI.spec.ts
│   │   ├── useValidation.spec.ts
│   │   └── useNotifications.spec.ts
│   ├── stores/              # Store测试
│   │   ├── stateMachine.spec.ts
│   │   ├── device.spec.ts
│   │   └── ui.spec.ts
│   ├── services/            # 服务层测试
│   │   ├── stateMachine.spec.ts
│   │   ├── device.spec.ts
│   │   └── validation.spec.ts
│   ├── utils/               # 工具函数测试
│   │   ├── dataConverter.spec.ts
│   │   └── validators.spec.ts
│   └── components/          # 组件测试
│       ├── StateMachineEditor.spec.ts
│       ├── TransitionEditor.spec.ts
│       └── TriggerEditor.spec.ts
├── integration/             # 集成测试
│   ├── api-integration.spec.ts
│   ├── state-machine-integration.spec.ts
│   └── device-integration.spec.ts
├── e2e/                    # E2E测试
│   ├── state-machine.spec.ts
│   ├── device-control.spec.ts
│   └── user-workflows.spec.ts
├── fixtures/               # 测试数据
│   ├── stateMachines.ts
│   ├── devices.ts
│   └── apiResponses.ts
└── setup/                  # 测试设置
    ├── vitest.setup.ts
    ├── test-utils.ts
    └── mocks/
        ├── api.ts
        └── device.ts
```

#### 测试覆盖率目标
```
┌─────────────────────────────────────────────────┐
│              测试覆盖率目标              │
├─────────────────────────────────────────────────┤
│                                                 │
│  🎯 整体目标：                                   │
│  ├── 语句覆盖率：≥ 90%                          │
│  ├── 分支覆盖率：≥ 85%                          │
│  ├── 函数覆盖率：≥ 95%                          │
│  └── 行覆盖率：≥ 90%                            │
│                                                 │
│  📊 分层目标：                                   │
│  ├── Composables：≥ 95% (核心业务逻辑)          │
│  ├── Stores：≥ 90% (状态管理)                   │
│  ├── Services：≥ 95% (API层)                   │
│  ├── Components：≥ 85% (UI层)                  │
│  └── Utils：≥ 95% (工具函数)                   │
│                                                 │
│  🔍 关键路径：                                   │
│  ├── 状态转换逻辑：100%                          │
│  ├── 数据验证：100%                             │
│  ├── 错误处理：≥ 95%                            │
│  ├── API调用：≥ 90%                            │
│  └── 用户交互：≥ 85%                            │
│                                                 │
└─────────────────────────────────────────────────┘
```

#### 单元测试规范（Vitest + Vue Test Utils）
```typescript
// tests/unit/composables/useStateMachine.spec.ts
import { describe, it, expect, beforeEach, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia } from 'pinia'
import { useStateMachine } from '@/composables/useStateMachine'
import type { StateMachineConfig } from '@/types/stateMachine'

describe('useStateMachine', () => {
  beforeEach(() => {
    // 重置所有mock
    vi.clearAllMocks()
  })

  it('should initialize with default config', () => {
    const { config, isLoading, error } = useStateMachine()
    
    expect(config.value).toEqual(getDefaultConfig())
    expect(isLoading.value).toBe(false)
    expect(error.value).toBe(null)
  })

  it('should load config successfully', async () => {
    const mockConfig: StateMachineConfig = {
      initialState: 'TEST',
      transitions: [],
      stateEffects: []
    }
    
    vi.mocked(stateMachineService.getConfig).mockResolvedValue(mockConfig)
    
    const { config, loadConfig } = useStateMachine()
    await loadConfig()
    
    expect(config.value).toEqual(mockConfig)
    expect(stateMachineService.getConfig).toHaveBeenCalledOnce()
  })

  it('should handle load error', async () => {
    const errorMessage = 'Network error'
    vi.mocked(stateMachineService.getConfig).mockRejectedValue(new Error(errorMessage))
    
    const { error, loadConfig } = useStateMachine()
    await expect(loadConfig()).rejects.toThrow(errorMessage)
    
    expect(error.value).toBe(errorMessage)
  })

  it('should validate config correctly', () => {
    const { config, isValid } = useStateMachine()
    const invalidConfig = { ...config.value!, initialState: '' }
    
    config.value = invalidConfig
    
    expect(isValid.value).toBe(false)
  })
})
```

#### 组件测试规范（@testing-library/vue）
```typescript
// tests/unit/components/StateMachineEditor.spec.ts
import { describe, it, expect, beforeEach } from 'vitest'
import { render, screen, fireEvent, waitFor } from '@testing-library/vue'
import userEvent from '@testing-library/user-event'
import StateMachineEditor from '@/components/StateMachineEditor.vue'
import type { StateMachineConfig } from '@/types/stateMachine'

describe('StateMachineEditor', () => {
  const defaultConfig: StateMachineConfig = {
    initialState: 'IDLE',
    transitions: [],
    stateEffects: []
  }

  beforeEach(() => {
    render(StateMachineEditor, {
      props: { initialConfig: defaultConfig }
    })
  })

  it('should render initial state correctly', () => {
    expect(screen.getByDisplayValue('IDLE')).toBeInTheDocument()
    expect(screen.getByText('状态机配置')).toBeInTheDocument()
  })

  it('should add transition when add button is clicked', async () => {
    const addButton = screen.getByText('添加转换')
    await userEvent.click(addButton)
    
    await waitFor(() => {
      expect(screen.getByText('转换 1')).toBeInTheDocument()
    })
  })

  it('should validate config before save', async () => {
    const saveButton = screen.getByText('保存')
    await userEvent.click(saveButton)
    
    // 验证错误提示
    await waitFor(() => {
      expect(screen.getByText('配置验证失败')).toBeInTheDocument()
    })
  })

  it('should emit save event when valid config is saved', async () => {
    const { emitted } = render(StateMachineEditor, {
      props: { initialConfig: defaultConfig }
    })
    
    const saveButton = screen.getByText('保存')
    await userEvent.click(saveButton)
    
    await waitFor(() => {
      expect(emitted('save')).toBeTruthy()
      expect(emitted('save')[0][0]).toEqual(defaultConfig)
    })
  })
})
```

#### 集成测试规范
```typescript
// tests/integration/state-machine-integration.spec.ts
import { describe, it, expect, beforeEach, afterEach } from 'vitest'
import { setupServer } from 'msw/node'
import { rest } from 'msw'
import { createApp } from 'vue'
import { createPinia } from 'pinia'
import App from '@/App.vue'

const server = setupServer(
  rest.get('/api/v3/state-machine', (req, res, ctx) => {
    return res(ctx.json({
      data: getDefaultConfig(),
      success: true
    }))
  }),
  rest.post('/api/v3/state-machine', (req, res, ctx) => {
    return res(ctx.json({ success: true }))
  })
)

beforeEach(() => server.listen())
afterEach(() => server.resetHandlers())

describe('State Machine Integration', () => {
  it('should load and save config end-to-end', async () => {
    const app = createApp(App)
    app.use(createPinia())
    
    const container = document.createElement('div')
    document.body.appendChild(container)
    app.mount(container)
    
    // 模拟用户操作
    const loadButton = screen.getByText('加载配置')
    await userEvent.click(loadButton)
    
    await waitFor(() => {
      expect(screen.getByDisplayValue('IDLE')).toBeInTheDocument()
    })
    
    const saveButton = screen.getByText('保存')
    await userEvent.click(saveButton)
    
    await waitFor(() => {
      expect(screen.getByText('配置保存成功')).toBeInTheDocument()
    })
  })
})
```

#### E2E测试规范（Playwright）
```typescript
// tests/e2e/state-machine.spec.ts
import { test, expect } from '@playwright/test'

test.describe('State Machine E2E', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/state-machine-v3')
  })

  test('should complete full state machine workflow', async ({ page }) => {
    // 加载配置
    await page.click('[data-testid="load-config-button"]')
    await expect(page.locator('[data-testid="initial-state"]')).toHaveValue('IDLE')
    
    // 添加转换
    await page.click('[data-testid="add-transition-button"]')
    await page.fill('[data-testid="current-state-input"]', 'RUNNING')
    await page.fill('[data-testid="next-state-input"]', 'IDLE')
    
    // 保存配置
    await page.click('[data-testid="save-button"]')
    await expect(page.locator('.success-message')).toBeVisible()
    
    // 验证JSON预览
    const jsonPreview = await page.locator('[data-testid="json-preview"]').textContent()
    const config = JSON.parse(jsonPreview!)
    expect(config.initialState).toBe('IDLE')
    expect(config.transitions).toHaveLength(1)
  })

  test('should handle validation errors', async ({ page }) => {
    await page.click('[data-testid="add-transition-button"]')
    await page.click('[data-testid="save-button"]')
    
    await expect(page.locator('.error-message')).toBeVisible()
    await expect(page.locator('[data-testid="save-button"]')).toBeDisabled()
  })
})
```

#### 测试配置文件
```typescript
// vitest.config.ts
import { defineConfig } from 'vitest/config'
import vue from '@vitejs/plugin-vue'
import path from 'path'

export default defineConfig({
  plugins: [vue()],
  test: {
    globals: true,
    environment: 'jsdom',
    setupFiles: ['./tests/setup/vitest.setup.ts'],
    coverage: {
      provider: 'v8',
      reporter: ['text', 'json', 'html'],
      exclude: [
        'node_modules/',
        'tests/',
        '**/*.d.ts',
        '**/*.config.*'
      ],
      thresholds: {
        global: {
          branches: 85,
          functions: 95,
          lines: 90,
          statements: 90
        }
      }
    }
  },
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src')
    }
  }
})
```

### 性能优化规范

#### 优化策略
1. **组件懒加载** - 路由级别的代码分割
2. **虚拟滚动** - 大列表性能优化
3. **计算属性缓存** - 避免重复计算
4. **事件监听器清理** - 防止内存泄漏
5. **图片懒加载** - 减少初始加载时间

#### 实施要求
- 使用`defineAsyncComponent`进行异步组件加载
- 使用`v-memo`优化重复渲染
- 使用`shallowRef`和`shallowReactive`优化大对象
- 使用`onUnmounted`清理定时器和事件监听器
