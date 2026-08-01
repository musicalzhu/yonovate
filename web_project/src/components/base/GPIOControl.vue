<!--
   Vue 3.5+ GPIO控制组件

   【功能说明】
   提供单个GPIO端口的控制界面，支持状态显示、模式展示和开关控制。
   显示当前值、模式、状态和最后更新时间。

   【核心概念】
   1. 模式系统：input（输入）、output（输出）、input_pullup（上拉输入）、input_pulldown（下拉输入）
   2. 状态显示：HIGH（高电平）或 LOW（低电平）
   3. 时间显示：显示最后更新时间（刚刚、X分钟前等）

   【适用场景】
   - GPIO控制页面（GpioControl.vue）
   - 设备控制页面
   - 实时监控面板

   Props:
   - gpio: GPIO状态对象（必需）
   - readonly: 是否只读（默认false）

   Emits:
   - toggle: 切换事件，携带引脚号
   - setValue: 设置值事件，携带引脚号和值

   使用示例：
   <GPIOControl
     :gpio="gpioStatus"
     :readonly="isReadOnly"
     @toggle="handleToggle"
   />
 -->
<template>
  <div class="gpio-control">
    <div class="gpio-control__header">
      <span class="gpio-control__label">{{ label || `GPIO ${gpio.pin}` }}</span>
      <span class="gpio-control__mode" :class="modeClass">{{ modeText }}</span>
    </div>

    <div class="gpio-control__body">
      <button
        class="gpio-control__toggle"
        :class="{ 'gpio-control__toggle--active': isActive }"
        :disabled="readonly"
        :aria-label="`切换 GPIO ${gpio.pin}`"
        @click="handleToggle"
      >
        <span class="gpio-control__value">{{ gpio.value }}</span>
        <span class="gpio-control__state">{{ isActive ? 'HIGH' : 'LOW' }}</span>
      </button>

      <div class="gpio-control__info">
        <span class="gpio-control__time">{{ lastUpdated }}</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue';
import type { GPIOStatus, GPIOMode } from '@/types';

interface Props {
  gpio: GPIOStatus;
  readonly?: boolean;
}

interface Emits {
  toggle: [pin: number];
  setValue: [pin: number, value: number];
}

const props = withDefaults(defineProps<Props>(), {
  readonly: false,
});

const emit = defineEmits<Emits>();

const label = computed(() => props.gpio.label);

const isActive = computed(() => props.gpio.value > 0);

const modeText = computed(() => {
  const modeMap: Record<GPIOMode, string> = {
    input: '输入',
    output: '输出',
    input_pullup: '上拉输入',
    input_pulldown: '下拉输入',
  };
  return modeMap[props.gpio.mode] || props.gpio.mode;
});

const modeClass = computed(() => {
  return {
    'gpio-control__mode--input': props.gpio.mode === 'input',
    'gpio-control__mode--output': props.gpio.mode === 'output',
    'gpio-control__mode--pullup': props.gpio.mode === 'input_pullup',
    'gpio-control__mode--pulldown': props.gpio.mode === 'input_pulldown',
  };
});

const lastUpdated = computed(() => {
  const date = new Date(props.gpio.lastUpdated);
  const now = new Date();
  const diff = now.getTime() - date.getTime();

  if (diff < 60000) {
    return '刚刚';
  } else if (diff < 3600000) {
    return `${Math.floor(diff / 60000)} 分钟前`;
  } else {
    return date.toLocaleTimeString();
  }
});

const handleToggle = () => {
  if (props.readonly) return;
  emit('toggle', props.gpio.pin);
};
</script>

<style scoped>
.gpio-control {
  border: 1px solid #e5e7eb;
  border-radius: 0.5rem;
  padding: 0.75rem;
  background-color: white;
  transition: all 0.2s;
}

.gpio-control__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 0.5rem;
}

.gpio-control__label {
  font-size: 0.875rem;
  font-weight: 500;
  color: #111827;
}

.gpio-control__mode {
  font-size: 0.75rem;
  padding: 0.25rem 0.5rem;
  border-radius: 0.25rem;
}

.gpio-control__mode--input {
  background-color: #dbeafe;
  color: #1e40af;
}

.gpio-control__mode--output {
  background-color: #dcfce7;
  color: #166534;
}

.gpio-control__mode--pullup {
  background-color: #f3e8ff;
  color: #6b21a8;
}

.gpio-control__mode--pulldown {
  background-color: #ffedd5;
  color: #9a3412;
}

.gpio-control__body {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.gpio-control__toggle {
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0.5rem 1rem;
  border-radius: 0.5rem;
  border: 2px solid #d1d5db;
  transition: all 0.2s;
}

.gpio-control__toggle:hover {
  border-color: #9ca3af;
}

.gpio-control__toggle--active {
  border-color: #22c55e;
  background-color: #f0fdf4;
}

.gpio-control__toggle--active:hover {
  border-color: #16a34a;
}

.gpio-control__toggle:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.gpio-control__value {
  font-size: 1.5rem;
  font-weight: 700;
  color: #111827;
}

.gpio-control__state {
  font-size: 0.875rem;
  font-weight: 500;
  color: #4b5563;
}

.gpio-control__info {
  text-align: center;
}

.gpio-control__time {
  font-size: 0.75rem;
  color: #6b7280;
}
</style>
