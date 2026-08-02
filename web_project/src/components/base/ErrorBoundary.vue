<script setup lang="ts">
/**
 * 错误边界组件（Error Boundary）
 *
 * 【功能说明】
 * 捕获子组件渲染过程中的错误，显示友好的错误提示界面。
 * 防止单个组件的错误导致整个应用崩溃。
 *
 * 【核心概念】
 * 1. 错误捕获：通过 Vue 的 onErrorCaptured 钩子捕获子组件错误
 * 2. 降级显示：当捕获到错误时，显示错误提示卡片而非崩溃
 * 3. 重试机制：提供重试按钮，用户可以重新渲染子组件
 *
 * 【适用场景】
 * - 包裹可能出错的异步组件
 * - 包裹第三方组件
 * - 包裹复杂的业务组件
 *
 * Props:
 * - fallbackMessage: 错误提示文本（默认'组件加载失败'）
 * - showDetails: 是否显示错误详情（默认false）
 *
 * Events:
 * - retry: 重试事件，点击重试按钮时触发
 *
 * 使用示例：
 * <ErrorBoundary fallback-message="页面加载失败" :show-details="true">
 *   <MyComplexComponent />
 * </ErrorBoundary>
 */
import { ref, onErrorCaptured } from 'vue';
import BaseCard from './BaseCard.vue';
import BaseButton from './BaseButton.vue';

interface Props {
  fallbackMessage?: string;
  showDetails?: boolean;
}
withDefaults(defineProps<Props>(), {
  fallbackMessage: '组件加载失败',
  showDetails: false,
});

const hasError = ref(false);
const error = ref<Error | null>(null);
const errorInfo = ref<Error | null>(null);

onErrorCaptured((err, _instance, info) => {
  hasError.value = true;
  error.value = err as Error;
  errorInfo.value = info as Error;
  console.error('ErrorBoundary caught error:', err, info);
  return false;
});
/**
 * 处理子组件错误
 */
/**
 * 记录子组件错误，并切换到错误兜底展示状态。
 */
const handleError = (err: Error, info: any) => {
  hasError.value = true;
  error.value = err;
  errorInfo.value = info;
  console.error('ErrorBoundary caught error:', err, info);
};
/**
 * 重置错误状态
 */
/**
 * 清空当前错误状态，允许后续重新尝试渲染子组件。
 */
const resetError = () => {
  hasError.value = false;
  error.value = null;
  errorInfo.value = null;
};
/**
 * 触发重试事件
 */
/**
 * 触发重试逻辑，重置错误状态后重新渲染插槽内容。
 */
const handleRetry = () => {
  resetError();
};
</script>

<template>
  <div v-if="hasError" class="error-boundary">
    <BaseCard class="error-card">
      <div class="error-content">
        <div class="error-icon">
          <svg
            xmlns="http://www.w3.org/2000/svg"
            width="48"
            height="48"
            viewBox="0 0 24 24"
            fill="none"
            stroke="currentColor"
            stroke-width="2"
            stroke-linecap="round"
            stroke-linejoin="round"
            class="text-error"
          >
            <circle cx="12" cy="12" r="10"></circle>
            <line x1="12" y1="8" x2="12" y2="12"></line>
            <line x1="12" y1="16" x2="12" y2="16"></line>
          </svg>
        </div>
        <h3 class="error-title">{{ fallbackMessage }}</h3>
        <p v-if="error && showDetails" class="error-message">
          {{ error.message }}
        </p>
        <p v-if="errorInfo && showDetails" class="error-info">
          {{ errorInfo.info }}
        </p>
        <BaseButton variant="primary" @click="handleRetry"> 重试 </BaseButton>
      </div>
    </BaseCard>
  </div>
  <slot v-else />
</template>

<style scoped>
.error-boundary {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
}

.error-card {
  width: 100%;
  max-width: 400px;
  text-align: center;
}

.error-content {
  padding: 24px;
}

.error-icon {
  margin-bottom: 16px;
}

.error-title {
  margin: 0 0 8px 0;
  font-size: 18px;
  font-weight: 600;
  color: var(--color-text, #303133);
}

.error-message {
  margin: 0 0 8px 0;
  font-size: 14px;
  color: var(--color-error, #f56c6c);
  word-break: break-all;
}

.error-info {
  margin: 0 0 16px 0;
  font-size: 12px;
  color: var(--color-text-secondary, #909399);
  word-break: break-all;
  white-space: pre-wrap;
}
</style>
