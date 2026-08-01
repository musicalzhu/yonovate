<!--
  Vue 3.5+ GPIO控制页面组件

  【功能说明】
  GPIO控制页面，用于配置和管理GPIO引脚的组模式和状态。
  提供加载和保存配置功能，支持12个GPIO引脚（P0-P11）的配置。

  【核心概念】
  1. 组模式：定义GPIO引脚的工作模式（输出/输入）
  2. GPIO状态：显示当前引脚的输出值（HIGH/LOW）
  3. 配置存储：使用本地存储保存配置，支持加载和保存操作

  【数据流程】
  加载页面 → 从本地存储加载配置 → 显示当前配置 →
  用户修改配置 → 保存配置到本地存储

  【适用场景】
  - 配置GPIO引脚的输入输出模式
  - 查看当前GPIO引脚状态
  - 保存GPIO配置以便下次使用

  【注意事项】
  - 此页面使用 useConfigStorage composable 管理本地配置
  - 不依赖后端API，所有配置存储在浏览器本地存储中
-->
<template>
  <PageLayout title="GPIO 控制" subtitle="配置和管理GPIO引脚状态">
    <template #actions>
      <div class="mb-6 flex gap-2">
        <BaseButton :loading="loading" variant="secondary" @click="loadConfig">
          加载配置
        </BaseButton>
        <BaseButton :loading="loading" variant="primary" @click="saveConfig"> 保存配置 </BaseButton>
      </div>
    </template>

    <BaseCard title="GPIO 组配置">
      <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
        <div>
          <h3 class="text-lg font-semibold mb-4">组模式</h3>
          <div class="space-y-2">
            <div
              v-for="(_, index) in gpioConfig.groupModes"
              :key="index"
              class="group-mode flex items-center gap-2"
            >
              <span class="w-10 font-bold text-gray-700">P{{ index }}:</span>
              <BaseSelect
                v-model="gpioConfig.groupModes[index]"
                :options="[
                  { label: '输出', value: 0 },
                  { label: '输入', value: 1 },
                ]"
                size="small"
              />
            </div>
          </div>
        </div>

        <div>
          <h3 class="text-lg font-semibold mb-4">GPIO 状态</h3>
          <div class="space-y-2">
            <div
              v-for="(state, index) in gpioStates"
              :key="index"
              class="gpio-item flex items-center gap-2"
            >
              <span class="w-10 font-bold text-gray-700">P{{ index }}:</span>
              <span
                class="px-3 py-1 rounded-full text-sm font-medium"
                :class="{
                  'bg-green-100 text-green-800': state,
                  'bg-red-100 text-red-800': !state,
                }"
              >
                {{ state ? 'HIGH' : 'LOW' }}
              </span>
            </div>
          </div>
        </div>
      </div>
    </BaseCard>
  </PageLayout>
</template>

<script setup lang="ts">
/**
 * Script Setup 部分
 *
 * 【依赖导入】
 * - Vue 3 组合式API：onMounted
 * - Composables：useConfigStorage (本地配置存储管理)
 * - Components：
 *   - PageLayout: 页面布局组件
 *   - BaseCard: 基础卡片组件
 *   - BaseButton: 基础按钮组件
 *   - BaseSelect: 基础选择器组件
 */
import { onMounted } from 'vue';
import { useConfigStorage } from '../composables/useConfigStorage';
import PageLayout from '../components/base/PageLayout.vue';
import BaseCard from '../components/base/BaseCard.vue';
import BaseButton from '../components/base/BaseButton.vue';
import BaseSelect from '../components/base/BaseSelect.vue';

// ==================== 默认配置 ====================
/**
 * GPIO默认配置对象
 *
 * 属性说明：
 * - groupModes: 组模式数组（12个元素），0=输出, 1=输入
 * - inputModuleTypes: 输入模块类型数组（12个元素）
 * - edgeTypes: 边沿类型数组（12个元素）
 * - outputModuleTypes: 输出模块类型数组（12个元素）
 *
 * 所有数组默认值为0，对应12个GPIO引脚（P0-P11）
 */
const defaultGpioConfig = {
  groupModes: new Array(12).fill(0),
  inputModuleTypes: new Array(12).fill(0),
  edgeTypes: new Array(12).fill(0),
  outputModuleTypes: new Array(12).fill(0),
};

// ==================== Composable 使用 ====================
/**
 * useConfigStorage - 配置存储管理 composable
 *
 * 返回值：
 * - gpioConfig: 当前GPIO配置对象（响应式）
 * - loading: 加载状态标志
 * - loadConfig: 从本地存储加载配置的方法
 * - saveConfig: 保存配置到本地存储的方法
 *
 * 参数：
 * - 'gpio-config': 本地存储的键名
 * - defaultGpioConfig: 默认配置对象（当本地存储无数据时使用）
 */
const {
  config: gpioConfig,
  loading,
  loadConfig,
  saveConfig,
} = useConfigStorage('gpio-config', defaultGpioConfig);

// ==================== GPIO状态 ====================
/**
 * GPIO状态数组
 *
 * 用于显示当前引脚的输出状态（HIGH/LOW）
 * 固定数组：12个元素，默认值为false（LOW）
 *
 * 注意：此数组为静态数据，实际应用中应从设备获取实时状态
 */
const gpioStates = new Array(12).fill(false);

// ==================== 生命周期钩子 ====================
/**
 * 组件挂载时执行
 *
 * 功能：自动加载已保存的GPIO配置
 *
 * 流程：
 * 1. 调用 loadConfig 方法
 * 2. 从本地存储读取配置
 * 3. 如果存在配置，更新 gpioConfig
 * 4. 如果不存在，使用 defaultGpioConfig
 */
onMounted(() => {
  loadConfig();
});
</script>

<style scoped>
.group-mode {
  margin-bottom: 0.5rem;
}

.gpio-item {
  margin-bottom: 0.5rem;
}
</style>
