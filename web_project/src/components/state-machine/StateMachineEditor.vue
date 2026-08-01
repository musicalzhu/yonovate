<template>
  <div class="state-machine-editor">
    <BaseCard class="state-machine-editor__config">
      <template #header>
        <h3 class="text-lg font-semibold">状态机配置</h3>
      </template>

      <div class="space-y-4">
        <BaseInput
          v-model="safeConfig.initialState"
          label="初始状态"
          placeholder="请输入初始状态"
          required
          :error="errors.initialState"
        />

        <div class="space-y-2">
          <div class="flex items-center justify-between">
            <h4 class="text-md font-medium">状态转换</h4>
            <BaseButton variant="primary" size="sm" @click="addTransition"> 添加转换 </BaseButton>
          </div>

          <div class="space-y-2">
            <TransitionEditor
              v-for="(transition, index) in safeConfig.transitions"
              :key="index"
              :model-value="transition"
              :available-states="availableStates"
              @update:model-value="updateTransition(index, $event)"
              @remove="removeTransition(index)"
            />
          </div>
        </div>

        <div class="grid grid-cols-2 gap-4">
          <BaseInput v-model="safeConfig.groupModes[0]" label="组模式 0" type="number" />
          <BaseInput v-model="safeConfig.groupModes[1]" label="组模式 1" type="number" />
          <BaseInput v-model="safeConfig.groupModes[2]" label="组模式 2" type="number" />
          <BaseInput v-model="safeConfig.groupModes[3]" label="组模式 3" type="number" />
        </div>
      </div>
    </BaseCard>

    <BaseCard class="state-machine-editor__states">
      <template #header>
        <h3 class="text-lg font-semibold">状态图</h3>
      </template>

      <div class="flex flex-wrap gap-2">
        <StateIndicator
          v-for="state in availableStates"
          :key="state"
          :state="state"
          :is-current="state === safeConfig.initialState"
          :is-active="activeStates.includes(state)"
        />
      </div>
    </BaseCard>

    <BaseCard class="state-machine-editor__preview">
      <template #header>
        <div class="flex items-center justify-between">
          <h3 class="text-lg font-semibold">JSON预览</h3>
          <BaseButton variant="ghost" size="sm" @click="copyToClipboard"> 复制 </BaseButton>
        </div>
      </template>

      <pre class="json-preview">{{ jsonPreview }}</pre>
    </BaseCard>

    <div class="state-machine-editor__actions">
      <BaseButton variant="secondary" @click="loadTemplate"> 加载模板 </BaseButton>
      <BaseButton variant="secondary" @click="saveAsTemplate"> 保存为模板 </BaseButton>
      <BaseButton variant="primary" :loading="loading === 'loading'" @click="saveConfig">
        保存配置
      </BaseButton>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import type { StateMachineConfig, Transition } from '@/types/domain/stateMachine';
import { useStateMachineLogic, getAvailableStates, evaluateTriggers } from '@/services/businessLogic';
import { useNotificationsStore } from '@/stores/notifications';
import { createDefaultStateMachineConfig } from '@/types/domain/stateMachine';
import BaseCard from '../base/BaseCard.vue';
import BaseInput from '../base/BaseInput.vue';
import BaseButton from '../base/BaseButton.vue';
import TransitionEditor from './TransitionEditor.vue';
import StateIndicator from './StateIndicator.vue';

const model = defineModel<StateMachineConfig>('modelValue', {
  required: false,
});

interface Props {
  readonly?: boolean;
}

interface Emits {
  save: [config: StateMachineConfig];
}

const props = withDefaults(defineProps<Props>(), {
  readonly: false,
});

const emit = defineEmits<Emits>();

const safeConfig = computed(() => model.value ?? createDefaultStateMachineConfig());

const stateMachineLogic = computed(() => useStateMachineLogic(safeConfig.value));

const availableStates = computed(() => getAvailableStates(safeConfig.value));

const activeStates = computed(() => evaluateTriggers(safeConfig.value.transitions, []));

const jsonPreview = computed(() => JSON.stringify(safeConfig.value, null, 2));

const notificationsStore = useNotificationsStore();

const errors = computed(() => ({
  initialState: '',
}));

const loading = computed(() => 'idle');

const addTransition = () => {
  const newConfig = { ...safeConfig.value };
  newConfig.transitions = [
    ...newConfig.transitions,
    {
      currentState: '',
      nextState: '',
      triggers: [],
      triggerCount: 0,
      triggerLogic: 0,
    },
  ];
  model.value = newConfig;
};

const removeTransition = (index: number) => {
  const newConfig = { ...safeConfig.value };
  newConfig.transitions = newConfig.transitions.filter((_, i) => i !== index);
  model.value = newConfig;
};

const updateTransition = (index: number, transition: Transition) => {
  const newConfig = { ...safeConfig.value };
  const newTransitions = [...newConfig.transitions];
  newTransitions[index] = transition;
  newConfig.transitions = newTransitions;
  model.value = newConfig;
};

const handleSave = (savedConfig: StateMachineConfig) => {
  emit('save', savedConfig);
  notificationsStore.addNotification({
    type: 'success',
    title: '配置已保存',
    message: '状态机配置已成功保存到设备',
  });
};

const pushNotification = (type: 'success' | 'error', title: string, message: string) => {
  notificationsStore.addNotification({ type, title, message });
};

const saveConfig = async () => {
  try {
    handleSave(safeConfig.value);
  } catch {
    pushNotification('error', '保存失败', '状态机配置保存失败，请稍后重试');
  }
};

const loadTemplate = () => {
  console.log('Loading template...');
};

const saveAsTemplate = () => {
  console.log('Saving as template...');
};

const copyToClipboard = async () => {
  try {
    await navigator.clipboard.writeText(jsonPreview.value);
    pushNotification('success', '复制成功', 'JSON配置已复制到剪贴板');
  } catch {
    pushNotification('error', '复制失败', '无法复制到剪贴板');
  }
};
</script>

<style scoped>
.state-machine-editor {
  display: flex;
  flex-direction: column;
  gap: 1.5rem;
  padding: 1.5rem;
}

.state-machine-editor__config {
  max-width: 42rem;
}

.state-machine-editor__states {
  max-width: 42rem;
}

.state-machine-editor__preview {
  max-width: 42rem;
}

.json-preview {
  background-color: #f3f4f6;
  color: #1f2937;
  padding: 1rem;
  border-radius: 0.375rem;
  font-size: 0.875rem;
  overflow: auto;
  max-height: 24rem;
  font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, monospace;
}

.state-machine-editor__actions {
  display: flex;
  gap: 0.5rem;
  justify-content: flex-end;
}
</style>