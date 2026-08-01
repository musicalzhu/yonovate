<!--
   Vue 3.5+ 基础卡片组件 (优化版)

   【功能说明】
   提供可定制的卡片容器组件，用于包裹和展示内容。
   支持多种变体、尺寸、样式和插槽，是页面布局的基础组件。

   【核心概念】
   1. 变体系统：default、outlined、elevated、flat四种视觉风格
   2. 尺寸系统：sm、md、lg三种文本尺寸
   3. 内边距控制：none、sm、md、lg四种内边距选项
   4. 插槽系统：header、default、footer三个插槽区域

   【适用场景】
   - 展示设备信息、GPIO状态、WiFi配置等数据
   - 构建页面布局和内容分组
   - 创建可复用的卡片式UI组件

   Props:
   - title: 卡片标题（可选）
   - variant: 卡片变体（default, outlined, elevated, flat）
   - size: 卡片文本尺寸（sm, md, lg）
   - padding: 内边距（none, sm, md, lg）
   - rounded: 是否圆角（默认true）
   - shadow: 是否显示阴影（默认true）
   - hover: 是否启用悬停效果（默认false）
   - bordered: 是否显示边框（默认false）
   - background: 背景颜色（white, gray, transparent）

   Slots:
   - default: 卡片主体内容（必需）
   - header: 卡片头部（可选）
   - footer: 卡片底部（可选）

   使用示例：
   <BaseCard title="设备信息" variant="elevated">
     <div>内容区域</div>
   </BaseCard>
 -->
<template>
  <div :class="cardClasses">
    <div v-if="title || $slots.header || $slots.actions" :class="headerClasses">
      <h3 v-if="title" class="base-card__title">{{ title }}</h3>
      <slot name="header" />
      <div v-if="$slots.actions" class="base-card__actions">
        <slot name="actions" />
      </div>
    </div>

    <div :class="bodyClasses">
      <slot />
    </div>

    <div v-if="$slots.footer" :class="footerClasses">
      <slot name="footer" />
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';

/**
 * 卡片组件属性
 */
interface Props {
  /** 卡片标题 */
  title?: string;
  /** 卡片变体 */
  variant?: 'default' | 'outlined' | 'elevated' | 'flat';
  /** 卡片文本尺寸 */
  size?: 'sm' | 'md' | 'lg';
  /** 内边距 */
  padding?: 'none' | 'sm' | 'md' | 'lg';
  /** 是否圆角 */
  rounded?: boolean;
  /** 是否显示阴影 */
  shadow?: boolean;
  /** 是否启用悬停效果 */
  hover?: boolean;
  /** 是否显示边框 */
  bordered?: boolean;
  /** 背景颜色 */
  background?: 'white' | 'gray' | 'transparent';
}

// Vue 3.5+ 响应式 Props 解构
const {
  title,
  variant = 'default',
  size = 'md',
  padding = 'md',
  rounded = true,
  shadow = true,
  hover = false,
  bordered = false,
  background = 'white',
} = defineProps<Props>();

/**
 * 计算卡片主体样式类。
 *
 * 通过变体、尺寸、背景和状态组合出最终的 CSS 类名，方便复用和扩展。
 */
const cardClasses = computed(() => {
  // 当 variant 为 elevated 时，自带阴影，忽略 shadow prop 避免冲突
  const isElevated = variant === 'elevated';

  return [
    'base-card',
    `base-card--variant-${variant}`,
    `base-card--size-${size}`,
    `base-card--bg-${background}`,
    {
      'base-card--rounded': rounded,
      'base-card--shadow': shadow && !isElevated,
      'base-card--hover': hover,
      'base-card--bordered': bordered,
    },
  ];
});

/**
 * 计算头部样式类。
 */
const headerClasses = computed(() => ['base-card__header', `base-card__padding-${padding}`]);

/**
 * 计算主体样式类。
 */
const bodyClasses = computed(() => ['base-card__body', `base-card__padding-${padding}`]);

