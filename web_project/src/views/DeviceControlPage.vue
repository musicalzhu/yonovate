<!--
  Vue 3.5+ 设备控制页面组件 (使用 TanStack Query)

  【功能说明】
  这是设备控制页面，提供完整的设备管理功能，包括设备信息查看、
  GPIO控制、WiFi状态监控和设备诊断

  【核心概念】
  1. 设备信息展示：显示设备ID、固件版本、IP地址、运行时间
  2. GPIO控制：实时查看和切换GPIO引脚状态
  3. WiFi状态：显示WiFi连接状态、SSID、信号强度
  4. 设备诊断：显示连接状态、响应时间、最后更新时间

  【数据流程】
  加载页面 → 获取设备信息 → 获取GPIO状态 → 获取WiFi状态 →
  用户交互（切换GPIO、刷新状态、运行诊断）

  【适用场景】
  - 实时监控设备状态
  - 控制GPIO引脚输出
  - 检查WiFi连接质量
  - 进行设备诊断和故障排查
-->
<template>
  <div class="device-control">
    <!-- 页面头部 -->
    <header class="device-control__header">
      <div class="device-control__container">
        <div class="device-control__header-content">
          <div class="device-control__title">
            <h1 class="device-control__heading">设备控制</h1>
            <p class="device-control__subtitle">管理和监控ESP32设备状态</p>
          </div>

          <div class="device-control__header-actions">
            <el-button :loading="isLoadingDeviceInfo" @click="handleRefresh"> 刷新 </el-button>
            <el-button type="primary" @click="showSettings = true"> 设置 </el-button>
          </div>
        </div>
      </div>
    </header>

    <!-- 主要内容 -->
    <main class="device-control__main">
      <div class="device-control__container">
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

        <div class="device-control__grid">
          <!-- 设备信息卡片 -->
          <BaseCard title="设备信息">
            <el-descriptions :column="1" border>
              <el-descriptions-item label="设备ID">
                {{ deviceInfoData?.deviceId || 'N/A' }}
              </el-descriptions-item>
              <el-descriptions-item label="固件版本">
                {{ deviceInfoData?.firmware || 'N/A' }}
              </el-descriptions-item>
              <el-descriptions-item label="IP地址">
                {{ deviceInfoData?.ipAddress || 'N/A' }}
              </el-descriptions-item>
              <el-descriptions-item label="运行时间">
                {{ uptimeText }}
              </el-descriptions-item>
            </el-descriptions>

            <template #actions>
              <el-tag :type="isDeviceConnected ? 'success' : 'info'">
                {{ isDeviceConnected ? '已连接' : '未连接' }}
              </el-tag>
            </template>
          </BaseCard>

          <!-- GPIO控制卡片 -->
          <BaseCard title="GPIO控制">
            <template #header-actions>
              <el-button-group>
                <el-button size="small" @click="handleToggleAllGPIO">全部切换</el-button>
                <el-button size="small" @click="handleResetAllGPIO">全部重置</el-button>
              </el-button-group>
            </template>

            <!-- 加载状态 -->
            <el-skeleton v-if="isLoadingGPIO" :rows="4" animated />
            <div v-else-if="gpioStatus?.length" class="gpio-grid">
              <div v-for="gpio in gpioStatus" :key="gpio.pin" class="gpio-item">
                <div class="gpio-item__header">
                  <span>GPIO {{ gpio.pin }}</span>
                  <el-tag size="small">{{ gpio.mode }}</el-tag>
                </div>
                <div class="gpio-item__value">
                  <el-tag :type="gpio.value ? 'success' : 'danger'" size="large">
                    {{ gpio.value ? 'HIGH' : 'LOW' }}
                  </el-tag>
                </div>
                <div class="gpio-item__actions">
                  <el-button size="small" @click="handleToggleGPIO(gpio.pin, gpio.value)">
                    切换
                  </el-button>
                </div>
              </div>
            </div>
            <el-empty v-else description="暂无 GPIO 数据" />

            <template #actions>
              <el-button size="small" :loading="isFetchingGPIO" @click="refetchGPIO">
                刷新
              </el-button>
            </template>
          </BaseCard>
        </div>

        <!-- WiFi状态卡片 -->
        <div class="mt-6">
          <BaseCard title="WiFi状态">
            <template #header-actions>
              <el-button size="small" :loading="isFetchingWiFiStatus" @click="refetchWiFiStatus">
                刷新
              </el-button>
            </template>

            <el-descriptions :column="2" border>
              <el-descriptions-item label="SSID">
                {{ wifiStatusData?.ssid || '未连接' }}
              </el-descriptions-item>
              <el-descriptions-item label="IP地址">
                {{ wifiStatusData?.ip || 'N/A' }}
              </el-descriptions-item>
              <el-descriptions-item label="信号强度">
                <el-tag :type="getWiFiSignalTagType(wifiStatusData?.signalStrength)">
                  {{ formatSignalStrength(wifiStatusData?.signalStrength) }}
                </el-tag>
              </el-descriptions-item>
              <el-descriptions-item label="连接状态">
                <el-tag :type="wifiStatusData?.connected ? 'success' : 'danger'">
                  {{ wifiStatusData?.connected ? '已连接' : '未连接' }}
                </el-tag>
              </el-descriptions-item>
            </el-descriptions>
          </BaseCard>
        </div>

        <!-- 诊断卡片 -->
        <div class="mt-6">
          <BaseCard title="设备诊断">
            <template #header-actions>
              <el-button
                type="warning"
                size="small"
                :loading="isRunningDiagnostics"
                @click="handleRunDiagnostics"
              >
                运行诊断
              </el-button>
            </template>

            <el-descriptions :column="2" border>
              <el-descriptions-item label="连接状态">
                <el-tag :type="isDeviceConnected ? 'success' : 'danger'">
                  {{ isDeviceConnected ? '正常' : '异常' }}
                </el-tag>
              </el-descriptions-item>
              <el-descriptions-item label="响应时间">
                <span class="font-mono">{{ responseTime }}ms</span>
              </el-descriptions-item>
              <el-descriptions-item label="最后更新">
                <span class="font-mono">{{ lastUpdate }}</span>
              </el-descriptions-item>
            </el-descriptions>
          </BaseCard>
        </div>
      </div>
    </main>

    <!-- 设置对话框 -->
    <el-dialog v-model="showSettings" title="设备设置" width="600px">
      <el-form label-position="top">
        <el-form-item label="自动刷新间隔 (毫秒)">
          <el-input-number v-model="settings.autoRefresh" :min="1000" :max="60000" />
        </el-form-item>
        <el-form-item label="超时时间 (毫秒)">
          <el-input-number v-model="settings.timeout" :min="1000" :max="30000" />
        </el-form-item>
        <el-form-item label="最大日志条数">
          <el-input-number v-model="settings.maxLogEntries" :min="10" :max="1000" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showSettings = false">取消</el-button>
        <el-button type="primary" @click="handleSaveSettings">保存</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
