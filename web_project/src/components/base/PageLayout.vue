<!--
   Vue 3.5+ 页面布局组件

   【功能说明】
   提供统一的页面布局结构，包含页面标题、副标题、操作区域和主体内容区。
   确保所有页面具有一致的视觉风格和布局结构。

   【核心概念】
   1. 页面头部：包含标题和副标题
   2. 操作区域：用于放置页面级别的操作按钮
   3. 主体内容：页面主要内容区域

   【适用场景】
   - 所有页面的统一布局容器
   - 提供页面级别的标题和操作按钮

   Props:
   - title: 页面标题（必需）
   - subtitle: 页面副标题（可选）
   - actions: 操作组件（可选，用于放置操作按钮）

   Slots:
   - actions: 操作区域插槽，用于放置页面操作按钮
   - default: 主体内容插槽，用于放置页面主要内容

   使用示例：
   <PageLayout title="GPIO控制" subtitle="配置和管理GPIO引脚状态">
     <template #actions>
       <BaseButton variant="primary">保存配置</BaseButton>
     </template>
     <BaseCard title="GPIO配置">
       内容区域
     </BaseCard>
   </PageLayout>
 -->
<script setup lang="ts">
import type { DefineComponent, Component } from 'vue';

interface Props {
  title: string;
  subtitle?: string;
  actions?: Component;
}

defineProps<Props>();
</script>

<template>
  <div class="page-layout">
    <div class="page-layout__container">
      <header class="page-layout__header">
        <h1 class="page-layout__title">{{ title }}</h1>
        <p v-if="subtitle" class="page-layout__subtitle">{{ subtitle }}</p>
      </header>

      <slot name="actions" />

      <main class="page-layout__main">
        <slot />
      </main>
    </div>
  </div>
</template>

<style scoped>
.page-layout__container {
  max-width: 1200px;
  margin: 0 auto;
  padding: 1rem 1rem 2rem;
}

.page-layout__header {
  margin-bottom: 2rem;
}

.page-layout__title {
  font-size: 1.875rem;
  font-weight: 700;
  color: #111827;
  margin-bottom: 0.5rem;
}

.page-layout__subtitle {
  color: #6b7280;
}
</style>