/**
 * 计算底部样式类。
 */
const footerClasses = computed(() => ['base-card__footer', `base-card__padding-${padding}`]);
</script>

<style scoped>
:root {
  --card-bg-default: #ffffff;
  --card-border-default: #e5e7eb;
  --card-border-outlined: #d1d5db;
  --card-shadow-elevated: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -4px rgba(0, 0, 0, 0.1);
  --card-shadow-hover: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -4px rgba(0, 0, 0, 0.1);
  --card-shadow-normal: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -2px rgba(0, 0, 0, 0.1);
  --card-bg-flat: #f9fafb;
  --card-bg-gray: #f3f4f6;
  --card-text: #1f2937;
  --card-border-color: #e5e7eb;
}

/* CSS 变量代理模式 - 变体 */
.base-card--variant-default {
  --card-bg: var(--card-bg-default);
  --card-border: var(--card-border-default);
}
.base-card--variant-outlined {
  --card-bg: var(--card-bg-default);
  --card-border: var(--card-border-outlined);
  --card-border-width: 2px;
}
.base-card--variant-elevated {
  --card-bg: var(--card-bg-default);
  --card-border: transparent;
  --card-shadow: var(--card-shadow-elevated);
}
.base-card--variant-flat {
  --card-bg: var(--card-bg-flat);
  --card-border: transparent;
}

/* CSS 变量代理模式 - 背景色 */
.base-card--bg-white {
  --card-bg: #ffffff;
}
.base-card--bg-gray {
  --card-bg: var(--card-bg-gray);
}
.base-card--bg-transparent {
  --card-bg: transparent;
}

/* CSS 变量代理模式 - 尺寸 */
.base-card--size-sm {
  --card-font: 0.875rem;
}
.base-card--size-md {
  --card-font: 1rem;
}
.base-card--size-lg {
  --card-font: 1.125rem;
}

/* CSS 变量代理模式 - 内边距 */
.base-card__padding-none {
  --card-padding: 0;
}
.base-card__padding-sm {
  --card-padding: 0.5rem 0.75rem;
}
.base-card__padding-md {
  --card-padding: 0.75rem 1rem;
}
.base-card__padding-lg {
  --card-padding: 1rem 1.5rem;
}

/* ================= 基础布局 ================= */
.base-card {
  display: flex;
  flex-direction: column;
  font-size: var(--card-font, 1rem);
  background-color: var(--card-bg, var(--card-bg-default));
  border: var(--card-border-width, 1px) solid var(--card-border, var(--card-border-default));
  box-shadow: var(--card-shadow, none);
  /* 明确指定过渡属性，避免 transition: all 带来的性能损耗 */
  transition:
    box-shadow 0.2s ease,
    transform 0.2s ease,
    background-color 0.2s ease,
    border-color 0.2s ease;
}

/* ================= 修饰符 (Modifiers) ================= */
.base-card--rounded {
  border-radius: 0.5rem;
}

.base-card--shadow {
  box-shadow: var(--card-shadow-normal);
}

.base-card--bordered {
  border: 1px solid var(--card-border-outlined);
}

.base-card--hover:hover {
  box-shadow: var(--card-shadow-hover);
  transform: translateY(-2px); /* 使用 translateY 替代 scale，视觉更自然 */
}

/* ================= 内部结构 ================= */
.base-card__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: var(--card-padding, 0.75rem 1rem);
  border-bottom: 1px solid var(--card-border-color);
}

.base-card__actions {
  display: flex;
  gap: 0.5rem;
}

.base-card__title {
  margin: 0;
  font-size: 1.125rem;
  font-weight: 600;
  color: var(--card-text);
}

.base-card__body {
  flex: 1;
  padding: var(--card-padding, 0.75rem 1rem);
}

.base-card__footer {
  padding: var(--card-padding, 0.75rem 1rem);
  border-top: 1px solid var(--card-border-color);
}
</style>
