/**
 * 设备连接组合式函数 (简化版)
 * 使用 TanStack Query 和 Orval 生成的 hooks
 */
import { ref, onMounted, onUnmounted, computed } from 'vue';
import type { ComputedRef } from 'vue';
import { useDeviceStatus } from './useDeviceStatus';
import { useGPIOStatus } from './useGPIO';
import { useWiFiConfig } from './useWiFiConfig';

export function useDeviceConnection(deviceIP: ComputedRef<string>) {
  const deviceStatusComposable = useDeviceStatus(deviceIP);
  const gpioComposable = useGPIOStatus();
  const wifiConfigComposable = useWiFiConfig();

  // 连接状态
  const isConnected = ref(false);
  const isOnline = ref(navigator.onLine);

  // 计算属性
  const deviceName = computed(
    () => deviceStatusComposable.deviceInfoData.value?.name ?? '未知设备'
  );
  const firmwareVersion = computed(
    () => deviceStatusComposable.deviceInfoData.value?.firmware ?? '未知'
  );
  const ipAddress = computed(
    () => deviceStatusComposable.deviceInfoData.value?.ipAddress ?? '未连接'
  );
  const lastSeen = computed(() => deviceStatusComposable.deviceInfoData.value?.lastSeen ?? '未知');
  const gpioCount = computed(() => gpioComposable.gpioStatus.value?.length ?? 0);
  const activeGpios = computed(
    () => gpioComposable.gpioStatus.value?.filter((g) => g.value === 1).length ?? 0
  );

  const connect = async () => {
    isConnected.value = true;
    await Promise.all([
      deviceStatusComposable.refetchDeviceInfo(),
      deviceStatusComposable.refetchDeviceStatus(),
      gpioComposable.refetchGPIO(),
      wifiConfigComposable.refetchConfig(),
      wifiConfigComposable.refetchWiFiStatus(),
    ]);
  };

  /**
   * 断开当前连接状态。
   */
  const disconnect = () => {
    isConnected.value = false;
  };

  /**
   * 设置指定 GPIO 引脚的输出值。
   */
  const setGPIOOutput = (pin: number, value: number) => {
    gpioComposable.setPinValue(pin, value);
  };

  /**
   * 保存 WiFi 配置。
   */
  const setWiFiConfig = (config: any) => {
    wifiConfigComposable.saveConfig(config);
  };

  /**
   * 重启设备并同时关闭连接态提示。
   */
  const reboot = () => {
    deviceStatusComposable.reboot();
    disconnect();
  };

  // 处理网络状态变化
  const handleOnline = () => {
    isOnline.value = true;
  };

  const handleOffline = () => {
    isOnline.value = false;
  };

  // 生命周期
  onMounted(() => {
    window.addEventListener('online', handleOnline);
    window.addEventListener('offline', handleOffline);
  });

  onUnmounted(() => {
    window.removeEventListener('online', handleOnline);
    window.removeEventListener('offline', handleOffline);
  });

  return {
    deviceInfo: deviceStatusComposable.deviceInfoData,
    deviceStatus: deviceStatusComposable.deviceStatusData,
    gpioStatus: gpioComposable.gpioStatus,
    wifiConfig: wifiConfigComposable.wifiConfigData,
    wifiStatus: wifiConfigComposable.wifiStatusData,
    isConnected,
    isOnline,

    deviceName,
    firmwareVersion,
    ipAddress,
    lastSeen,
    gpioCount,
    activeGpios,

    connect,
    disconnect,
    setGPIOOutput,
    setWiFiConfig,
    reboot,
    refreshDeviceInfo: deviceStatusComposable.refetchDeviceInfo,
    refreshDeviceStatus: deviceStatusComposable.refetchDeviceStatus,
    refreshGPIOStatus: gpioComposable.refetchGPIO,
    refreshWiFiConfig: wifiConfigComposable.refetchConfig,
  };
}
