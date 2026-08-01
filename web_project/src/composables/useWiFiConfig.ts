import { ref, computed } from 'vue';
import { useQueryClient } from '@tanstack/vue-query';
import {
  useGetWiFiConfigQuery,
  useGetWiFiStatusQuery,
  useScanWiFiNetworksQuery,
  useSaveWiFiConfigMutation,
  QUERY_KEYS,
} from '@/generated/api';
import type { WiFiConfig } from '@/generated/api';
import { useDebounce } from './useDebounce';

export function useWiFiConfig() {
  const queryClient = useQueryClient();

  const isDirty = ref(false);
  const pendingConfig = ref<WiFiConfig | null>(null);

  const {
    data: configData,
    isLoading: isLoadingConfig,
    error: errorConfig,
    refetch: refetchConfig,
  } = useGetWiFiConfigQuery({
    staleTime: 1000 * 60 * 5,
    placeholderData: { ssid: '', dhcp: true },
  });

  const {
    data: statusData,
    isLoading: isLoadingStatus,
    error: errorStatus,
    refetch: refetchWiFiStatus,
  } = useGetWiFiStatusQuery({
    staleTime: 1000 * 10,
  });

  const {
    data: networksData,
    isLoading: isScanning,
    error: errorNetworks,
    refetch: refetchNetworks,
  } = useScanWiFiNetworksQuery({
    staleTime: 1000 * 60,
    enabled: false,
  });

  const wifiConfigData = computed(() => configData.value ?? { ssid: '', dhcp: true });
  const wifiStatusData = computed(() => statusData.value ?? null);

  const {
    mutate: saveConfig,
    isPending: isSaving,
    error: saveError,
    reset: resetSave,
  } = useSaveWiFiConfigMutation({
    onSuccess: () => {
      isDirty.value = false;
      pendingConfig.value = null;
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.wifiConfig });
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.wifiStatus });
    },
  });

  const { debouncedFn: debouncedSave, cancel: cancelSave } = useDebounce(
    (newConfig: WiFiConfig) => {
      saveConfig(newConfig);
    },
    { delay: 1500 }
  );

  const updateConfig = (newConfig: WiFiConfig) => {
    isDirty.value = true;
    pendingConfig.value = newConfig;
    debouncedSave(newConfig);
  };

  const saveNow = (newConfig: WiFiConfig) => {
    isDirty.value = false;
    cancelSave();
    pendingConfig.value = null;
    saveConfig(newConfig);
  };

  return {
    wifiConfigData,
    wifiStatusData,
    networksData,
    isDirty,
    pendingConfig,

    isLoadingConfig,
    isLoadingStatus,
    isScanning,
    isSaving,

    errorConfig,
    errorStatus,
    errorNetworks,
    saveError,

    saveConfig: updateConfig,
    saveNow,
    refetchConfig,
    refetchWiFiStatus,
    refetchNetworks,
    resetSave,
  };
}
