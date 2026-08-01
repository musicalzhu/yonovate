<!--
   Vue 3.5+ 基础选择器组件

   【功能说明】
   提供可定制的下拉选择器组件，支持多种尺寸、验证和状态管理。
   使用 Vue 3.5+ useId() 生成唯一ID，避免SSR hydration错误。

   【核心概念】
   1. 选项系统：通过 options 属性传入选项列表
   2. 尺寸系统：sm、md、lg三种尺寸
   3. 状态管理：disabled（禁用）、error（错误状态）
   4. 验证支持：required（必填）、error（错误提示）、hint（提示信息）

   【适用场景】
   - 表单数据选择（GPIO模式、逻辑门类型等）
   - 配置选项选择
   - 筛选和过滤

   Props:
   - modelValue: 选中值（必需）
   - label: 标签文本（可选）
   - options: 选项列表（必需），格式：{ label: string, value: number | string }
   - disabled: 是否禁用（默认false）
   - required: 是否必填（默认false）
   - error: 错误提示文本（可选）
   - hint: 提示信息文本（可选）
   - size: 尺寸（sm, md, lg）

   Events:
   - update:modelValue: 值变化事件

   使用示例：
   <BaseSelect
     v-model="gpioMode"
     label="GPIO模式"
     :options="[
       { label: '输出', value: 0 },
       { label: '输入', value: 1 }
     ]"
     size="small"
   />
 -->
<template>
  <div :class="containerClasses">
    <label v-if="label" :for="selectId" class="base-select__label">
      {{ label }}
      <span v-if="required" class="base-select__required">*</span>
    </label>

    <div class="base-select__wrapper">
      <select
        :id="selectId"
        :class="selectClasses"
        :value="modelValue"
        :disabled="disabled"
        v-bind="$attrs"
        @change="handleChange"
      >
        <option v-for="option in options" :key="option.value" :value="option.value">
          {{ option.label }}
        </option>
      </select>
    </div>

    <span v-if="error" class="base-select__error">{{ error }}</span>
    <span v-if="hint" class="base-select__hint">{{ hint }}</span>
  </div>
</template>

<script setup lang="ts">
import { useId, computed } from 'vue';

interface SelectOption {
  label: string;
  value: number | string;
}

// Vue 3.5+ 响应式 Props 解构
const {
  modelValue,
  label,
  options,
  disabled = false,
  required = false,
  error = '',
  hint = '',
  size = 'md',
} = defineProps<{
  modelValue: number | string;
  label?: string;
  options: SelectOption[];
  disabled?: boolean;
  required?: boolean;
  error?: string;
  hint?: string;
  size?: 'sm' | 'md' | 'lg';
}>();

const emit = defineEmits<{
  'update:modelValue': [value: number | string];
}>();

const selectId = useId();

// 组合容器和选择框的样式类，便于统一控制状态样式。
const containerClasses = computed(() => [
  'base-select',
  `base-select--${size}`,
  {
    'base-select--disabled': disabled,
    'base-select--error': !!error,
  },
]);

// 选择框本身使用固定的基础样式名，避免重复声明。
const selectClasses = computed(() => ['base-select__select']);

/**
 * 把原生 select 的变更事件转成组件的更新事件。
 */
const handleChange = (event: Event) => {
  const target = event.target as HTMLSelectElement;
  emit('update:modelValue', target.value);
};
</script>

<style scoped>
:root {
  --base-select-bg: #ffffff;
  --base-select-border: #d1d5db;
  --base-select-focus: rgba(59, 130, 246, 0.5);
  --base-select-text: #1f2937;
  --base-select-label: #374151;
  --base-select-disabled: #f3f4f6;
  --base-select-error: #dc2626;
  --base-select-hint: #6b7280;
}

/* CSS 变量代理模式 - 尺寸变体 */
.base-select--sm {
  --select-padding: 0.375rem 0.5rem;
  --select-font: 0.875rem;
}
.base-select--md {
  --select-padding: 0.5rem 0.75rem;
  --select-font: 1rem;
}
.base-select--lg {
  --select-padding: 0.75rem 1rem;
  --select-font: 1.125rem;
}

.base-select {
  display: flex;
  flex-direction: column;
  gap: 0.25rem;
}

.base-select__label {
  font-size: 0.875rem;
  font-weight: 500;
  color: var(--base-select-label);
}

.base-select__required {
  color: var(--base-select-error);
  margin-left: 0.25rem;
}

.base-select__wrapper {
  position: relative;
}

.base-select__select {
  width: 100%;
  padding: var(--select-padding, 0.5rem 0.75rem);
  font-size: var(--select-font, 1rem);
  border: 1px solid var(--base-select-border);
  border-radius: 0.375rem;
  background-color: var(--base-select-bg);
  color: var(--base-select-text);
  outline: none;
  transition:
    box-shadow 0.2s,
    border-color 0.2s;
  appearance: none;
  cursor: pointer;
}

.base-select__select:focus {
  box-shadow: 0 0 0 2px var(--base-select-focus);
  border-color: transparent;
}

.base-select__select:disabled {
  background-color: var(--base-select-disabled);
  cursor: not-allowed;
}

.base-select__error {
  font-size: 0.875rem;
  color: var(--base-select-error);
}

.base-select__hint {
  font-size: 0.875rem;
  color: var(--base-select-hint);
}

.base-select--error .base-select__select {
  border-color: var(--base-select-error);
}

.base-select--disabled .base-select__select {
  opacity: 0.6;
}
</style>
