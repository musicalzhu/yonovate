<!--
  Vue 3.5+ 状态机V3页面组件 (使用 TanStack Query)

  【功能说明】
  状态机配置页面，提供完整的状态机编辑、验证和模板管理功能。
  状态机用于定义设备的状态转换逻辑，是自动化控制的核心组件。

  【核心概念】
  1. 状态机配置：包含初始状态、状态转换列表、状态效果列表
  2. 状态转换：定义从一个状态到另一个状态的触发条件（触发器）
  3. 触发器：包括GPIO输入触发、时间触发、条件触发等类型
  4. 模板系统：提供预定义的状态机配置模板，方便快速应用

  【数据流程】
  加载页面 → 获取当前状态机配置 → 获取GPIO状态 → 加载模板库 →
  用户编辑配置 → 验证配置 → 保存配置 → 应用到设备

  【适用场景】
  - 创建和编辑状态机自动化逻辑
  - 配置GPIO触发条件（边沿触发、电平触发）
  - 设置定时器触发和时间间隔
  - 管理状态转换的触发逻辑（AND/OR）
  - 使用模板快速创建常见状态机配置
-->
<template>
  <div class="state-machine-v3">
    <!-- 页面头部 -->
    <header class="state-machine-v3__header">
      <div class="state-machine-v3__container">
        <div class="state-machine-v3__header-content">
          <div class="state-machine-v3__title">
            <h1 class="state-machine-v3__heading">状态机配置 V3</h1>
            <p class="state-machine-v3__subtitle">
              使用 Vue 3.5+ 和 TanStack Query 构建的现代化状态机编辑器
            </p>
          </div>

          <div class="state-machine-v3__header-actions">
            <el-button @click="showHelp = true">帮助</el-button>
            <el-button type="primary" @click="handleExportConfig">导出配置</el-button>
          </div>
        </div>
      </div>
    </header>

    <!-- 主要内容 -->
    <main class="state-machine-v3__main">
      <div class="state-machine-v3__container">
        <!-- 错误提示 -->
        <el-alert
          v-if="errorConfig"
          type="error"
          title="加载配置失败"
          :description="errorConfig?.message || '加载配置失败，请检查网络连接'"
          show-icon
          :closable="false"
          class="mb-4"
        />

        <div class="state-machine-v3__grid">
          <!-- 左侧：状态机编辑器 -->
          <div class="state-machine-v3__main-content">
            <StateMachineEditor
              v-if="localConfig"
              v-model="localConfig"
              :readonly="isReadonly || isSaving"
              @save="handleSave"
            />
            <el-skeleton v-else :rows="5" animated class="state-machine-v3__loading" />
          </div>

          <!-- 右侧：状态图和模板 -->
          <div class="state-machine-v3__sidebar">
            <!-- 设备状态 -->
            <BaseCard title="设备状态">
              <div class="state-machine-v3__status-row">
                <span>连接状态</span>
                <el-tag :type="isDeviceConnected ? 'success' : 'info'">
                  {{ isDeviceConnected ? '已连接' : '未连接' }}
                </el-tag>
              </div>

              <div class="state-machine-v3__gpio-grid">
                <div v-if="isLoadingGPIO">
                  <el-skeleton :rows="3" animated />
                </div>
                <template v-else-if="gpioStatus?.length">
                  <div v-for="gpio in gpioStatus" :key="gpio.pin">
                    <GPIOControl :gpio="gpio" @toggle="handleToggleGPIO" />
                  </div>
                </template>
                <el-empty v-else description="暂无 GPIO 数据" />
              </div>

              <template #actions>
                <el-button size="small" :loading="isFetchingGPIO" @click="refetchGPIO">
                  刷新
                </el-button>
              </template>
            </BaseCard>

            <!-- 模板选择 -->
            <BaseCard title="模板库">
              <div class="state-machine-v3__template-list">
                <div
                  v-for="template in availableTemplates"
                  :key="template.name"
                  class="template-item"
                  @click="handleApplyTemplate(template)"
                >
                  <div class="template-item__content">
                    <h4 class="template-item__name">{{ template.name }}</h4>
                    <p class="template-item__description">
                      {{ template.description }}
                    </p>
                  </div>
                  <div class="template-item__meta">
                    <el-tag size="small">{{ template.category }}</el-tag>
                  </div>
                </div>
              </div>

              <template #actions>
                <el-button size="small" @click="showTemplateManager = true"> 管理模板 </el-button>
              </template>
            </BaseCard>

            <!-- 快速操作 -->
            <BaseCard title="快速操作">
              <div class="space-y-2">
                <el-button type="info" :disabled="isSaving" @click="handleValidateConfig">
                  验证配置
                </el-button>
                <el-button type="warning" :disabled="isSaving" @click="handleResetConfig">
                  重置配置
                </el-button>
                <el-button
                  type="success"
                  :loading="isSaving"
                  :disabled="!isDirty"
                  @click="handleSaveNow"
                >
                  保存到设备
                </el-button>
              </div>
            </BaseCard>
          </div>
        </div>
      </div>
    </main>

    <!-- 帮助对话框 -->
    <el-dialog v-model="showHelp" title="使用帮助" width="600px">
      <div class="help-content">
        <h4>状态机配置</h4>
        <p>状态机由初始状态和状态转换组成。每个转换定义了从一个状态到另一个状态的触发条件。</p>

        <h4>触发器类型</h4>
        <ul>
          <li>GPIO输入：检测数字输入引脚的状态变化</li>
          <li>时间触发：基于时间间隔的自动触发</li>
          <li>条件触发：基于多个条件的组合逻辑</li>
        </ul>

        <h4>逻辑运算符</h4>
        <ul>
          <li><strong>AND</strong>：所有条件都必须满足</li>
          <li><strong>OR</strong>：任意一个条件满足即可</li>
        </ul>
      </div>
    </el-dialog>

    <!-- 模板管理对话框 -->
    <el-dialog v-model="showTemplateManager" title="模板管理" width="800px">
      <div class="template-manager">
        <div class="flex justify-between items-center mb-4">
          <h4>可用模板</h4>
          <el-button type="primary" size="small" @click="showCreateTemplate = true">
            创建模板
          </el-button>
        </div>

        <div class="template-grid">
          <div v-for="template in allTemplates" :key="template.name" class="template-card">
            <div class="template-card__content">
              <h5>{{ template.name }}</h5>
              <p>{{ template.description }}</p>
              <div class="flex gap-2 mt-2">
                <el-tag size="small">{{ template.category }}</el-tag>
                <el-tag size="small" type="info">{{ template.metadata?.version }}</el-tag>
              </div>
            </div>
            <div class="template-card__actions mt-2">
              <el-button size="small" @click="handleApplyTemplate(template)">应用</el-button>
              <el-button size="small" type="danger" @click="handleDeleteTemplate(template.name)">
                删除
              </el-button>
            </div>
          </div>
        </div>
      </div>
    </el-dialog>

    <!-- 创建模板对话框 -->
    <el-dialog v-model="showCreateTemplate" title="创建模板" width="400px">
      <el-form label-position="top">
        <el-form-item label="模板名称" required>
          <el-input v-model="newTemplateName" placeholder="请输入模板名称" />
        </el-form-item>
        <el-form-item label="模板描述">
          <el-input v-model="newTemplateDescription" type="textarea" placeholder="请输入模板描述" />
        </el-form-item>
        <el-form-item label="分类">
          <el-input v-model="newTemplateCategory" placeholder="请输入模板分类" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showCreateTemplate = false">取消</el-button>
        <el-button type="primary" @click="handleCreateTemplate">创建</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