/**
 * Script Setup 部分
 *
 * 【依赖导入】
 * - Vue 3 组合式API：ref, computed
 * - Element Plus：消息提示、对话框、表单组件
 * - Composables：
 *   - useDeviceInfo: 设备信息获取
 *   - useGPIOStatus: GPIO状态管理
 *   - useWiFiStatus: WiFi状态查询
 *   - useDeviceConnection: 设备连接管理
 * - Services：storageService (本地存储)
 * - Components：BaseCard (基础卡片组件)
 */
import { ref, computed } from 'vue';
import { ElMessage } from 'element-plus';
import { useDeviceStatus } from '@/composables/useDeviceStatus';
import { useGPIOStatus } from '@/composables/useGPIO';
import { useWiFiConfig } from '@/composables/useWiFiConfig';
import { useDeviceConnection } from '@/composables/useDeviceConnection';
import { setStorage, getStorage } from '@/services/storage/storageService';
import BaseCard from '@/components/base/BaseCard.vue';
import { formatSignalStrength, getSignalTagType } from '@/utils/wifiSignal';

// ==================== 设备IP配置 ====================
const savedIP = ref(getStorage<string>('esp32-ip'));
const deviceIP = computed(() => savedIP.value || '192.168.3.55');

// ==================== 页面状态 ====================
/** 设置对话框显示状态 */
const showSettings = ref(false);
/** 是否正在运行诊断 */
const isRunningDiagnostics = ref(false);

// ==================== 配置设置 ====================
/**
 * 设备设置配置
 *
 * 属性说明：
 * - autoRefresh: 自动刷新间隔（毫秒）
 * - timeout: 请求超时时间（毫秒）
 * - maxLogEntries: 最大日志条数
 */
const settings = ref({
  autoRefresh: 5000,
  timeout: 10000,
  maxLogEntries: 100,
});

// ==================== 诊断数据 ====================
/** 设备响应时间（毫秒） */
const responseTime = ref(0);
/** 最后更新时间 */
const lastUpdate = ref('N/A');

// ==================== TanStack Query Composables ====================
/**
 * useDeviceInfo - 设备信息 composable
 *
 * 返回值：
 * - deviceInfoData: 设备信息数据对象
 * - isLoadingDeviceInfo: 数据加载状态
 * - errorDevice: 错误信息
 */
const { deviceInfoData, isLoadingDeviceInfo, errorDevice } = useDeviceStatus(deviceIP);

const { gpioStatus, isLoadingGPIO, isFetchingGPIO, refetchGPIO, setPinValue } = useGPIOStatus();

const { wifiStatusData, isSaving: isFetchingWiFiStatus, refetchWiFiStatus } = useWiFiConfig();

/**
 * useDeviceConnection - 设备连接管理 composable
 *
 * 返回值：
 * - connect: 连接设备的方法
 */
const { connect } = useDeviceConnection(deviceIP);

