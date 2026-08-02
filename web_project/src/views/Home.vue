<!--
  Vue 3.5+ 主页组件 (使用 TanStack Query)

  【功能说明】
  这是应用的首页，包含设备配置和功能导航两个核心区域

  【核心概念】
  1. 设备配置：用户输入设备IP地址，测试连接并保存配置
  2. 设备状态：显示当前设备的连接状态和基本信息
  3. 功能导航：提供快速访问其他功能模块的卡片导航

  【数据流程】
  用户输入IP → 测试连接 → 获取设备信息 → 保存配置到本地存储

  【适用场景】
  - 用户首次使用应用时配置设备连接
  - 查看设备当前状态和基本信息
  - 快速导航到各个功能模块（状态机、GPIO、逻辑电路、WiFi）
-->
<template>
  <div class="home">
    <div class="home__container">
      <!-- 页面头部 -->
      <header class="home__header">
        <h1 class="home__title">ESP32 设备控制平台</h1>
        <p class="home__subtitle">设备管理与状态机配置系统</p>
      </header>

      <!-- 错误提示 -->
      <el-alert
        v-if="errorDevice"
        type="error"
        title="设备连接失败"
        :description="errorDevice?.message || '未知连接错误，请检查网络或IP地址'"
        show-icon
        :closable="false"
        class="mb-4"
      />

      <!-- 设备配置区域 -->
      <div class="home__config-grid">
        <BaseCard title="设备配置">
          <div class="home__form-content">
            <div>
              <label class="home__label">设备IP地址</label>
              <el-input v-model="deviceConfig.ip" placeholder="192.168.3.55" />
            </div>
            <div class="home__button-group">
              <el-button :loading="isLoadingDeviceInfo" @click="handleTestConnection">
                测试连接
              </el-button>
              <el-button type="primary" @click="handleSaveConfig"> 保存配置 </el-button>
            </div>
          </div>
        </BaseCard>

        <BaseCard title="设备状态">
          <div class="home__status-info">
            <el-tag :type="connectionTagType">
              {{ connectionText }}
            </el-tag>
            <p v-if="deviceInfoData" class="home__device-info">
              固件版本: {{ deviceInfoData.firmware }}<br />
              IP地址: {{ deviceInfoData.ipAddress }}
            </p>
          </div>

          <template #actions>
            <el-button size="small" :loading="isRefreshing" @click="refetchAllDeviceData">
              刷新
            </el-button>
          </template>
        </BaseCard>
      </div>

      <!-- 功能导航区域 -->
      <div class="home__nav-grid">
        <BaseCard
          v-for="card in NAVIGATION_CARDS"
          :key="card.path"
          :title="card.title"
          class="home__function-card"
          @click="navigateTo(card.path)"
        >
          <div class="home__card-content">
            <div class="home__card-icon">{{ card.icon }}</div>
            <p class="home__card-text">{{ card.text }}</p>
          </div>
        </BaseCard>
      </div>
    </div>
  </div>
</template>

<script lang="ts">
interface DeviceConfig {
  ip: string;
}

interface NavigationCard {
  title: string;
  path: string;
  icon: string;
  text: string;
}

const NAVIGATION_CARDS: readonly NavigationCard[] = [
  { title: '状态机 V3', path: '/state-machine-v3', icon: '⚙️', text: '完整格式状态机配置' },
  { title: 'GPIO 控制', path: '/gpio-control', icon: '🔌', text: 'GPIO 引脚控制' },
  { title: '逻辑电路', path: '/logic-circuit', icon: '🔗', text: '逻辑门配置' },
  { title: 'WiFi 配置', path: '/wifi-config', icon: '📶', text: '网络设置' },
];
</script>

<script setup lang="ts">
/**
 * Script Setup 部分
 *
 * 【依赖导入】
 * - Vue 3 组合式API：ref, computed, onMounted
 * - Vue Router：用于页面导航
 * - Element Plus：消息提示组件
 * - Composables：useDeviceInfo (设备信息获取)
 * - Services：storageService (本地存储操作)
 * - Components：BaseCard (基础卡片组件)
 */
import { ref, computed } from 'vue';
import { useRouter } from 'vue-router';
import { ElMessage } from 'element-plus';
import { useDeviceStatus } from '@/composables/useDeviceStatus';
import { setStorage, getStorage } from '@/services/storage/storageService';
import BaseCard from '@/components/base/BaseCard.vue';

// ==================== 实例初始化 ====================
const router = useRouter();

// ==================== 响应式状态 ====================
const savedIP = getStorage<string>('esp32-ip');

