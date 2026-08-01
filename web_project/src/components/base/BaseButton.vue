<!--
   Vue 3.5+ 基础按钮组件 (极致优化版)

   【功能说明】
   提供可定制的按钮组件，支持多种变体、尺寸和状态。
   集成加载状态、禁用状态和块级显示功能。

   【核心概念】
   1. 变体系统：primary、secondary、success、warning、danger、ghost六种按钮风格
   2. 尺寸系统：sm、md、lg三种按钮尺寸
   3. 状态管理：loading（加载中）、disabled（禁用）
   4. 类型支持：button、submit、reset三种HTML按钮类型

   【适用场景】
   - 表单提交和取消操作
   - 数据刷新和保存操作
   - 导航和交互触发

   Props:
   - variant: 按钮变体（primary, secondary, success, warning, danger, ghost）
   - size: 按钮尺寸（sm, md, lg）
   - type: HTML按钮类型（button, submit, reset）
   - disabled: 是否禁用（默认false）
   - loading: 是否加载中（默认false）
   - block: 是否块级显示（默认false）

   Events:
   - click: 点击事件，仅在非禁用和非加载状态时触发

   使用示例：
   <BaseButton variant="primary" @click="handleClick">提交</BaseButton>
   <BaseButton variant="danger" loading>删除中...</BaseButton>
 -->
<template>
  <button
    :class="buttonClasses"
    :disabled="disabled || loading"
    :type="type"
    @click="emit('click', $event)"
  >
    <!-- 保持文本占位，防止 Loading 时布局抖动 -->
    <span class="btn-content" :class="{ 'is-loading': loading }">
      <slot />
    </span>
    <!-- Loading 图标绝对定位居中 -->
    <LoadingSpinner v-if="loading" class="btn-spinner" :size="loadingSize" />
  </button>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import LoadingSpinner from '../ui/LoadingSpinner.vue';

// 优化1：使用 Vue 3.5+ 响应式 Props 解构，告别 withDefaults 和 props. 前缀
const {
  variant = 'primary',
  size = 'md',
  type = 'button',
  disabled = false,
  loading = false,
  block = false,
} = defineProps<{
  variant?: 'primary' | 'secondary' | 'success' | 'warning' | 'danger' | 'ghost';
  size?: 'sm' | 'md' | 'lg';
  type?: 'button' | 'submit' | 'reset';
  disabled?: boolean;
  loading?: boolean;
  block?: boolean;
}>();

const emit = defineEmits<{
  click: [event: MouseEvent];
}>();

// 结合变体、尺寸和状态生成按钮样式类。
const buttonClasses = computed(() => ({
  'base-button': true,
  [`base-button--${variant}`]: true,
  [`base-button--${size}`]: true,
  'base-button--block': block,
  'base-button--disabled': disabled,
  'base-button--loading': loading,
}));

// 将加载中的尺寸与按钮尺寸保持一致，避免布局抖动。
const loadingSize = computed(() => (size === 'sm' ? 'xs' : 'sm'));
</script>

<style scoped>
.base-button--primary {
  --btn-bg: #7c3aed;
  --btn-hover: #6d28d9;
}
.base-button--secondary {
  --btn-bg: #e5e7eb;
  --btn-hover: #d1d5db;
  --btn-text: #111827;
}
.base-button--success {
  --btn-bg: #16a34a;
  --btn-hover: #15803d;
}
.base-button--warning {
  --btn-bg: #eab308;
  --btn-hover: #ca8a04;
}
.base-button--danger {
  --btn-bg: #dc2626;
  --btn-hover: #b91c1c;
}
.base-button--ghost {
  --btn-bg: transparent;
  --btn-hover: #f3f4f6;
  --btn-text: #374151;
  --btn-border: #d1d5db;
}

.base-button--sm {
  --btn-padding: 0.375rem 0.75rem;
  --btn-font: 0.875rem;
}
.base-button--md {
  --btn-padding: 0.5rem 1rem;
  --btn-font: 1rem;
}
.base-button--lg {
  --btn-padding: 0.75rem 1.5rem;
  --btn-font: 1.125rem;
}

.base-button {
  position: relative;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 0.5rem;
  padding: var(--btn-padding);
  font-size: var(--btn-font);
  font-weight: 500;
  color: var(--btn-text, white);
  background-color: var(--btn-bg);
  border: 1px solid var(--btn-border, transparent);
  border-radius: 0.375rem;
  cursor: pointer;
  transition: all 0.2s;
}

.base-button:hover:not(:disabled) {
  background-color: var(--btn-hover);
}

.base-button:focus-visible {
  outline: none;
  box-shadow: 0 0 0 2px rgba(124, 58, 237, 0.5);
}

.base-button:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.base-button--block {
  width: 100%;
}

.btn-content.is-loading {
  visibility: hidden;
}
.btn-spinner {
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
}
</style>
