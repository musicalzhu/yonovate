import type {
  Trigger as ApiTrigger,
  Transition as ApiTransition,
  StateEffect as ApiStateEffect,
  StateMachineConfig as ApiStateMachineConfig,
  StateMachineStatus as ApiStateMachineStatus,
} from '../generated/api';

export enum TriggerType {
  NONE = 0,
  EDGE = 1,
  LEVEL = 2,
  TIMER = 3,
  MANUAL = 4,
}

export enum LogicType {
  AND = 0,
  OR = 1,
}

/**
 * 业务层触发器类型，补充触发类型枚举信息。
 */
export type Trigger = ApiTrigger & {
  triggerType: TriggerType;
};

/**
 * 业务层状态转换类型，补充触发器数组与逻辑类型字段。
 */
export type Transition = ApiTransition & {
  triggers: Trigger[];
  triggerLogic: LogicType;
};

export type StateEffect = ApiStateEffect;

/**
 * 业务层状态机配置类型，统一使用转换和效果数组的业务视图。
 */
export type StateMachineConfig = ApiStateMachineConfig & {
  transitions: Transition[];
  stateEffects: StateEffect[];
};

export type StateMachineStatus = ApiStateMachineStatus;

export interface ValidationResult {
  valid: boolean;
  errors: string[];
}

/**
 * 检查值是否为有效的触发器类型。
 */
export function isValidTriggerType(value: unknown): value is TriggerType {
  return typeof value === 'number' && Object.values(TriggerType).includes(value as TriggerType);
}

/**
 * 检查值是否为有效的触发源字符串。
 */
export function isValidTriggerSource(value: unknown): value is string {
  if (typeof value !== 'string') return false;
  return /^(P(?:1[0-1]|[0-9])|T[0-2]|MANUAL)$/.test(value);
}

/**
 * 检查值是否为有效的逻辑类型。
 */
export function isValidLogicType(value: unknown): value is LogicType {
  return typeof value === 'number' && Object.values(LogicType).includes(value as LogicType);
}

/**
 * 检查值是否为有效的触发器对象。
 */
export function isValidTrigger(value: unknown): value is Trigger {
  if (!value || typeof value !== 'object') return false;

  const trigger = value as Trigger;
  return (
    isValidTriggerType(trigger.triggerType) &&
    isValidTriggerSource(trigger.triggerSource) &&
    typeof trigger.operator === 'number' &&
    trigger.operator >= 0 &&
    trigger.operator <= 5 &&
    typeof trigger.triggerValue === 'number'
  );
}

/**
 * 检查值是否为有效的状态转换对象。
 */
export function isValidTransition(value: unknown): value is Transition {
  if (!value || typeof value !== 'object') return false;

  const transition = value as Transition;
  return (
    typeof transition.currentState === 'string' &&
    typeof transition.nextState === 'string' &&
    Array.isArray(transition.triggers) &&
    transition.triggers.every(isValidTrigger) &&
    typeof transition.triggerCount === 'number' &&
    transition.triggerCount >= 1 &&
    transition.triggerCount <= 3 &&
    isValidLogicType(transition.triggerLogic)
  );
}

/**
 * 检查值是否为有效的状态机配置对象。
 */
export function isValidStateMachineConfig(value: unknown): value is StateMachineConfig {
  if (!value || typeof value !== 'object') return false;

  const config = value as StateMachineConfig;
  return (
    typeof config.initialState === 'string' &&
    Array.isArray(config.transitions) &&
    config.transitions.every(isValidTransition) &&
    Array.isArray(config.stateEffects)
  );
}

/**
 * 创建默认触发器对象。
 */
/**
 * 创建默认触发器对象，便于编辑器和表单中快速初始化。
 */
export function createDefaultTrigger(): Trigger {
  return {
    triggerType: TriggerType.NONE,
    triggerSource: '',
    operator: 0,
    triggerValue: 0,
  };
}

/**
 * 创建默认状态转换对象。
 */
/**
 * 创建默认状态转换对象，包含一个初始触发器。
 */
export function createDefaultTransition(): Transition {
  return {
    currentState: '',
    nextState: '',
    triggers: [createDefaultTrigger()],
    triggerCount: 1,
    triggerLogic: LogicType.AND,
  };
}

/**
 * 创建默认状态机配置对象。
 */
/**
 * 创建默认状态机配置，作为新建配置时的基础模板。
 */
export function createDefaultStateMachineConfig(): StateMachineConfig {
  return {
    initialState: 'IDLE',
    transitions: [],
    stateEffects: [],
    groupModes: [0, 0, 0, 0],
    inputModuleTypes: [0, 0, 0, 0],
    edgeTypes: [0, 0, 0, 0],
    outputModuleTypes: [0, 0, 0, 0],
  };
}

/**
 * 克隆单个触发器对象。
 */
/**
 * 克隆单个触发器对象，避免共享引用导致意外修改。
 */
export function cloneTrigger(trigger: Trigger): Trigger {
  return { ...trigger };
}

/**
 * 克隆单个状态转换对象。
 */
/**
 * 克隆单个状态转换对象，并递归复制其中的触发器列表。
 */
export function cloneTransition(transition: Transition): Transition {
  return {
    ...transition,
    triggers: transition.triggers.map(cloneTrigger),
  };
}

/**
 * 克隆完整状态机配置对象。
 */
/**
 * 克隆完整状态机配置对象，确保编辑过程不会污染原始数据。
 */
export function cloneStateMachineConfig(config: StateMachineConfig): StateMachineConfig {
  return {
    ...config,
    transitions: config.transitions.map(cloneTransition),
    stateEffects: [...config.stateEffects],
  };
}
