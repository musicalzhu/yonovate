import { ref, computed } from 'vue';
import { useQueryClient } from '@tanstack/vue-query';
import {
  useGetAllGPIOStatusQuery,
  useSetGPIOValueMutation,
  useUpdateGPIOConfigMutation,
  QUERY_KEYS,
} from '@/generated/api';
import type { GPIOStatus, GPIOSetRequest, GPIOConfigRequest } from '@/generated/api';
import { useDebounce } from './useDebounce';

export function useGPIOStatus() {
  const queryClient = useQueryClient();

  const {
    data: gpioData,
    isLoading: isLoadingGPIO,
    isFetching: isFetchingGPIO,
    error: errorGPIO,
    refetch: refetchGPIO,
  } = useGetAllGPIOStatusQuery({
    staleTime: 1000 * 5,
  });

  const gpioStatus = computed(() => gpioData.value ?? []);

  const { mutate: setPinValue, isPending: isSettingPin } = useSetGPIOValueMutation({
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.gpioStatus });
    },
  });

  const { mutate: updatePinConfig, isPending: isUpdatingConfig } = useUpdateGPIOConfigMutation({
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.gpioStatus });
    },
  });

  const setPinValueDebounced = useDebounce(setPinValue, { delay: 300 });

  return {
    gpioStatus,
    isLoadingGPIO,
    isFetchingGPIO,
    errorGPIO,
    refetchGPIO,
    setPinValue: setPinValueDebounced.debouncedFn,
    updatePinConfig,
    isSettingPin,
    isUpdatingConfig,
  };
}
