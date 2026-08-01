<!--
   Vue 3.5+ 基础输入组件

   【功能说明】
   提供可定制的输入框组件，支持多种输入类型、验证和状态管理。
   使用 Vue 3.5+ defineModel 简化双向绑定。

   【核心概念】
   1. 输入类型：text、password、email、number、tel、url
   2. 状态管理：loading（加载中）、disabled（禁用）、readonly（只读）
   3. 验证支持：required（必填）、error（错误提示）、hint（提示信息）
   4. 清除功能：clearable 属性提供快速清除按钮

   【适用场景】
   - 表单数据输入（设备IP、WiFi SSID、密码等）
   - 数据验证和错误提示
   - 搜索和筛选输入

   Props:
   - label: 输入框标签（可选）
   - type: 输入类型（text, password, email, number, tel, url）
   - placeholder: 占位符文本
   - disabled: 是否禁用
   - readonly: 是否只读
   - required: 是否必填
   - clearable: 是否可清除
   - loading: 是否加载中
   - error: 错误提示文本
   - hint: 提示信息文本
   - maxLength: 最大长度
   - min: 最小值（number类型）
   - max: 最大值（number类型）
   - step: 步长（number类型）
   - size: 输入框尺寸（sm, md, lg）
   - variant: 输入框变体（default, outlined, filled）

   Events:
   - blur: 失焦事件
   - focus: 获焦事件
   - input: 输入事件

   使用示例：
   <BaseInput
     v-model="deviceIp"
     label="设备IP地址"
     placeholder="192.168.3.55"
     required
     :error="ipError"
   />

   <BaseInput
     v-model="wifiPassword"
     type="password"
     label="WiFi密码"
     clearable
   />
 -->
<template>
  <div :class="containerClasses">
    <label v-if="label" :for="inputId" :class="labelClasses">
      {{ label }}
      <span v-if="required" class="base-input__required">*</span>
    </label>

    <div class="base-input__wrapper">
      <input
        :id="inputId"
        ref="inputRef"
        :class="inputClasses"
        :type="type"
        :placeholder="placeholder"
        :disabled="disabled"
        :readonly="readonly"
        :value="modelValue"
        :maxlength="maxLength"
        :min="min"
        :max="max"
        :step="step"
        v-bind="$attrs"
        @input="handleInput"
        @blur="handleBlur"
        @focus="handleFocus"
      />

      <div
        v-if="clearable && modelValue !== null && modelValue !== ''"
        class="base-input__clear"
        @click="handleClear"
      >
        ×
      </div>

      <div v-if="loading" class="base-input__loading">
        <LoadingSpinner size="sm" />
      </div>
    </div>

    <div v-if="error" class="base-input__error">
      {{ error }}
    </div>

    <div v-if="hint" class="base-input__hint">
      {{ hint }}
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, nextTick, useId } from 'vue';
import LoadingSpinner from '../ui/LoadingSpinner.vue';

// 使用 Vue 3.5+ defineModel 简化双向绑定
const model = defineModel<string | number | null>('modelValue', {
  default: '',
  required: false,
});

// Vue 3.5+ 响应式 Props 解构
const {
  label,
  type = 'text',
  placeholder,
  disabled = false,
  readonly = false,
  required = false,
  clearable = false,
  loading = false,
  error,
  hint,
  maxLength,
  min,
  max,
  step,
  size = 'md',
  variant = 'default',
} = defineProps<{
  label?: string;
  type?: 'text' | 'password' | 'email' | 'number' | 'tel' | 'url';
  placeholder?: string;
  disabled?: boolean;
  readonly?: boolean;
  required?: boolean;
  clearable?: boolean;
  loading?: boolean;
  error?: string;
  hint?: string;
  maxLength?: number;
  min?: number;
  max?: number;
  step?: number;
  size?: 'sm' | 'md' | 'lg';
  variant?: 'default' | 'outlined' | 'filled';
}>();

const emit = defineEmits<{
  blur: [event: FocusEvent];
  focus: [event: FocusEvent];
  input: [event: Event];
  clear: [];
}>();

const inputRef = ref<HTMLInputElement>();
// 使用 useId() 替代 Math.random()，消除 SSR 水合报错隐患
const inputId = useId();

// 计算是否需要显示后缀元素（清除按钮或加载状态）
const hasSuffix = computed(
  () => loading || (clearable && model.value !== null && model.value !== '')
);

const containerClasses = computed(() => {
  return [
    'base-input',
    `base-input--${size}`,
    `base-input--${variant}`,
    {
      'base-input--disabled': disabled,
      'base-input--error': error,
      'base-input--readonly': readonly,
    },
  ];
});

const labelClasses = computed(() => {
  return [
    'base-input__label',
    {
      'base-input__label--required': required,
    },
  ];
});

const inputClasses = computed(() => {
  return [
    'base-input__field',
    {
      'base-input__field--with-suffix': hasSuffix.value,
    },
  ];
});

const modelValue = computed({
  get: () => model.value,
  set: (value) => {
    model.value = value;
  },
});

/**
 * 处理输入框内容变化事件。
 *
 * 对于数字类型输入，会把空字符串转为 null，避免无效值进入表单模型。
 */
