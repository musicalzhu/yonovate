<!--
   Vue 3.5+ 触发器编辑器组件

   【功能说明】
   编辑单个触发器条件的组件，是状态转换编辑器的子组件。
   支持配置触发类型、触发源、运算符和触发值。

   【核心概念】
   1. 触发类型：GPIO、TIMER、MANUAL、LOGIC等
   2. 触发源：GPIO端口（P0-P11）、定时器（T0-T2）、MANUAL
   3. 运算符：EQ（等于）、NEQ（不等于）、GT（大于）、LT（小于）等
   4. 触发值：触发条件的比较值

   【数据流程】
   用户编辑触发器 → 更新本地 localValue 对象 → 触发 update:model-value 事件 →
   父组件更新触发器列表

   【适用场景】
   - 状态转换编辑器（TransitionEditor.vue）内部使用
   - 编辑单个触发器条件

   Props:
   - modelValue: 触发器对象（必需，双向绑定）
   - index: 触发器在列表中的索引（显示序号）
   - canRemove: 是否可删除（默认true）

   Emits:
   - update:model-value: 触发器变化事件
   - remove: 删除触发器事件

   使用示例：
   <TriggerEditor
     v-model="trigger"
     :index="index"
     :can-remove="triggers.length > 1"
     @update:model-value="updateTrigger(index, $event)"
     @remove="removeTrigger(index)"
   />
 -->
<template>
  <div class="trigger-editor">
    <div class="border rounded-lg p-4 bg-gray-50">
      <div class="flex items-center justify-between mb-3">
        <span class="text-sm font-medium">触发器 {{ index + 1 }}</span>
        <BaseButton v-if="canRemove" variant="ghost" size="xs" @click="$emit('remove')">
          删除
        </BaseButton>
      </div>

      <div class="grid grid-cols-2 gap-4">
        <!-- 触发类型 -->
        <BaseSelect
          :model-value="localValue.triggerType"
          label="触发类型"
          :options="triggerTypeOptions"
          @update:model-value="updateField('triggerType', $event)"
        />

        <!-- 触发源 -->
        <BaseInput
          :model-value="localValue.triggerSource"
          label="触发源"
          placeholder="P0-P11, T0-T2, MANUAL"
          @update:model-value="updateField('triggerSource', $event)"
        />

        <!-- 运算符 -->
        <BaseSelect
          :model-value="localValue.operator"
          label="运算符"
          :options="operatorOptions"
          @update:model-value="updateField('operator', $event)"
        />

        <!-- 触发值 -->
        <BaseInput
          :model-value="localValue.triggerValue"
          label="触发值"
          type="number"
          @update:model-value="updateField('triggerValue', $event)"
        />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import type { Trigger } from '../../types/stateMachine';
import BaseButton from '../base/BaseButton.vue';
import BaseInput from '../base/BaseInput.vue';
import BaseSelect from '../base/BaseSelect.vue';

// 使用 Vue 3.5+ defineModel 简化双向绑定
const model = defineModel<Trigger>('modelValue', {
  required: true,
});

interface Props {
  index: number;
  canRemove?: boolean;
}

withDefaults(defineProps<Props>(), {
  canRemove: true,
});

defineEmits<{
  remove: [];
}>();

// 本地值引用
const localValue = computed({
  get: () => model.value,
  set: (value) => {
    model.value = value;
  },
});

// 触发类型选项
const triggerTypeOptions = [
  { label: '无效', value: 0 },
  { label: '边沿', value: 1 },
  { label: '电平', value: 2 },
  { label: '定时器', value: 3 },
  { label: '手动', value: 4 },
];

// 运算符选项
const operatorOptions = [
  { label: '==', value: 0 },
  { label: '<', value: 1 },
  { label: '>', value: 2 },
  { label: '<=', value: 3 },
  { label: '>=', value: 4 },
  { label: '!=', value: 5 },
];

/**
 * 更新当前触发器对象的单个字段。
 */
const updateField = <K extends keyof Trigger>(key: K, value: Trigger[K]) => {
  model.value = { ...model.value, [key]: value };
};
</script>

<style scoped>
.trigger-editor {
  width: 100%;
}
</style>
