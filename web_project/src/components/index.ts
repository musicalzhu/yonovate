/**
 * Vue 3.5+ 分层组件架构入口
 *
 * 【功能说明】
 * 统一导出所有可复用的组件，支持按需导入和全局注册。
 * 采用分层架构组织组件：base（基础组件）、state-machine（状态机组件）、ui（UI组件）。
 *
 * 【组件分层】
 * 1. base - 基础组件：可复用的通用UI组件
 *    - BaseButton: 按钮组件
 *    - BaseInput: 输入框组件
 *    - BaseCard: 卡片容器组件
 *    - BaseModal: 模态框组件
 *    - BaseSelect: 选择器组件
 *    - GPIOControl: GPIO控制组件
 *    - WiFiConfig: WiFi配置组件
 *    - ErrorBoundary: 错误边界组件
 *
 * 2. state-machine - 状态机组件：状态机配置和监控相关
 *    - StateMachineEditor: 状态机编辑器
 *    - TriggerCard: 触发器卡片
 *    - TransitionEditor: 转换编辑器
 *    - StateIndicator: 状态指示器
 *
 * 3. ui - UI组件：通用UI辅助组件
 *    - NotificationToast: 通知提示
 *    - LoadingSpinner: 加载动画
 *
 * 【使用示例】
 * ```typescript
 * // 按需导入
 * import { BaseButton, BaseCard } from '@/components'
 *
 * // 全局注册（在 main.ts 中）
 * import * as Components from '@/components'
 * Object.entries(Components).forEach(([name, component]) => {
 *   app.component(name, component)
 * })
 * ```
 */
export { default as BaseButton } from './base/BaseButton.vue';
export { default as BaseInput } from './base/BaseInput.vue';
export { default as BaseCard } from './base/BaseCard.vue';
export { default as BaseModal } from './base/BaseModal.vue';
export { default as BaseSelect } from './base/BaseSelect.vue';
export { default as GPIOControl } from './base/GPIOControl.vue';
export { default as WiFiConfig } from './base/WiFiConfig.vue';
export { default as ErrorBoundary } from './base/ErrorBoundary.vue';

export { default as StateMachineEditor } from './state-machine/StateMachineEditor.vue';
export { default as TriggerCard } from './state-machine/TriggerCard.vue';
export { default as TransitionEditor } from './state-machine/TransitionEditor.vue';
export { default as StateIndicator } from './state-machine/StateIndicator.vue';

export { default as NotificationToast } from './ui/NotificationToast.vue';
export { default as LoadingSpinner } from './ui/LoadingSpinner.vue';
