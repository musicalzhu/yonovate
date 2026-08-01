<!--
   Vue 3.5+ 基础模态框组件

   【功能说明】
   提供可定制的模态框组件，支持多种尺寸、关闭方式和动画效果。
   使用 Teleport 将模态框渲染到 body 元素，确保正确的层级关系。

   【核心概念】
   1. 尺寸系统：sm、md、lg、xl四种尺寸
   2. 关闭方式：点击遮罩层关闭、按Escape键关闭
   3. 插槽系统：header、default（内容）、footer三个插槽
   4. 动画效果：通过 CSS Transition 实现淡入淡出效果

   【适用场景】
   - 设置对话框（设备设置、主题设置等）
   - 确认对话框（删除确认、操作确认）
   - 详情展示（模板管理、帮助信息）

   Props:
   - modelValue: 显示/隐藏状态（必需）
   - title: 对话框标题（可选）
   - size: 对话框尺寸（sm, md, lg, xl）
   - closeOnOverlay: 点击遮罩层是否关闭（默认true）
   - closeOnEscape: 按Escape键是否关闭（默认true）

   Events:
   - update:modelValue: 状态变化事件
   - close: 关闭事件

   Slots:
   - header: 头部区域（可选）
   - default: 内容区域（必需）
   - footer: 底部区域（可选）

   使用示例：
   <BaseModal v-model="showSettings" title="设备设置" size="lg">
     <template #footer>
       <BaseButton @click="showSettings = false">取消</BaseButton>
       <BaseButton variant="primary" @click="saveSettings">保存</BaseButton>
     </template>
     内容区域
   </BaseModal>
 -->
<template>
  <Teleport to="body">
    <Transition name="modal">
      <div v-if="modelValue" class="base-modal__overlay" @click="handleOverlayClick">
        <div :class="containerClasses" @click.stop>
          <!-- 头部 -->
          <div v-if="title || $slots.header" class="base-modal__header">
            <h3 v-if="title" class="base-modal__title">{{ title }}</h3>
            <slot name="header" />
            <button class="base-modal__close" aria-label="关闭" @click="handleClose">
              <svg width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
                <path
                  fill-rule="evenodd"
                  d="M4.293 4.293a1 1 0 011.414 0L10 8.586l4.293-4.293a1 1 0 111.414 1.414L11.414 10l4.293 4.293a1 1 0 01-1.414 1.414L10 11.414l-4.293 4.293a1 1 0 01-1.414-1.414L8.586 10 4.293 5.707a1 1 0 010-1.414z"
                  clip-rule="evenodd"
                />
              </svg>
            </button>
          </div>

          <!-- 内容 -->
          <div class="base-modal__body">
            <slot></slot>
          </div>

          <!-- 底部 -->
          <div v-if="$slots.footer" class="base-modal__footer">
            <slot name="footer"></slot>
          </div>
        </div>
      </div>
    </Transition>
  </Teleport>
</template>

<script setup lang="ts">
import { computed, onMounted, onUnmounted } from 'vue';

interface Props {
  modelValue: boolean;
  title?: string;
  size?: 'sm' | 'md' | 'lg' | 'xl';
  closeOnOverlay?: boolean;
  closeOnEscape?: boolean;
}

const props = withDefaults(defineProps<Props>(), {
  title: '',
  size: 'md',
  closeOnOverlay: true,
  closeOnEscape: true,
});

const emit = defineEmits<{
  'update:modelValue': [value: boolean];
  close: [];
}>();

const containerClasses = computed(() => [
  'base-modal__container',
  `base-modal__container--${props.size}`,
]);

/**
 * 关闭当前模态框，并向父组件发出关闭事件。
 */
const handleClose = () => {
  emit('update:modelValue', false);
  emit('close');
};

/**
 * 处理点击遮罩层关闭模态框的行为。
 */
const handleOverlayClick = () => {
  if (props.closeOnOverlay) {
    handleClose();
  }
};

/**
 * 处理键盘 Escape 按键，用于关闭当前打开的模态框。
 */
const handleEscape = (e: KeyboardEvent) => {
  if (props.closeOnEscape && e.key === 'Escape' && props.modelValue) {
    handleClose();
  }
};

onMounted(() => {
  window.addEventListener('keydown', handleEscape);
});

onUnmounted(() => {
  window.removeEventListener('keydown', handleEscape);
});
</script>

<style scoped>
:root {
  --base-modal-bg: #ffffff;
  --base-modal-border: #e5e7eb;
  --base-modal-text: #111827;
  --base-modal-label: #374151;
  --base-modal-close: #9ca3af;
  --base-modal-overlay: rgba(0, 0, 0, 0.5);
}

.base-modal__overlay {
  position: fixed;
  inset: 0;
  z-index: 50;
  display: flex;
  align-items: center;
  justify-content: center;
  background-color: var(--base-modal-overlay);
  backdrop-filter: blur(4px);
  padding: 1rem;
}

.base-modal__container {
  background-color: var(--base-modal-bg);
  border-radius: 0.5rem;
  box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.1);
  max-height: 90vh;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  width: 100%;
}

.base-modal__container--sm {
  max-width: 24rem;
}
.base-modal__container--md {
  max-width: 28rem;
}
.base-modal__container--lg {
  max-width: 32rem;
}
.base-modal__container--xl {
  max-width: 36rem;
}

.base-modal__header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 1rem 1.5rem;
  border-bottom: 1px solid var(--base-modal-border);
}

.base-modal__title {
  font-size: 1.125rem;
  font-weight: 600;
  color: var(--base-modal-text);
  margin: 0;
}

.base-modal__close {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 2rem;
  height: 2rem;
  color: var(--base-modal-close);
  border: none;
  background: transparent;
  border-radius: 9999px;
  cursor: pointer;
  transition:
    color 0.2s,
    background-color 0.2s;
}

.base-modal__close:hover {
  color: var(--base-modal-text);
  background-color: rgba(0, 0, 0, 0.05);
}

.base-modal__body {
  padding: 1rem 1.5rem;
  overflow-y: auto;
  flex: 1;
}

.base-modal__footer {
  padding: 1rem 1.5rem;
  border-top: 1px solid var(--base-modal-border);
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 0.5rem;
}

/* 过渡动画 */
.modal-enter-active,
.modal-leave-active {
  transition: opacity 0.3s;
}

.modal-enter-from,
.modal-leave-to {
  opacity: 0;
}

.modal-enter-from .base-modal__container,
.modal-leave-to .base-modal__container {
  transform: scale(0.95);
}
</style>
