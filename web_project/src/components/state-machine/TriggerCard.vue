<!--
   Vue 3.5+ 触发器卡片组件

   【功能说明】
   展示单个触发器配置和状态的卡片组件，用于状态机监控页面。
   显示触发源、触发条件和当前状态。

   【核心概念】
   1. 触发类型：无效、边沿、电平、定时器、手动
   2. 触发条件：运算符和触发值的组合
   3. 状态指示：显示触发器当前是"触发中"还是"空闲"状态

   【适用场景】
   - 状态机监控页面（StateMachineV3Page.vue）
   - 触发器状态展示
   - 调试和监控

   Props:
   - modelValue: 触发器对象（必需）
   - isActive: 触发器是否激活（默认false）

   使用示例：
   <TriggerCard
     v-model="trigger"
     :is-active="trigger.isActive"
   />
 -->
<template>
  <div class="trigger-card">
    <BaseCard>
      <template #header>
        <div class="flex items-center justify-between">
          <span class="text-sm font-medium">触发器配置</span>
          <span class="text-xs px-2 py-1 rounded-full" :class="typeBadgeClass">
            {{ typeLabel }}
          </span>
        </div>
      </template>

      <div class="space-y-4">
        <!-- 触发源 -->
        <div class="flex items-center gap-3">
          <span class="text-sm font-medium w-24">触发源:</span>
          <span class="text-sm text-gray-700 font-mono">{{ modelValue.triggerSource }}</span>
        </div>

        <!-- 触发条件 -->
        <div class="flex items-center gap-3">
          <span class="text-sm font-medium w-24">条件:</span>
          <span class="text-sm text-gray-700">
            {{ operatorLabel }} {{ modelValue.triggerValue }}
          </span>
        </div>

        <!-- 状态指示 -->
        <div class="flex items-center gap-3">
          <span class="text-sm font-medium w-24">状态:</span>
          <span class="flex items-center gap-2">
            <span
              class="w-2 h-2 rounded-full"
              :class="isActive ? 'bg-green-500 animate-pulse' : 'bg-gray-300'"
            />
            <span class="text-sm" :class="isActive ? 'text-green-600' : 'text-gray-500'">
              {{ isActive ? '触发中' : '空闲' }}
            </span>
          </span>
        </div>
      </div>
    </BaseCard>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import type { Trigger } from '../../types/stateMachine';
import BaseCard from '../base/BaseCard.vue';

interface Props {
  modelValue: Trigger;
  isActive?: boolean;
}

const props = withDefaults(defineProps<Props>(), {
  isActive: false,
});

// 触发类型标签
const typeLabel = computed(() => {
  const types: Record<number, string> = {
    0: '无效',
    1: '边沿',
    2: '电平',
    3: '定时器',
    4: '手动',
  };
  return types[props.modelValue.triggerType] || '未知';
});

// 触发类型徽章样式
const typeBadgeClass = computed(() => {
  const classes: Record<number, string> = {
    0: 'bg-gray-100 text-gray-600',
    1: 'bg-blue-100 text-blue-600',
    2: 'bg-green-100 text-green-600',
    3: 'bg-yellow-100 text-yellow-600',
    4: 'bg-purple-100 text-purple-600',
  };
  return classes[props.modelValue.triggerType] || 'bg-gray-100 text-gray-600';
});

// 运算符标签
const operatorLabel = computed(() => {
  const operators: Record<number, string> = {
    0: '==',
    1: '<',
    2: '>',
    3: '<=',
    4: '>=',
    5: '!=',
  };
  return operators[props.modelValue.operator] || '==';
});
</script>

<style scoped>
.trigger-card {
  width: 100%;
}
</style>