const handleInput = (event: Event) => {
  const target = event.target as HTMLInputElement;
  let value: string | number | null = target.value;

  if (type === 'number') {
    // 数字类型：空字符串转为 null，否则转为数字
    value = target.value === '' ? null : parseFloat(target.value) || null;
  }

  model.value = value;
  emit('input', event);
};

/**
 * 处理输入框失焦事件，并向父组件透传原始事件。
 */
const handleBlur = (event: FocusEvent) => {
  emit('blur', event);
};

/**
 * 处理输入框获得焦点事件，并向父组件透传原始事件。
 */
const handleFocus = (event: FocusEvent) => {
  emit('focus', event);
};

/**
 * 清空当前输入内容，并把焦点恢复到输入框。
 */
const handleClear = () => {
  // 清空时设置为 null，符合真实业务场景
  model.value = null;
  emit('clear');

  nextTick(() => {
    inputRef.value?.focus();
  });
};

/**
 * 让父组件可直接调用输入框聚焦。
 */
const focus = () => {
  inputRef.value?.focus();
};

const blur = () => {
  inputRef.value?.blur();
};

const select = () => {
  inputRef.value?.select();
};

// 暴露原生 DOM 引用，方便父组件在极端场景下直接操作
defineExpose({
  focus,
  blur,
  select,
  inputEl: inputRef,
});
</script>

<style scoped>
:root {
  --base-input-bg: #ffffff;
  --base-input-border: #d1d5db;
  --base-input-focus: rgba(59, 130, 246, 0.5);
  --base-input-text: #1f2937;
  --base-input-label: #374151;
  --base-input-placeholder: #9ca3af;
  --base-input-disabled: #f3f4f6;
  --base-input-error: #dc2626;
  --base-input-hint: #6b7280;
}

/* CSS 变量代理模式 - 尺寸变体 */
.base-input--sm {
  --input-padding: 0.375rem 0.5rem;
  --input-font: 0.875rem;
}
.base-input--md {
  --input-padding: 0.5rem 0.75rem;
  --input-font: 1rem;
}
.base-input--lg {
  --input-padding: 0.75rem 1rem;
  --input-font: 1.125rem;
}

/* CSS 变量代理模式 - 样式变体 */
.base-input--default {
  --input-bg: var(--base-input-bg);
  --input-border: var(--base-input-border);
}
.base-input--outlined {
  --input-bg: var(--base-input-bg);
  --input-border: var(--base-input-border);
}
.base-input--filled {
  --input-bg: var(--base-input-disabled);
  --input-border: transparent;
}

.base-input {
  display: flex;
  flex-direction: column;
  gap: 0.25rem;
}

.base-input__label {
  display: block;
  font-size: 0.875rem;
  font-weight: 500;
  color: var(--base-input-label);
  margin-bottom: 0.25rem;
}

.base-input__required {
  color: var(--base-input-error);
  margin-left: 0.25rem;
}

.base-input__wrapper {
  position: relative;
}

.base-input__field {
  width: 100%;
  padding: var(--input-padding, 0.5rem 0.75rem);
  font-size: var(--input-font, 1rem);
  border: 1px solid var(--input-border, var(--base-input-border));
  border-radius: 0.375rem;
  background-color: var(--input-bg, var(--base-input-bg));
  color: var(--base-input-text);
  transition:
    box-shadow 0.2s,
    border-color 0.2s;
}

.base-input__field::placeholder {
  color: var(--base-input-placeholder);
}

.base-input__field:focus {
  outline: none;
  box-shadow: 0 0 0 2px var(--base-input-focus);
  border-color: transparent;
}

.base-input__field:disabled {
  background-color: var(--base-input-disabled);
  cursor: not-allowed;
}

/* 统一后缀样式：清除按钮和加载状态共享同一套 padding */
.base-input__field--with-suffix {
  padding-right: 2.5rem;
}

.base-input__clear {
  position: absolute;
  right: 0.75rem;
  top: 50%;
  transform: translateY(-50%);
  color: var(--base-input-placeholder);
  cursor: pointer;
  width: 1rem;
  height: 1rem;
  display: flex;
  align-items: center;
  justify-content: center;
}

.base-input__clear:hover {
  color: var(--base-input-text);
}

.base-input__loading {
  position: absolute;
  right: 0.75rem;
  top: 50%;
  transform: translateY(-50%);
  width: 1rem;
  height: 1rem;
  display: flex;
  align-items: center;
  justify-content: center;
}

.base-input__error {
  font-size: 0.875rem;
  color: var(--base-input-error);
  margin-top: 0.25rem;
}

.base-input__hint {
  font-size: 0.875rem;
  color: var(--base-input-hint);
  margin-top: 0.25rem;
}

.base-input--error .base-input__field {
  border-color: var(--base-input-error);
}

.base-input--error .base-input__field:focus {
  box-shadow: 0 0 0 2px color-mix(in srgb, var(--base-input-error) 50%, transparent);
}

.base-input--disabled .base-input__field {
  background-color: var(--base-input-disabled);
  color: var(--base-input-hint);
}

.base-input--readonly .base-input__field {
  background-color: var(--base-input-disabled);
  cursor: default;
}
</style>
