<!--
   Vue 3.5+ 加载动画组件

   【功能说明】
   提供可定制的加载动画组件，支持多种尺寸、颜色和动画类型。
   用于在数据加载、操作处理时显示加载状态。

   【核心概念】
   1. 尺寸系统：xs、sm、md、lg、xl五种尺寸
   2. 颜色系统：primary、secondary、white、gray四种颜色
   3. 动画类型：spinner（旋转）、dots（点状）、pulse（脉冲）
   4. 遮罩模式：overlay 属性可创建全屏遮罩效果

   【适用场景】
   - 按钮加载状态（BaseButton组件内部使用）
   - 页面加载状态
   - 数据请求加载状态
   - 全屏加载遮罩

   Props:
   - size: 尺寸（xs, sm, md, lg, xl）
   - color: 颜色（primary, secondary, white, gray）
   - type: 动画类型（spinner, dots, pulse）
   - text: 加载提示文本（可选）
   - overlay: 是否全屏遮罩（默认false）

   使用示例：
   <LoadingSpinner size="sm" />
   <LoadingSpinner size="lg" text="加载中..." overlay />
-->
<template>
  <div :class="spinnerClasses" :style="spinnerStyle">
    <div class="loading-spinner__circle"></div>
    <div v-if="text" class="loading-spinner__text">{{ text }}</div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';

interface Props {
  size?: 'xs' | 'sm' | 'md' | 'lg' | 'xl';
  color?: 'primary' | 'secondary' | 'white' | 'gray';
  type?: 'spinner' | 'dots' | 'pulse';
  text?: string;
  overlay?: boolean;
}

const props = withDefaults(defineProps<Props>(), {
  size: 'md',
  color: 'primary',
  type: 'spinner',
  overlay: false,
});

const spinnerClasses = computed(() => {
  return [
    'loading-spinner',
    `loading-spinner--${props.size}`,
    `loading-spinner--${props.color}`,
    `loading-spinner--${props.type}`,
    {
      'loading-spinner--overlay': props.overlay,
    },
  ];
});

const spinnerStyle = computed(() => {
  if (props.overlay) {
    return {
      position: 'fixed' as const,
      top: '0',
      left: '0',
      right: '0',
      bottom: '0',
      zIndex: '9999',
      backgroundColor: 'rgba(255, 255, 255, 0.8)',
      display: 'flex',
      alignItems: 'center',
      justifyContent: 'center',
    };
  }
  return {};
});
</script>

<style scoped>
.loading-spinner {
  display: flex;
  align-items: center;
  justify-content: center;
}

.loading-spinner--xs {
  width: 1rem;
  height: 1rem;
}

.loading-spinner--sm {
  width: 1.5rem;
  height: 1.5rem;
}

.loading-spinner--md {
  width: 2rem;
  height: 2rem;
}

.loading-spinner--lg {
  width: 3rem;
  height: 3rem;
}

.loading-spinner--xl {
  width: 4rem;
  height: 4rem;
}

.loading-spinner--primary .loading-spinner__circle {
  border-color: #2563eb;
}

.loading-spinner--secondary .loading-spinner__circle {
  border-color: #4b5563;
}

.loading-spinner--white .loading-spinner__circle {
  border-color: white;
}

.loading-spinner--gray .loading-spinner__circle {
  border-color: #9ca3af;
}

.loading-spinner--spinner .loading-spinner__circle {
  border: 2px solid currentColor;
  border-top-color: transparent;
  border-radius: 9999px;
  animation: spin 1s linear infinite;
}

.loading-spinner--dots .loading-spinner__circle {
  display: flex;
  gap: 0.25rem;
}

.loading-spinner--dots .loading-spinner__circle::before,
.loading-spinner--dots .loading-spinner__circle::after {
  content: '';
  width: 0.5rem;
  height: 0.5rem;
  background-color: currentColor;
  border-radius: 9999px;
  animation: dot-pulse 1.4s infinite ease-in-out both;
}

.loading-spinner--dots .loading-spinner__circle::after {
  animation-delay: -0.32s;
}

.loading-spinner--pulse .loading-spinner__circle {
  background-color: currentColor;
  border-radius: 9999px;
  animation: pulse 2s cubic-bezier(0.4, 0, 0.6, 1) infinite;
}

.loading-spinner__text {
  margin-left: 0.5rem;
  font-size: 0.875rem;
  color: #4b5563;
}

@keyframes spin {
  to {
    transform: rotate(360deg);
  }
}

@keyframes dot-pulse {
  0%,
  80%,
  100% {
    transform: scale(0);
  }
  40% {
    transform: scale(1);
  }
}

@keyframes pulse {
  0%,
  100% {
    opacity: 1;
  }
  50% {
    opacity: 0.5;
  }
}
</style>
