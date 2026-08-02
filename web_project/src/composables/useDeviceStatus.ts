import { computed, watch } from 'vue';
import type { ComputedRef } from 'vue';
import {
  useGetDeviceInfoQuery,
  useGetDeviceStatusQuery,
  useGetDeviceMetricsQuery,
  useRebootDeviceMutation,
  useFactoryResetDeviceMutation,
  QUERY_KEYS,
} from '@/generated/api';
import { useQueryClient } from '@tanstack/vue-query';

export function useDeviceStatus(deviceIP: ComputedRef<string>) {
  const queryClient = useQueryClient();

  watch(
    deviceIP,
    (newIP) => {
      localStorage.setItem('esp32-ip', newIP);
    },
    { immediate: true }
  );

  const {
    data: infoData,
    isLoading: isLoadingDeviceInfo,
    isFetching: isFetchingDeviceInfo,
    error: errorDevice,
    refetch: refetchDeviceInfo,
  } = useGetDeviceInfoQuery({
    staleTime: 1000 * 60 * 5,
    queryKey: [...QUERY_KEYS.deviceInfo, deviceIP.value],
  });

  const {
    data: statusData,
    isLoading: isLoadingDeviceStatus,
    isFetching: isFetchingDeviceStatus,
    error: errorStatus,
    refetch: refetchDeviceStatus,
  } = useGetDeviceStatusQuery({
    staleTime: 1000 * 5,
    queryKey: [...QUERY_KEYS.deviceStatus, deviceIP.value],
  });

  const {
    data: metricsData,
    isLoading: isLoadingDeviceMetrics,
    isFetching: isFetchingDeviceMetrics,
    error: errorMetrics,
    refetch: refetchDeviceMetrics,
  } = useGetDeviceMetricsQuery({
    staleTime: 1000 * 30,
    queryKey: [...QUERY_KEYS.deviceMetrics, deviceIP.value],
  });

  const deviceInfoData = computed(() => infoData.value ?? null);
  const deviceStatusData = computed(() => statusData.value ?? null);
  const deviceMetricsData = computed(() => metricsData.value ?? null);

  const isConnected = computed(() => {
    return deviceStatusData.value?.status === 'online' || !!deviceInfoData.value;
  });

  const {
    mutate: reboot,
    isPending: isRebooting,
    error: rebootError,
  } = useRebootDeviceMutation({
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.deviceStatus });
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.deviceInfo });
    },
  });

  const {
    mutate: factoryReset,
    isPending: isFactoryResetting,
    error: factoryResetError,
  } = useFactoryResetDeviceMutation({
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.deviceInfo });
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.deviceStatus });
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.gpioStatus });
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.wifiConfig });
    },
  });

  const refetchAllDeviceData = () => {
    refetchDeviceInfo();
    refetchDeviceStatus();
    refetchDeviceMetrics();
  };

  return {
    deviceInfoData,
    deviceStatusData,
    deviceMetricsData,
    isConnected,

    isLoadingDeviceInfo,
    isLoadingDeviceStatus,
    isLoadingDeviceMetrics,
    isFetchingDeviceInfo,
    isFetchingDeviceStatus,
    isFetchingDeviceMetrics,
    isRebooting,
    isFactoryResetting,

    errorDevice,
    errorStatus,
    errorMetrics,
    rebootError,
    factoryResetError,

    refetchDeviceInfo,
    refetchDeviceStatus,
    refetchDeviceMetrics,
    refetchAllDeviceData,
    reboot,
    factoryReset,
  };
}
