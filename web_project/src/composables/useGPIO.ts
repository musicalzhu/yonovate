import { computed } from 'vue';
import { useQueryClient } from '@tanstack/vue-query';
import {
  useGetAllGPIOStatusQuery,
  useSetGPIOValueMutation,
  useUpdateGPIOConfigMutation,
  QUERY_KEYS,
} from '@/generated/api';

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

  const { mutate: setPinValueMutation, isPending: isSettingPin } = useSetGPIOValueMutation({
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.gpioStatus });
    },
  });

  const { mutate: updatePinConfig, isPending: isUpdatingConfig } = useUpdateGPIOConfigMutation({
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.gpioStatus });
    },
  });

  const setPinValue = (pin: number, value: number) => {
    setPinValueMutation({ pin, value });
  };

  return {
    gpioStatus,
    isLoadingGPIO,
    isFetchingGPIO,
    errorGPIO,
    refetchGPIO,
    setPinValue,
    updatePinConfig,
    isSettingPin,
    isUpdatingConfig,
  };
}
