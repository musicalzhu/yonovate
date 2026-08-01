<!--
   Vue 3.5+ 通知提示组件

   【功能说明】
   提供可定制的通知提示组件，支持多种类型、自动关闭和进度条显示。
   用于在应用中显示操作结果、警告、错误等信息。

   【核心概念】
   1. 类型系统：success（成功）、error（错误）、warning（警告）、info（信息）
   2. 自动关闭：默认5秒后自动关闭，可配置
   3. 进度条：可显示关闭倒计时进度
   4. 关闭按钮：支持手动关闭

   【适用场景】
   - 操作成功提示（保存成功、删除成功等）
   - 操作失败提示（网络错误、验证失败等）
   - 警告信息（配置不完整等）
   - 一般信息提示

   Props:
   - id: 通知唯一标识（必需）
   - type: 通知类型（success, error, warning, info）
   - title: 通知标题（可选）
   - message: 通知内容（可选）
   - duration: 自动关闭时间（默认5000毫秒）
   - closable: 是否显示关闭按钮（默认true）
   - showProgress: 是否显示进度条（默认false）
   - autoClose: 是否自动关闭（默认true）

   Events:
   - close: 关闭事件，携带通知ID

   使用示例：
   <NotificationToast
     :id="notification.id"
     :type="notification.type"
     :title="notification.title"
     :message="notification.message"
     @close="removeNotification"
   />
 -->
<template>
  <Transition name="notification" appear @before-leave="onBeforeLeave" @after-leave="onAfterLeave">
    <div v-if="visible" :class="toastClasses">
      <div class="notification-toast__icon">
        <component :is="iconComponent" />
      </div>

      <div class="notification-toast__content">
        <h4 v-if="title" class="notification-toast__title">{{ title }}</h4>
        <p v-if="message" class="notification-toast__message">{{ message }}</p>
      </div>

      <button v-if="closable" class="notification-toast__close" @click="close">
        <CloseIcon />
      </button>

      <div
        v-if="showProgress"
        class="notification-toast__progress"
        :style="{ width: `${progress}%` }"
      />
    </div>
  </Transition>
</template>

<script setup lang="ts">
import { computed, ref, onMounted, onUnmounted } from 'vue';

interface NotificationToastProps {
  id: string;
  type: 'success' | 'error' | 'warning' | 'info';
  title?: string;
  message?: string;
  duration?: number;
  closable?: boolean;
  showProgress?: boolean;
  autoClose?: boolean;
}

interface Emits {
  close: [id: string];
}

const props = withDefaults(defineProps<NotificationToastProps>(), {
  duration: 5000,
  closable: true,
  showProgress: false,
  autoClose: true,
});

const emit = defineEmits<Emits>();

const visible = ref(false);
const progress = ref(0);
let progressTimer: NodeJS.Timeout | null = null;
let closeTimer: NodeJS.Timeout | null = null;

// 图标组件
const iconComponent = computed(() => {
  switch (props.type) {
    case 'success':
      return 'CheckCircleIcon';
    case 'error':
      return 'XCircleIcon';
    case 'warning':
      return 'ExclamationTriangleIcon';
    case 'info':
      return 'InformationCircleIcon';
    default:
      return 'InformationCircleIcon';
  }
});

const toastClasses = computed(() => {
  return [
    'notification-toast',
    `notification-toast--${props.type}`,
    {
      'notification-toast--with-progress': props.showProgress,
    },
  ];
});

// 方法
const close = () => {
  visible.value = false;
  emit('close', props.id);
  clearTimers();
};

const clearTimers = () => {
  if (progressTimer) {
    clearInterval(progressTimer);
    progressTimer = null;
  }
  if (closeTimer) {
    clearTimeout(closeTimer);
    closeTimer = null;
  }
};

const onBeforeLeave = () => {
  clearTimers();
};

const onAfterLeave = () => {
  // 动画结束后的清理
};

// 生命周期
onMounted(() => {
  // 延迟显示以触发进入动画
  setTimeout(() => {
    visible.value = true;
  }, 50);

  // 自动关闭
  if (props.autoClose && props.duration > 0) {
    closeTimer = setTimeout(() => {
      close();
    }, props.duration);
  }

  // 进度条动画
  if (props.showProgress && props.duration > 0) {
    const interval = 50; // 50ms更新一次
    const totalSteps = props.duration / interval;
    let currentStep = 0;

    progressTimer = setInterval(() => {
      currentStep++;
      progress.value = (currentStep / totalSteps) * 100;

      if (currentStep >= totalSteps) {
        clearTimers();
        close();
      }
    }, interval);
  }
});

onUnmounted(() => {
  clearTimers();
});
</script>

<style scoped>
.notification-toast {
  display: flex;
  align-items: flex-start;
  padding: 1rem;
  border-radius: 0.5rem;
  box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.1);
  border: 1px solid;
  max-width: 28rem;
  min-width: 0;
  transform: translateX(0);
  transition: all 0.3s ease-in-out;
  background-color: white;
}

.notification-toast--success {
  border-color: #bbf7d0;
  background-color: #f0fdf4;
}

.notification-toast--error {
  border-color: #fecaca;
  background-color: #fef2f2;
}

.notification-toast--warning {
  border-color: #fef08a;
  background-color: #fefce8;
}

.notification-toast--info {
  border-color: #bfdbfe;
  background-color: #eff6ff;
}

.notification__icon {
  flex-shrink: 0;
  margin-right: 0.75rem;
  width: 1.25rem;
  height: 1.25rem;
}

.notification-toast--success .notification__icon {
  color: #22c55e;
}

.notification-toast--error .notification__icon {
  color: #ef4444;
}

.notification-toast--warning .notification__icon {
  color: #eab308;
}

.notification-toast--info .notification__icon {
  color: #3b82f6;
}

.notification__content {
  flex: 1;
  min-width: 0;
}

.notification__title {
  font-weight: 600;
  color: #111827;
  margin-bottom: 0.25rem;
  font-size: 0.875rem;
  line-height: 1.25;
}

.notification__message {
  color: #4b5563;
  font-size: 0.875rem;
  line-height: 1.625;
  word-break: break-words;
}

.notification__close {
  margin-left: 0.75rem;
  color: #9ca3af;
  transition: color 0.2s;
  flex-shrink: 0;
}

.notification__close:hover {
  color: #4b5563;
}

.notification__close:focus {
  outline: none;
  box-shadow: 0 0 0 2px rgba(59, 130, 246, 0.5);
}

.notification__progress {
  position: absolute;
  bottom: 0;
  left: 0;
  height: 0.25rem;
  transition: width 0.3s ease-linear;
}

.notification-toast--success .notification__progress {
  background-color: #22c55e;
}

.notification-toast--error .notification__progress {
  background-color: #ef4444;
}

.notification-toast--warning .notification__progress {
  background-color: #eab308;
}

.notification-toast--info .notification__progress {
  background-color: #3b82f6;
}

/* 进入和离开动画 */
.notification-enter-active,
.notification-leave-active {
  transition: all 0.3s ease-in-out;
}

.notification-enter-from {
  transform: translateX(100%);
  opacity: 0;
}

.notification-leave-to {
  transform: translateX(100%);
  opacity: 0;
}

.notification-move {
  transition: transform 0.3s ease-in-out;
}
</style>
