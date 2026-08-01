<!--
   Vue 3.5+ 状态转换编辑器组件

   【功能说明】
   编辑单个状态转换规则的组件，是状态机编辑器的子组件。
   支持配置当前状态、下一状态、触发次数、触发逻辑和触发器列表。

   【核心概念】
   1. 当前状态：转换开始时的状态名称
   2. 下一状态：转换完成后的状态名称
   3. 触发次数：支持1-3个触发器
   4. 触发逻辑：AND（所有触发器满足）或 OR（任一触发器满足）
   5. 触发器：定义具体的触发条件（GPIO、定时器、手动等）

   【数据流程】
   用户编辑转换 → 更新本地 localValue 对象 → 触发 update:model-value 事件 →
   父组件更新转换列表

   【适用场景】
   - 状态机编辑器（StateMachineEditor.vue）内部使用
   - 编辑单个状态转换规则

   Props:
   - modelValue: 转换对象（必需，双向绑定）
   - availableStates: 可用状态列表（用于验证）
   - index: 转换在列表中的索引

   Emits:
   - update:model-value: 转换变化事件
   - remove: 删除转换事件

   使用示例：
   <TransitionEditor
     v-model="transition"
     :available-states="availableStates"
     @update:model-value="updateTransition(index, $event)"
     @remove="removeTransition(index)"
   />
 -->
<template>
  <div class="transition-editor">
    <BaseCard>
      <template #header>
        <div class="flex items-center justify-between">
          <span class="text-sm font-medium">状态转换</span>
          <BaseButton variant="ghost" size="sm" @click="$emit('remove')"> 删除 </BaseButton>
        </div>
      </template>

      <div class="space-y-4">
        <!-- 当前状态 -->
        <BaseInput
          :model-value="localValue.currentState"
          label="当前状态"
          placeholder="例如: IDLE"
          @update:model-value="updateField('currentState', $event)"
        />

        <!-- 下一状态 -->
        <BaseInput
          :model-value="localValue.nextState"
          label="下一状态"
          placeholder="例如: RUNNING"
          @update:model-value="updateField('nextState', $event)"
        />

        <!-- 触发次数 -->
        <div class="grid grid-cols-2 gap-4">
          <BaseInput
            :model-value="localValue.triggerCount"
            label="触发次数"
            type="number"
            :min="1"
            :max="3"
            @update:model-value="updateField('triggerCount', $event)"
          />

          <!-- 触发逻辑 -->
          <BaseSelect
            :model-value="localValue.triggerLogic"
            label="触发逻辑"
            :options="triggerLogicOptions"
            @update:model-value="updateField('triggerLogic', $event)"
          />
        </div>

        <!-- 触发器列表 -->
        <div class="space-y-2">
          <div class="flex items-center justify-between">
            <span class="text-sm font-medium">触发器</span>
            <BaseButton
              v-if="localValue.triggers.length < 3"
              variant="primary"
              size="sm"
              @click="addTrigger"
            >
              添加触发器
            </BaseButton>
          </div>

          <div class="space-y-2">
            <TriggerEditor
              v-for="(trigger, index) in localValue.triggers"
              :key="index"
              :model-value="trigger"
              :index="index"
              @update:model-value="updateTrigger(index, $event)"
              @remove="removeTrigger(index)"
            />
          </div>
        </div>
      </div>
    </BaseCard>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import type { Transition, Trigger } from '../../types/stateMachine';
import BaseCard from '../base/BaseCard.vue';
import BaseInput from '../base/BaseInput.vue';
import BaseButton from '../base/BaseButton.vue';
import BaseSelect from '../base/BaseSelect.vue';
import TriggerEditor from './TriggerEditor.vue';

// 使用 Vue 3.5+ defineModel 简化双向绑定
const model = defineModel<Transition>('modelValue', {
  required: true,
});

interface Props {
  availableStates?: string[];
}

withDefaults(defineProps<Props>(), {
  availableStates: () => [],
});

const emit = defineEmits<{
  remove: [];
}>();

// 本地值引用
const localValue = computed({
  get: () => model.value,
  set: (value) => {
    model.value = value;
  },
});

// 触发逻辑选项
const triggerLogicOptions = [
  { label: 'AND', value: 0 },
  { label: 'OR', value: 1 },
  { label: 'XOR', value: 2 },
];

/**
 * 更新当前转换对象的单个字段。
 */
const updateField = <K extends keyof Transition>(key: K, value: Transition[K]) => {
  model.value = { ...model.value, [key]: value };
};

/**
 * 向当前转换中追加一个新的触发器，并同步更新触发器计数。
 */
const addTrigger = () => {
  const newTrigger: Trigger = {
    triggerType: 1,
    triggerSource: '',
    operator: 0,
    triggerValue: 0,
  };
  model.value = {
    ...model.value,
    triggers: [...model.value.triggers, newTrigger],
    triggerCount: model.value.triggers.length + 1,
  };
};

/**
 * 更新指定索引位置的触发器。
 */
const updateTrigger = (index: number, value: Trigger) => {
  const triggers = [...model.value.triggers];
  triggers[index] = value;
  model.value = { ...model.value, triggers };
};

/**
 * 删除指定索引位置的触发器，并同步更新计数。
 */
const removeTrigger = (index: number) => {
  const triggers = model.value.triggers.filter((_, i) => i !== index);
  model.value = {
    ...model.value,
    triggers,
    triggerCount: triggers.length,
  };
};
</script>

<style scoped>
.transition-editor {
  width: 100%;
}
</style>