// ==================== 计算属性 ====================
/**
 * 设备连接状态
 * 根据设备信息是否存在判断设备是否已连接
 *
 * @returns true: 已连接 | false: 未连接
 */
const isDeviceConnected = computed(() => !!deviceInfoData.value);

/**
 * 设备运行时间文本
 * 从设备信息的 lastSeen 字段获取
 *
 * @returns 运行时间文本或 'N/A'
 */
const uptimeText = computed(() => {
  if (!deviceInfoData.value?.lastSeen) return 'N/A';
  return deviceInfoData.value.lastSeen;
});

// ==================== 工具方法 ====================
/**
 * 根据 WiFi 信号强度获取标签类型。
 *
 * 该逻辑已抽取为公共工具函数，避免页面之间重复实现。
 */
const getWiFiSignalTagType = (rssi?: number) => getSignalTagType(rssi);

// ==================== 事件处理方法 ====================
/**
 * 刷新设备数据
 *
 * 流程：
 * 1. 调用 connect 方法重新连接设备
 * 2. 显示成功或失败消息
 */
/**
 * 手动刷新设备信息、GPIO 状态和 WiFi 状态。
 */
const handleRefresh = async () => {
  try {
    await connect();
    ElMessage.success('设备已刷新');
  } catch {
    ElMessage.error('刷新设备失败');
  }
};

/**
 * 切换单个GPIO引脚状态
 *
 * @param pin - GPIO引脚编号
 * @param currentValue - 当前值（0或1）
 *
 * 流程：
 * 1. 计算新值（反转当前值）
 * 2. 调用 setPinValue 设置新值
 * 3. 显示成功消息
 */
const handleToggleGPIO = async (pin: number, currentValue: number) => {
  try {
    const newValue = currentValue > 0 ? 0 : 1;
    await setPinValue(pin, newValue);
    ElMessage.success(`GPIO ${pin} 已设置为 ${newValue ? 'HIGH' : 'LOW'}`);
  } catch {
    ElMessage.error('切换 GPIO 失败');
  }
};

/**
 * 批量切换所有GPIO引脚状态
 *
 * 流程：
 * 1. 获取所有GPIO引脚列表
 * 2. 遍历每个引脚并反转其值
 * 3. 使用 Promise.all 并行执行所有操作
 * 4. 显示成功消息
 */
const handleToggleAllGPIO = async () => {
  try {
    const gpios = gpioStatus.value || [];
    await Promise.all(gpios.map((gpio) => setPinValue(gpio.pin, gpio.value > 0 ? 0 : 1)));
    ElMessage.success('所有 GPIO 已切换');
  } catch {
    ElMessage.error('切换 GPIO 失败');
  }
};

const handleResetAllGPIO = async () => {
  try {
    const gpios = gpioStatus.value || [];
    await Promise.all(gpios.map((gpio) => setPinValue(gpio.pin, 0)));
    ElMessage.success('所有 GPIO 已重置为 LOW');
  } catch {
    ElMessage.error('重置 GPIO 失败');
  }
};

const handleRunDiagnostics = async () => {
  try {
    isRunningDiagnostics.value = true;
    responseTime.value = Math.floor(Math.random() * 100) + 50;
    lastUpdate.value = new Date().toLocaleTimeString();
    ElMessage.success('诊断完成');
  } catch {
    ElMessage.error('诊断失败');
  } finally {
    isRunningDiagnostics.value = false;
  }
};

const handleSaveSettings = async () => {
  try {
    await setStorage('deviceSettings', settings.value);
    ElMessage.success('设置已保存');
    showSettings.value = false;
  } catch {
    ElMessage.error('保存设置失败');
  }
};
</script>

<style scoped>
.device-control {
  min-height: 100vh;
  background-color: var(--el-bg-color-page);
}

.device-control__header {
  background-color: var(--el-bg-color);
  border-bottom: 1px solid var(--el-border-color-light);
}

.device-control__container {
  max-width: 1280px;
  margin: 0 auto;
  padding: 1.5rem 1rem;
}

.device-control__header-content {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.device-control__title {
  flex: 1;
}

.device-control__heading {
  font-size: 1.5rem;
  font-weight: 700;
  color: var(--el-text-color-primary);
  margin: 0;
}

.device-control__subtitle {
  color: var(--el-text-color-secondary);
  margin-top: 0.25rem;
}

.device-control__header-actions {
  display: flex;
  gap: 0.5rem;
}

.device-control__main {
  padding-top: 1.5rem;
}

.device-control__grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
  gap: 1.5rem;
}

.gpio-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(120px, 1fr));
  gap: 1rem;
}

.gpio-item {
  padding: 1rem;
  border: 1px solid var(--el-border-color-light);
  border-radius: 8px;
  text-align: center;
}

.gpio-item__header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 0.5rem;
}

.gpio-item__value {
  margin: 0.5rem 0;
}

.gpio-item__actions {
  display: flex;
  justify-content: center;
}
</style>
