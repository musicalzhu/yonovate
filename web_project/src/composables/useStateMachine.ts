import { useStateMachineConfig } from './useStateMachineConfig';

export function useStateMachine() {
  const configComposable = useStateMachineConfig();

  return {
    config: configComposable.config,
    status: configComposable.status,
    isDirty: configComposable.isDirty,
    lastSaved: configComposable.lastSaved,

    transitions: configComposable.transitions,
    transitionsCount: configComposable.transitionsCount,
    stateEffects: configComposable.stateEffects,
    stateEffectsCount: configComposable.stateEffectsCount,

    isLoadingConfig: configComposable.isLoadingConfig,
    isLoadingStatus: configComposable.isLoadingStatus,
    isSaving: configComposable.isSaving,
    isRebooting: configComposable.isRebooting,

    errorConfig: configComposable.errorConfig,
    errorStatus: configComposable.errorStatus,
    saveError: configComposable.saveError,
    rebootError: configComposable.rebootError,

    updateConfig: configComposable.updateConfig,
    partialUpdateConfig: configComposable.partialUpdateConfig,
    saveNow: configComposable.saveNow,
    refetchConfig: configComposable.refetchConfig,
    refetchStatus: configComposable.refetchStatus,
    reboot: configComposable.reboot,
    resetSave: configComposable.resetSave,

    addTransition: configComposable.addTransition,
    removeTransition: configComposable.removeTransition,
    updateTransition: configComposable.updateTransition,

    addTrigger: configComposable.addTrigger,
    removeTrigger: configComposable.removeTrigger,
    updateTrigger: configComposable.updateTrigger,

    addStateEffect: configComposable.addStateEffect,
    removeStateEffect: configComposable.removeStateEffect,
    updateStateEffect: configComposable.updateStateEffect,
  };
}