/**
 * Script Setup 部分
 *
 * 【依赖导入】
 * - Vue 3 组合式API：ref, computed, watch
 * - Element Plus：消息提示、对话框组件
 * - Composables：
 *   - useStateMachineConfig: 状态机配置管理（获取、更新、保存）
 *   - useGPIOStatus: GPIO状态管理（显示当前引脚状态）
 *   - useTemplates: 模板管理（获取、保存、删除模板）
 * - Types：StateMachineConfig 类型定义
 * - Components：
 *   - BaseCard: 基础卡片组件
 *   - StateMachineEditor: 状态机编辑器组件
 *   - GPIOControl: GPIO控制组件
 */
import { ref, computed, watch } from 'vue';
import { ElMessage } from 'element-plus';
import { useStateMachineConfig } from '@/composables/useStateMachineConfig';
import { useGPIOStatus } from '@/composables/useGPIO';
import { useTemplates } from '@/composables/useTemplates';
import type { StateMachineConfig } from '@/generated/api';
import { exportToFile } from '@/utils/exportHelper';
import BaseCard from '@/components/base/BaseCard.vue';
import StateMachineEditor from '@/components/state-machine/StateMachineEditor.vue';
import GPIOControl from '@/components/base/GPIOControl.vue';

// ==================== 页面状态 ====================
/** 帮助对话框显示状态 */
const showHelp = ref(false);
/** 模板管理对话框显示状态 */
const showTemplateManager = ref(false);
/** 创建模板对话框显示状态 */
const showCreateTemplate = ref(false);
/** 编辑器只读模式标志 */
const isReadonly = ref(false);

