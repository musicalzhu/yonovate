import { ref, computed } from 'vue';
import { useQueryClient } from '@tanstack/vue-query';
import {
  useGetStateMachineConfigQuery,
  useGetStateMachineStatusQuery,
  useSaveStateMachineConfigMutation,
  useResetStateMachineMutation,
  QUERY_KEYS,
} from '@/generated/api';
import type { StateMachineConfig, Transition, Trigger, StateEffect } from '@/generated/api';
import { useDebounce } from './useDebounce';
import { createDefaultStateMachineConfig } from '@/types/domain/stateMachine';

export function useStateMachineConfig() {
  const queryClient = useQueryClient();

  const isDirty = ref(false);
  const lastSaved = ref<Date | null>(null);
  const pendingConfig = ref<StateMachineConfig | null>(null);

  const { debouncedFn: debouncedSave, cancel: cancelSave } = useDebounce(
    (config: StateMachineConfig) => {
      saveConfig(config);
    },
    { delay: 1000 }
  );

  const {
    data: configData,
    isLoading: isLoadingConfig,
    error: errorConfig,
    refetch: refetchConfig,
  } = useGetStateMachineConfigQuery({
    staleTime: 1000 * 60 * 5,
    placeholderData: createDefaultStateMachineConfig(),
  });

  const {
    data: statusData,
    isLoading: isLoadingStatus,
    error: errorStatus,
    refetch: refetchStatus,
  } = useGetStateMachineStatusQuery({
    staleTime: 1000 * 30,
  });

  const config = computed(() => configData.value ?? createDefaultStateMachineConfig());
  const status = computed(() => statusData.value ?? null);

  const transitions = computed(() => config.value.transitions);
  const transitionsCount = computed(() => config.value.transitions.length);
  const stateEffects = computed(() => config.value.stateEffects);
  const stateEffectsCount = computed(() => config.value.stateEffects.length);

  const {
    mutate: saveConfig,
    isPending: isSaving,
    error: saveError,
    reset: resetSave,
  } = useSaveStateMachineConfigMutation({
    onSuccess: () => {
      isDirty.value = false;
      lastSaved.value = new Date();
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.stateMachineConfig });
    },
  });

  const {
    mutate: reboot,
    isPending: isRebooting,
    error: rebootError,
  } = useResetStateMachineMutation({
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: QUERY_KEYS.stateMachineStatus });
    },
  });

  const updateConfig = (newConfig: StateMachineConfig) => {
    isDirty.value = true;
    pendingConfig.value = newConfig;
    debouncedSave(newConfig);
  };

  /**
   * 统一封装配置变更提交逻辑，避免不同更新入口重复编排配置对象。
   */
  const commitConfigUpdate = (newConfig: StateMachineConfig) => {
    updateConfig(newConfig);
  };

  const partialUpdateConfig = (partial: Partial<StateMachineConfig>) => {
    const newConfig = { ...config.value, ...partial };
    commitConfigUpdate(newConfig);
  };

  const saveNow = (newConfig?: StateMachineConfig) => {
    isDirty.value = false;
    cancelSave();
    pendingConfig.value = null;
    saveConfig(newConfig ?? config.value);
  };

  const addTransition = (transition: Transition) => {
    partialUpdateConfig({ transitions: [...config.value.transitions, transition] });
  };

  const removeTransition = (index: number) => {
    partialUpdateConfig({ transitions: config.value.transitions.filter((_: Transition, i: number) => i !== index) });
  };

  const updateTransition = (index: number, transition: Transition) => {
    const newTransitions = [...config.value.transitions];
    newTransitions[index] = transition;
    partialUpdateConfig({ transitions: newTransitions });
  };

  const addTrigger = (transitionIndex: number, trigger: Trigger) => {
    const transition = config.value.transitions[transitionIndex];
    if (transition && transition.triggers.length < 3) {
      const newTriggers = [...transition.triggers, trigger];
      const updatedTransition = {
        ...transition,
        triggers: newTriggers,
        triggerCount: newTriggers.length,
      };
      updateTransition(transitionIndex, updatedTransition);
    }
  };

  const removeTrigger = (transitionIndex: number, triggerIndex: number) => {
    const transition = config.value.transitions[transitionIndex];
    if (transition && transition.triggers.length > 1) {
      const newTriggers = transition.triggers.filter((_: Trigger, i: number) => i !== triggerIndex);
      const updatedTransition = {
        ...transition,
        triggers: newTriggers,
        triggerCount: newTriggers.length,
      };
      updateTransition(transitionIndex, updatedTransition);
    }
  };

  const updateTrigger = (transitionIndex: number, triggerIndex: number, trigger: Trigger) => {
    const transition = config.value.transitions[transitionIndex];
    if (transition) {
      const newTriggers = [...transition.triggers];
      newTriggers[triggerIndex] = trigger;
      const updatedTransition = {
        ...transition,
        triggers: newTriggers,
      };
      updateTransition(transitionIndex, updatedTransition);
    }
  };

  const addStateEffect = (effect: StateEffect) => {
    partialUpdateConfig({ stateEffects: [...config.value.stateEffects, effect] });
  };

  const removeStateEffect = (index: number) => {
    partialUpdateConfig({ stateEffects: config.value.stateEffects.filter((_: StateEffect, i: number) => i !== index) });
  };

  const updateStateEffect = (index: number, effect: StateEffect) => {
    const newEffects = [...config.value.stateEffects];
    newEffects[index] = effect;
    partialUpdateConfig({ stateEffects: newEffects });
  };

  return {
    config,
    status,
    isDirty,
    lastSaved,

    transitions,
    transitionsCount,
    stateEffects,
    stateEffectsCount,

    isLoading: isLoadingConfig,
    isLoadingConfig,
    isLoadingStatus,
    isSaving,
    isRebooting,

    error: errorConfig,
    errorConfig,
    errorStatus,
    saveError,
    rebootError,

    refetch: refetchConfig,
    updateConfig,
    partialUpdateConfig,
    saveNow,
    refetchConfig,
    refetchStatus,
    reboot,
    resetSave,

    addTransition,
    removeTransition,
    updateTransition,

    addTrigger,
    removeTrigger,
    updateTrigger,

    addStateEffect,
    removeStateEffect,
    updateStateEffect,
  };
}
