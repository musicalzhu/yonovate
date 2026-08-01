import type { StateMachineConfig, StateMachineStatus } from '@/generated/api';

export const mockStateMachineConfig: StateMachineConfig = {
  initialState: 'Idle',
  transitions: [
    {
      currentState: 'Idle',
      nextState: 'Active',
      triggers: [{ triggerType: 1, triggerSource: 'P0', operator: 0, triggerValue: 1 }],
      triggerCount: 1,
      triggerLogic: 0,
    },
    {
      currentState: 'Active',
      nextState: 'Idle',
      triggers: [{ triggerType: 1, triggerSource: 'P1', operator: 0, triggerValue: 1 }],
      triggerCount: 1,
      triggerLogic: 0,
    },
  ],
  stateEffects: [{ stateName: 'Active', effectType: 1, target: 'GPIO2', value: 1, duration: 5000 }],
  groupModes: [0, 1, 0],
  inputModuleTypes: [1, 1, 0],
  edgeTypes: [0, 0, 0],
  outputModuleTypes: [1, 1, 1],
};

export const mockStateMachineStatus: StateMachineStatus = {
  currentState: 'Idle',
  lastTriggerTime: Date.now() - 10000,
  triggerCount: 42,
  errorCount: 0,
  uptime: 3600,
};