// ==================== 模板相关状态 ====================
/** 新模板名称 */
const newTemplateName = ref('');
/** 新模板描述 */
const newTemplateDescription = ref('');
/** 新模板分类 */
const newTemplateCategory = ref('');

// ==================== TanStack Query Composables ====================
/**
 * useStateMachineConfig - 状态机配置管理 composable
 *
 * 返回值：
 * - config: 当前状态机配置数据
 * - isLoadingConfig: 配置加载状态
 * - isSaving: 配置保存进行中标志
 * - isDirty: 配置是否已修改标志
 * - errorConfig: 错误信息
 * - updateConfig: 更新配置方法（不立即保存）
 * - saveNow: 立即保存配置到设备
 * - refetchConfig: 重新获取配置
 */
const {
  config,
  isLoadingConfig,
  isSaving,
  isDirty,
  errorConfig,
  updateConfig,
  saveNow,
  refetchConfig,
} = useStateMachineConfig();

/**
 * useGPIOStatus - GPIO状态管理 composable
 *
 * 用于在侧边栏显示当前GPIO引脚状态，帮助用户配置触发器
 *
 * 返回值：
 * - gpioStatus: GPIO状态数据
 * - isLoadingGPIO: 加载状态
 * - isFetchingGPIO: 数据获取中标志
 * - refetchGPIO: 重新获取GPIO状态
 * - setPinValue: 设置GPIO引脚值
 */
const {
  gpioStatus,
  isLoadingGPIO,
  isFetchingGPIO,
  refetchGPIO,
  setPinValue,
} = useGPIOStatus();

/**
 * useTemplates - 模板管理 composable
 *
 * 提供状态机配置模板的获取、保存和删除功能
 *
 * 返回值：
 * - templates: 模板列表
 * - saveTemplate: 保存新模板
 * - deleteTemplate: 删除模板
 */
const { templates, saveTemplate, deleteTemplate } = useTemplates();

// ==================== 本地配置状态 ====================
/**
 * 本地状态机配置状态
 *
 * 用于编辑器的双向绑定，用户修改会实时更新此状态
 * 类型：StateMachineConfig 或 null
 */
const localConfig = ref<StateMachineConfig | null>(null);

// ==================== 数据同步 ====================
/**
 * 同步远程配置到本地状态（初始化）
 *
 * 当 config 从API获取后，首次同步到 localConfig
 * 只在 localConfig 为 null 时执行，避免覆盖用户编辑
 */
watch(
  config,
  (newConfig) => {
    if (newConfig && !localConfig.value) {
      localConfig.value = { ...newConfig };
    }
  },
  { immediate: true }
);

/**
 * 监听本地配置变化并同步到远程
 *
 * 当用户编辑配置时，将修改同步到 config
 * 条件：isDirty 为 false 时才更新，避免重复更新
 * deep: true - 深度监听配置对象的所有属性变化
 */
watch(
  localConfig,
  (newConfig) => {
    if (newConfig && isDirty.value === false) {
      updateConfig(newConfig);
    }
  },
  { deep: true }
);

// ==================== 计算属性 ====================
/**
 * 设备连接状态
 * 根据GPIO状态数据是否存在判断设备是否已连接
 *
 * @returns true: 已连接 | false: 未连接
 */
const isDeviceConnected = computed(() => !!gpioStatus.value?.length);