const deviceConfig = ref<DeviceConfig>({
  ip: savedIP || '192.168.3.55',
});

// ==================== TanStack Query 状态 ====================
/**
 * 使用 useDeviceInfo composable
 * 通过 TanStack Query 自动管理设备信息的获取、缓存和状态
 *
 * 返回值说明：
 * - deviceInfoData: 设备信息数据（包含固件版本、IP地址等）
 * - isLoadingDeviceInfo: 数据加载状态
 * - errorDevice: 错误信息
 * - refetchDeviceInfo: 重新获取数据方法
 */
const {
  deviceInfoData,
  isLoadingDeviceInfo,
  isLoadingDeviceStatus,
  isLoadingDeviceMetrics,
  isFetchingDeviceInfo,
  isFetchingDeviceStatus,
  isFetchingDeviceMetrics,
  errorDevice,
  refetchDeviceInfo,
  refetchAllDeviceData,
} = useDeviceStatus(computed(() => deviceConfig.value.ip));

const isRefreshing = computed(
  () => isFetchingDeviceInfo.value || isFetchingDeviceStatus.value || isFetchingDeviceMetrics.value
);
/**
 * 连接状态文本
 * 根据设备信息返回对应的状态文本
 *
 * @returns '连接失败' | '未连接' | '已连接'
 */
const connectionText = computed(() => {
  if (errorDevice.value) return '连接失败';
  if (!deviceInfoData.value) return '未连接';
  return '已连接';
});

/**
 * 连接状态标签类型
 * 用于 Element Plus Tag 组件的类型属性
 *
 * @returns 'danger' (连接失败) | 'info' (未连接) | 'success' (已连接)
 */
const connectionTagType = computed(() => {
  if (errorDevice.value) return 'danger';
  if (!deviceInfoData.value) return 'info';
  return 'success';
});

// ==================== 方法定义 ====================
/**
 * 导航到指定路径
 *
 * @param path - 目标路径（如 '/state-machine-v3'）
 *
 * 使用示例：
 * navigateTo('/gpio-control') // 导航到GPIO控制页面
 */
/**
 * 跳转到指定页面。
 *
 * @param path - 目标路由路径
 */
const navigateTo = (path: string) => {
  router.push(path);
};

/**
 * 测试设备连接
 *
 * 流程：
 * 1. 调用 refetchDeviceInfo 重新获取设备信息
 * 2. 根据返回结果显示成功或失败消息
 *
 * 错误处理：捕获异常并显示错误消息
 */
const handleTestConnection = async () => {
  const result = await refetchDeviceInfo();

  if (result.error || !result.data) {
    ElMessage.error('设备连接失败');
  } else {
    ElMessage.success('设备连接成功');
  }
};

const handleSaveConfig = () => {
  setStorage('esp32-ip', deviceConfig.value.ip);
  ElMessage.success('配置已保存');
};
</script>

<style scoped>
.home {
  min-height: 100vh;
  background-color: var(--el-bg-color-page);
  padding: 2rem;
}

.home__container {
  max-width: 1280px;
  margin: 0 auto;
}

.home__header {
  text-align: center;
  margin-bottom: 3rem;
}

.home__title {
  font-size: 2rem;
  font-weight: 700;
  color: var(--el-text-color-primary);
  margin: 0;
}

.home__subtitle {
  color: var(--el-text-color-secondary);
  margin-top: 0.5rem;
}

.home__config-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
  gap: 1.5rem;
  margin-bottom: 2rem;
}

.home__form-content {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.home__label {
  display: block;
  margin-bottom: 0.5rem;
  font-weight: 500;
  color: var(--el-text-color-regular);
}

.home__button-group {
  display: flex;
  gap: 0.5rem;
}

.home__status-info {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.home__device-info {
  font-size: 0.875rem;
  color: var(--el-text-color-secondary);
  margin: 0;
  line-height: 1.5;
}

.home__nav-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 1.5rem;
}

.home__function-card {
  cursor: pointer;
  transition:
    transform 0.2s,
    box-shadow 0.2s;
}

.home__function-card:hover {
  transform: translateY(-4px);
  box-shadow: 0 8px 16px rgba(0, 0, 0, 0.1);
}

.home__card-content {
  display: flex;
  flex-direction: column;
  align-items: center;
  text-align: center;
  gap: 0.5rem;
}

.home__card-icon {
  font-size: 2.5rem;
}

.home__card-text {
  color: var(--el-text-color-secondary);
  margin: 0;
}
</style>
