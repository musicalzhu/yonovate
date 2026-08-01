<!--
   Vue 3.5+ 状态指示器组件

   【功能说明】
   显示状态机状态的标签组件，用于状态机配置和监控页面。
   支持标记当前状态和活跃状态。

   【核心概念】
   1. 当前状态：标记为初始状态的状态（蓝色背景）
   2. 活跃状态：当前正在执行的状态（绿色背景，脉冲动画）

   【适用场景】
   - 状态机配置页面
   - 状态机监控页面
   - 状态转换列表

   Props:
   - state: 状态名称（必需）
   - isCurrent: 是否为初始状态（默认false）
   - isActive: 是否为活跃状态（默认false）

   使用示例：
   <StateIndicator state="IDLE" :is-current="true" />
   <StateIndicator state="RUNNING" :is-active="true" />
 -->
<template>
  <div
    class="state-indicator"
    :class="{
      'state-indicator--current': isCurrent,
      'state-indicator--active': isActive,
    }"
  >
    <span class="state-indicator__label">{{ state }}</span>
    <span v-if="isCurrent" class="state-indicator__badge">初始</span>
    <span v-if="isActive" class="state-indicator__badge state-indicator__badge--active">活跃</span>
  </div>
</template>

<script setup lang="ts">
interface Props {
  state: string;
  isCurrent?: boolean;
  isActive?: boolean;
}

withDefaults(defineProps<Props>(), {
  isCurrent: false,
  isActive: false,
});
</script>

<style scoped>
.state-indicator {
  display: inline-flex;
  align-items: center;
  gap: 0.5rem;
  padding: 0.5rem 1rem;
  border-radius: 9999px;
  background-color: #f3f4f6;
  color: #374151;
  transition: all 0.2s;
}

.state-indicator--current {
  background-color: #dbeafe;
  color: #1e40af;
  box-shadow: 0 0 0 2px #93c5fd;
}

.state-indicator--active {
  background-color: #dcfce7;
  color: #166534;
  animation: pulse 2s cubic-bezier(0.4, 0, 0.6, 1) infinite;
}

.state-indicator__label {
  font-size: 0.875rem;
  font-weight: 500;
}

.state-indicator__badge {
  padding: 0.125rem 0.5rem;
  font-size: 0.75rem;
  border-radius: 9999px;
  background-color: #bfdbfe;
  color: #1e40af;
}

.state-indicator__badge--active {
  background-color: #bbf7d0;
  color: #166534;
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