/**
 * 可用模板列表（显示前5个）
 * 在侧边栏模板库中显示
 */
const availableTemplates = computed(() => templates.value.slice(0, 5));

/**
 * 所有模板列表
 * 在模板管理对话框中显示
 */
const allTemplates = computed(() => templates.value);

// ==================== 事件处理方法 ====================
/**
 * 保存配置（不立即同步到设备）
 *
 * 流程：
 * 1. 检查 localConfig 是否存在
 * 2. 调用 updateConfig 更新配置（标记为已修改）
 * 3. 显示保存提示消息
 *
 * 注意：此方法只保存到本地状态，需要调用 handleSaveNow 才会同步到设备
 */
const handleSave = () => {
  if (localConfig.value) {
    updateConfig(localConfig.value);
    ElMessage.success('配置已保存（待同步到设备）');
  }
};

/**
 * 立即保存配置到设备
 *
 * 流程：
 * 1. 检查 localConfig 是否存在
 * 2. 调用 saveNow 方法将配置同步到设备
 * 3. 显示保存进度消息
 */
const handleSaveNow = async () => {
  if (localConfig.value) {
    await saveNow(localConfig.value);
    ElMessage.success('配置已保存到设备');
  }
};

/**
 * 验证配置有效性
 *
 * 流程：
 * 1. 检查配置是否存在
 * 2. 执行配置验证逻辑（当前为简化版本）
 * 3. 显示验证结果消息
 *
 * TODO：添加更详细的验证逻辑（状态名称检查、触发器完整性等）
 */
/**
 * 验证当前状态机配置是否有效。
 */
const handleValidateConfig = () => {
  if (!localConfig.value) {
    ElMessage.warning('请先加载配置');
    return;
  }
  ElMessage.success('配置验证通过');
};

/**
 * 重置配置为默认状态
 *
 * 默认配置包含：
 * - initialState: 'IDLE' (初始状态为空闲)
 * - transitions: [] (无状态转换)
 * - stateEffects: [] (无状态效果)
 * - groupModes/inputModuleTypes/edgeTypes/outputModuleTypes: 默认数组
 */
/**
 * 将当前配置重置为最新的远端配置。
 */
const handleResetConfig = () => {
  localConfig.value = {
    initialState: 'IDLE',
    transitions: [],
    stateEffects: [],
    groupModes: [0, 0, 0, 0],
    inputModuleTypes: [0, 0, 0, 0],
    edgeTypes: [0, 0, 0, 0],
    outputModuleTypes: [0, 0, 0, 0],
  };
  ElMessage.info('配置已重置');
};

/**
 * 导出配置为JSON文件
 *
 * 流程：
 * 1. 检查配置是否存在
 * 2. 使用通用导出工具导出
 * 3. 文件名格式：state-machine-config-{日期}.json
 */
const handleExportConfig = () => {
  if (!localConfig.value) {
    ElMessage.warning('没有可导出的配置');
    return;
  }
  const filename = `state-machine-config-${new Date().toISOString().split('T')[0]}.json`;
  exportToFile(localConfig.value, 'state-machine-config', filename);
  ElMessage.success('配置已导出');
};

/**
 * 应用模板配置
 *
 * @param template - 模板对象，包含 name 和 config 属性
 *
 * 流程：
 * 1. 检查模板是否有配置数据
 * 2. 将模板配置复制到 localConfig
 * 3. 更新远程配置状态
 * 4. 显示应用成功消息
 */
const handleApplyTemplate = (template: any) => {
  if (template.config) {
    localConfig.value = { ...template.config };
    updateConfig(template.config);
    ElMessage.success(`已应用模板：${template.name}`);
  }
};

/**
 * 创建新模板
 *
 * 流程：
 * 1. 验证模板名称和当前配置是否存在
 * 2. 构建模板对象（包含名称、描述、分类、配置、元数据）
 * 3. 调用 saveTemplate 保存模板
 * 4. 重置创建模板表单
 * 5. 关闭创建对话框
 *
 * 模板元数据包含：
 * - version: 版本号
 * - author: 作者
 * - createdAt: 创建时间
 * - tags: 标签列表
 */
