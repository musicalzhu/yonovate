<!--
  Vue 3.5+ 逻辑电路配置页面组件

  【功能说明】
  逻辑电路配置页面，用于配置和管理逻辑门电路。
  提供逻辑门类型选择、输入源配置、输出目标配置，以及逻辑图预览功能。

  【核心概念】
  1. 逻辑门类型：支持AND、OR、NOT、NAND、NOR、XOR六种基本逻辑门
  2. 输入源：定义逻辑门的输入引脚（P0-P11）
  3. 输出目标：定义逻辑门的输出引脚（P0-P11）
  4. 逻辑图预览：可视化显示当前的逻辑门配置

  【数据流程】
  加载页面 → 从本地存储加载配置 → 显示当前配置 →
  用户修改配置 → 保存配置到本地存储

  【适用场景】
  - 配置简单的逻辑门电路
  - 实现GPIO引脚之间的逻辑运算
  - 创建自动化的逻辑控制规则

  【注意事项】
  - 此页面使用 useConfigStorage composable 管理本地配置
  - 不依赖后端API，所有配置存储在浏览器本地存储中
  - 默认配置为AND门，输入P0和P1，输出P2
-->
<template>
  <PageLayout title="逻辑电路配置" subtitle="配置和管理逻辑门电路">
    <template #actions>
      <div class="mb-6 flex gap-2">
        <BaseButton :loading="loading" variant="secondary" @click="loadConfig">
          加载配置
        </BaseButton>
        <BaseButton :loading="loading" variant="primary" @click="saveConfig"> 保存配置 </BaseButton>
      </div>
    </template>

    <BaseCard title="逻辑门配置">
      <div class="space-y-6">
        <div>
          <label class="block text-sm font-medium text-gray-700 mb-2">逻辑门类型</label>
          <BaseSelect
            v-model="logicConfig.gateType"
            :options="[
              { label: 'AND', value: 'AND' },
              { label: 'OR', value: 'OR' },
              { label: 'NOT', value: 'NOT' },
              { label: 'NAND', value: 'NAND' },
              { label: 'NOR', value: 'NOR' },
              { label: 'XOR', value: 'XOR' },
            ]"
          />
        </div>

        <div>
          <label class="block text-sm font-medium text-gray-700 mb-2">输入源</label>
          <div class="space-y-2">
            <div
              v-for="(_, index) in logicConfig.inputs"
              :key="index"
              class="input-item flex items-center gap-2"
            >
              <span class="w-16 text-gray-700">输入 {{ index + 1 }}:</span>
              <BaseSelect
                v-model="logicConfig.inputs[index]"
                :options="
                  Array.from({ length: 12 }, (_, i) => ({ label: `P${i}`, value: `P${i}` }))
                "
                size="small"
              />
            </div>
          </div>
        </div>

        <div>
          <label class="block text-sm font-medium text-gray-700 mb-2">输出目标</label>
          <BaseSelect
            v-model="logicConfig.output"
            :options="Array.from({ length: 12 }, (_, i) => ({ label: `P${i}`, value: `P${i}` }))"
          />
        </div>
      </div>
    </BaseCard>

    <BaseCard title="逻辑图预览" class="mt-6">
      <div class="diagram-container bg-gray-50 p-6 rounded-lg text-center">
        <div class="gate-diagram inline-block relative">
          <div
            class="gate-block bg-blue-500 text-white px-10 py-5 rounded-lg font-bold text-lg mb-5"
          >
            {{ logicConfig.gateType }}
          </div>
          <div class="inputs mb-5">
            <div
              v-for="(input, index) in logicConfig.inputs"
              :key="index"
              class="input-line bg-orange-400 text-white px-4 py-2 rounded mb-1 font-bold"
            >
              {{ input }}
            </div>
          </div>
          <div class="output bg-green-500 text-white px-4 py-2 rounded font-bold">
            {{ logicConfig.output }}
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
 * 逻辑电路默认配置对象
 *
 * 属性说明：
 * - gateType: 逻辑门类型，默认为 'AND'
 * - inputs: 输入源数组，默认为 ['P0', 'P1']（两个输入）
 * - output: 输出目标，默认为 'P2'
 *
 * 支持的逻辑门类型：
 * - AND: 与门（所有输入为高时输出高）
 * - OR: 或门（任一输入为高时输出高）
 * - NOT: 非门（反转输入）
 * - NAND: 与非门（AND的反转）
 * - NOR: 或非门（OR的反转）
 * - XOR: 异或门（输入不同时输出高）
 */
const defaultLogicConfig = {
  gateType: 'AND',
  inputs: ['P0', 'P1'],
  output: 'P2',
};

// ==================== Composable 使用 ====================
/**
 * useConfigStorage - 配置存储管理 composable
 *
 * 返回值：
 * - logicConfig: 当前逻辑电路配置对象（响应式）
 * - loading: 加载状态标志
 * - loadConfig: 从本地存储加载配置的方法
 * - saveConfig: 保存配置到本地存储的方法
 *
 * 参数：
 * - 'logic-circuit-config': 本地存储的键名
 * - defaultLogicConfig: 默认配置对象（当本地存储无数据时使用）
 */
const {
  config: logicConfig,
  loading,
  loadConfig,
  saveConfig,
} = useConfigStorage('logic-circuit-config', defaultLogicConfig);

// ==================== 生命周期钩子 ====================
/**
 * 组件挂载时执行
 *
 * 功能：自动加载已保存的逻辑电路配置
 *
 * 流程：
 * 1. 调用 loadConfig 方法
 * 2. 从本地存储读取配置
 * 3. 如果存在配置，更新 logicConfig
 * 4. 如果不存在，使用 defaultLogicConfig
 */
/**
 * 页面挂载后加载已保存的逻辑电路配置。
 */
onMounted(() => {
  loadConfig();
});
</script>

<style scoped>
.input-item {
  margin-bottom: 0.5rem;
}

.diagram-container {
  text-align: center;
}

.gate-diagram {
  display: inline-block;
  position: relative;
}

.gate-block {
  margin-bottom: 1.25rem;
}

.inputs {
  margin-bottom: 1.25rem;
}

.input-line {
  margin-bottom: 0.125rem;
}
</style>
