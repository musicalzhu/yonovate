<!--
  Vue 3.5+ WiFi 配置页面组件 (使用 TanStack Query)

  【功能说明】
  WiFi配置管理页面，提供完整的WiFi网络配置功能，包括：
  基础网络设置、静态IP配置、连接状态监控和可用网络扫描

  【核心概念】
  1. 网络设置：配置SSID、密码、IP模式（DHCP/静态）
  2. 静态IP配置：当选择静态IP模式时，配置IP地址、子网掩码、网关、DNS
  3. 连接状态：实时显示当前WiFi连接状态、SSID、信号强度
  4. 可用网络：扫描并列出可用的WiFi网络

  【数据流程】
  加载页面 → 获取当前WiFi配置 → 同步到本地状态 →
  用户编辑配置 → 保存配置 → 设备应用新配置

  【适用场景】
  - 配置设备WiFi连接参数
  - 切换DHCP和静态IP模式
  - 扫描并选择可用的WiFi网络
  - 查看当前WiFi连接质量
-->
<template>
  <PageLayout title="WiFi 配置" subtitle="配置和管理WiFi网络连接">
    <template #actions>
      <div class="mb-6 flex gap-2">
        <el-button :loading="isLoadingConfig" @click="refetchConfig"> 加载配置 </el-button>
        <el-button type="primary" :loading="isSaving" @click="handleSaveConfig">
          保存配置
        </el-button>
      </div>
    </template>

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

    <BaseCard title="网络设置">
      <div class="space-y-6">
        <div>
          <label class="block text-sm font-medium mb-2">SSID</label>
          <el-input v-model="config.ssid" placeholder="WiFi网络名称" />
        </div>

        <div>
          <label class="block text-sm font-medium mb-2">密码</label>
          <el-input
            v-model="config.password"
            type="password"
            placeholder="WiFi密码"
            show-password
          />
        </div>

        <div>
          <label class="block text-sm font-medium mb-2">IP模式</label>
          <el-radio-group v-model="ipMode">
            <el-radio :value="true">DHCP (自动获取)</el-radio>
            <el-radio :value="false">静态IP</el-radio>
          </el-radio-group>
        </div>

        <template v-if="!config.dhcp">
          <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div>
              <label class="block text-sm font-medium mb-2">IP地址</label>
              <el-input v-model="config.ip" placeholder="192.168.1.100" />
            </div>
            <div>
              <label class="block text-sm font-medium mb-2">子网掩码</label>
              <el-input v-model="config.subnet" placeholder="255.255.255.0" />
            </div>
            <div>
              <label class="block text-sm font-medium mb-2">网关</label>
              <el-input v-model="config.gateway" placeholder="192.168.1.1" />
            </div>
            <div>
              <label class="block text-sm font-medium mb-2">DNS</label>
              <el-input v-model="config.dns" placeholder="8.8.8.8" />
            </div>
          </div>
        </template>
      </div>
    </BaseCard>

    <BaseCard title="连接状态" class="mt-6">
      <!-- 加载状态 -->
      <el-skeleton v-if="isLoadingStatus" :rows="3" animated />
      <template v-else>
        <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div class="status-item">
            <span class="font-bold">当前SSID:</span>
            <el-tag>{{ wifiStatusData?.ssid || '未连接' }}</el-tag>
          </div>
          <div class="status-item">
            <span class="font-bold">IP地址:</span>
            <el-tag type="success">{{ wifiStatusData?.ip || '未获取' }}</el-tag>
          </div>
          <div class="status-item">
            <span class="font-bold">信号强度:</span>
            <el-tag :type="getSignalTagType(wifiStatusData?.rssi)">
              {{ formatSignalStrength(wifiStatusData?.rssi) }}
            </el-tag>
          </div>
          <div class="status-item">
            <span class="font-bold">连接状态:</span>
            <el-tag :type="wifiStatusData?.connected ? 'success' : 'danger'">
              {{ wifiStatusData?.connected ? '已连接' : '未连接' }}
            </el-tag>
          </div>
        </div>
      </template>
      <template #actions>
        <el-button size="small" :loading="isFetchingWiFiStatus" @click="refetchWiFiStatus">
          刷新
        </el-button>
      </template>
    </BaseCard>

    <BaseCard title="可用网络" class="mt-6">
      <div class="space-y-4">
        <el-button :loading="isScanning" @click="handleScanNetworks">扫描网络</el-button>

        <el-empty v-if="!networksData?.length" description="暂无可用网络" />
        <el-table v-else :data="networksData" style="width: 100%">
          <el-table-column prop="ssid" label="SSID" />
          <el-table-column prop="rssi" label="信号强度" width="100">
            <template #default="{ row }">
              <el-tag :type="getSignalTagType(row.rssi)">{{
                formatSignalStrength(row.rssi)
              }}</el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="secure" label="加密类型" width="100" />
          <el-table-column label="操作" width="100">
            <template #default="{ row }">
              <el-button size="small" @click="handleSelectNetwork(row)">选择</el-button>
            </template>
          </el-table-column>
        </el-table>
      </div>
    </BaseCard>
  </PageLayout>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';
import { ElMessage } from 'element-plus';
import { useWiFiConfig } from '@/composables/useWiFiConfig';
import type { WiFiConfig } from '@/generated/api';
import PageLayout from '@/components/base/PageLayout.vue';
import BaseCard from '@/components/base/BaseCard.vue';
import { formatSignalStrength, getSignalTagType } from '@/utils/wifiSignal';

const {
  wifiConfigData,
  isLoadingConfig,
  errorConfig,
  refetchConfig,
  saveConfig,
  wifiStatusData,
  isLoadingStatus,
  isSaving: isFetchingWiFiStatus,
  refetchWiFiStatus,
  networksData,
  isScanning,
  refetchNetworks,
} = useWiFiConfig();

const config = ref<WiFiConfig>({
  ssid: '',
  password: '',
  dhcp: true,
});

const ipMode = computed({
  get: () => config.value.dhcp ?? true,
  set: (value) => {
    config.value.dhcp = value;
  },
});

const isSaving = ref(false);

const handleSaveConfig = async () => {
  isSaving.value = true;
  try {
    await saveConfig(config.value);
    ElMessage.success('WiFi 配置已保存');
  } catch {
    ElMessage.error('保存 WiFi 配置失败');
  } finally {
    isSaving.value = false;
  }
};

const handleScanNetworks = async () => {
  try {
    await refetchNetworks();
    ElMessage.success('网络扫描完成');
  } catch {
    ElMessage.error('网络扫描失败');
  }
};

const handleSelectNetwork = (network: { ssid: string }) => {
  config.value.ssid = network.ssid;
  ElMessage.info(`已选择网络: ${network.ssid}`);
};
</script>

<style scoped>
.status-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0.75rem;
  background-color: var(--el-fill-color-light);
  border-radius: 8px;
}
</style>