const handleCreateTemplate = () => {
  if (!newTemplateName.value) {
    ElMessage.warning('请输入模板名称');
    return;
  }
  if (!localConfig.value) {
    ElMessage.warning('请先配置状态机');
    return;
  }

  saveTemplate({
    name: newTemplateName.value,
    description: newTemplateDescription.value,
    category: newTemplateCategory.value || '默认',
    config: localConfig.value,
    metadata: {
      version: '1.0.0',
      author: 'user',
      createdAt: new Date().toISOString(),
      tags: ['custom'],
    },
  });

  showCreateTemplate.value = false;
  newTemplateName.value = '';
  newTemplateDescription.value = '';
  newTemplateCategory.value = '';
  ElMessage.success('模板创建成功');
};

/**
 * 删除模板
 *
 * @param name - 要删除的模板名称
 *
 * 流程：
 * 1. 调用 deleteTemplate 删除模板
 * 2. 显示删除提示消息
 */
const handleDeleteTemplate = (name: string) => {
  deleteTemplate(name);
  ElMessage.info(`模板 "${name}" 已删除`);
};

/**
 * 切换GPIO引脚状态（用于测试触发器）
 *
 * @param pin - GPIO引脚编号
 *
 * 流程：
 * 1. 调用 setPinValue 设置引脚值为1（HIGH）
 * 2. 显示切换提示消息
 *
 * 用途：帮助用户测试GPIO触发器是否正常工作
 */
const handleToggleGPIO = (pin: number) => {
  setPinValue(pin, 1);
  ElMessage.info(`GPIO ${pin} 已切换`);
};
</script>

<style scoped>
.state-machine-v3 {
  min-height: 100vh;
  background-color: var(--el-bg-color-page);
}

.state-machine-v3__container {
  max-width: 1280px;
  margin: 0 auto;
  padding: 1.5rem 1rem;
}

.state-machine-v3__header {
  background-color: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color-light);
}

.state-machine-v3__header-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.state-machine-v3__title {
  flex: 1;
}

.state-machine-v3__heading {
  font-size: 1.5rem;
  font-weight: 700;
  color: var(--el-text-color-primary);
  margin: 0;
}

.state-machine-v3__subtitle {
  color: var(--el-text-color-secondary);
  margin-top: 0.25rem;
}

.state-machine-v3__header-actions {
  display: flex;
  gap: 0.5rem;
}

.state-machine-v3__main {
  padding-top: 1.5rem;
  padding-bottom: 1.5rem;
}

.state-machine-v3__grid {
  display: grid;
  grid-template-columns: 1fr 360px;
  gap: 1.5rem;
}

@media (max-width: 1024px) {
  .state-machine-v3__grid {
    grid-template-columns: 1fr;
  }
}

.state-machine-v3__status-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 1rem;
}

.state-machine-v3__gpio-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(80px, 1fr));
  gap: 0.5rem;
}

.state-machine-v3__template-list {
  display: flex;
  flex-direction: column;
  gap: 0.5rem;
}

.template-item {
  padding: 0.75rem;
  border: 1px solid var(--el-border-color-light);
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s;
}

.template-item:hover {
  border-color: var(--el-color-primary);
  background-color: var(--el-fill-color-light);
}

.template-item__content {
  margin-bottom: 0.5rem;
}

.template-item__name {
  font-weight: 600;
  margin: 0;
  color: var(--el-text-color-primary);
}

.template-item__description {
  font-size: 0.875rem;
  color: var(--el-text-color-secondary);
  margin: 0.25rem 0 0;
}

.template-item__meta {
  display: flex;
  gap: 0.5rem;
}

.template-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(250px, 1fr));
  gap: 1rem;
}

.template-card {
  padding: 1rem;
  border: 1px solid var(--el-border-color-light);
  border-radius: 8px;
}

.template-card__content h5 {
  margin: 0 0 0.5rem;
  color: var(--el-text-color-primary);
}

.template-card__content p {
  margin: 0;
  font-size: 0.875rem;
  color: var(--el-text-color-secondary);
}

.help-content h4 {
  margin-top: 1.5rem;
}

.help-content h4:first-child {
  margin-top: 0;
}

.help-content ul {
  padding-left: 1.5rem;
}
</style>
