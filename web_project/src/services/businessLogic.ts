/**
 * Vue 3.5+ 可复用业务逻辑工具（纯函数）
 *
 * 本文件仅包含纯函数，不包含任何模块级可变状态。
 * 所有状态管理应通过 Pinia Store 或其他状态管理方案处理。
 */
import type { StateMachineConfig, Transition, Trigger } from '@/types';
import { LogicType, TriggerType } from '@/types';
import type { GPIOStatus } from '@/types';
import { validateTrigger as validateTriggerFn } from '@/services/validation';

/**
 * 基于映射表读取可读名称，减少各类名称转换函数里重复的 fallback 逻辑。
 */
function getReadableName<T extends PropertyKey>(
  mapping: Record<T, string>,
  value: T,
  fallback = '未知'
): string {
  return mapping[value] ?? fallback;
}

/**
 * 获取状态机中所有可用的状态名称
 * @param config - 状态机配置对象
 * @returns 排序后的状态名称数组
 * @example
 * ```ts
 * const states = getAvailableStates(config)
 * // returns: ['IDLE', 'MOVING_UP', 'STOPPED']
 * ```
 */
export function getAvailableStates(config: StateMachineConfig): string[] {
  const states = new Set<string>();

  // 添加初始状态
  states.add(config.initialState);

  // 从转换中收集所有状态
  config.transitions.forEach((transition: Transition) => {
    states.add(transition.currentState);
    states.add(transition.nextState);
  });

  return Array.from(states).sort();
}

/**
 * 评估给定的转换列表，返回所有满足触发条件的下一状态
 * @param transitions - 转换数组
 * @param gpioStatus - GPIO 状态数组
 * @returns 满足触发条件的下一状态名称数组
 * @example
 * ```ts
 * const activeStates = evaluateTriggers(transitions, gpioStatus)
 * // returns: ['MOVING_UP', 'STOPPED']
 * ```
 */
export function evaluateTriggers(
  transitions: readonly Transition[],
  gpioStatus: GPIOStatus[]
): string[] {
  const activeTransitions: string[] = [];

  transitions.forEach((transition) => {
    if (evaluateTransitionTrigger(transition, gpioStatus)) {
      activeTransitions.push(transition.nextState);
    }
  });

  return activeTransitions;
}

/**
 * 评估单个转换的触发条件是否满足
 * @param transition - 转换对象
 * @param gpioStatus - GPIO 状态数组
 * @returns 触发条件是否满足
 * @example
 * ```ts
 * const triggered = evaluateTransitionTrigger(transition, gpioStatus)
 * // returns: true
 * ```
 */
export function evaluateTransitionTrigger(
  transition: Transition,
  gpioStatus: GPIOStatus[]
): boolean {
  if (!transition.triggers?.length) {
    return false;
  }

  const triggerResults = transition.triggers.map((trigger: Trigger) =>
    evaluateSingleTrigger(trigger, gpioStatus)
  );

  return transition.triggerLogic === LogicType.AND
    ? triggerResults.every(Boolean)
    : transition.triggerLogic === LogicType.OR
      ? triggerResults.some(Boolean)
      : false;
}

/**
 * 评估单个触发器条件是否满足
 * @param trigger - 触发器对象
 * @param gpioStatus - GPIO 状态数组
 * @returns 触发器条件是否满足
 * @example
 * ```ts
 * const triggered = evaluateSingleTrigger(trigger, gpioStatus)
 * // returns: true
 * ```
 */
export function evaluateSingleTrigger(trigger: Trigger, gpioStatus: GPIOStatus[]): boolean {
  if (!/^P\d+$/.test(trigger.triggerSource)) {
    return false;
  }

  const pin = Number.parseInt(trigger.triggerSource.slice(1), 10);
  const gpio = gpioStatus.find((g) => g.pin === pin);
  if (!gpio) {
    return false;
  }

  const gpioValue = gpio.value;
  const { triggerValue, operator } = trigger;

  switch (operator) {
    case 0:
      return gpioValue === triggerValue;
    case 1:
      return gpioValue < triggerValue;
    case 2:
      return gpioValue > triggerValue;
    case 3:
      return gpioValue <= triggerValue;
    case 4:
      return gpioValue >= triggerValue;
    case 5:
      return gpioValue !== triggerValue;
    default:
      return false;
  }
}

/**
 * 验证触发器是否有效
 * @param trigger - 触发器对象
 * @returns 触发器是否有效
 * @example
 * ```ts
 * const valid = validateTrigger(trigger)
 * // returns: true
 * ```
 */
export function validateTrigger(trigger: Trigger): boolean {
  return validateTriggerFn(trigger).valid;
}

/**
 * 获取触发器类型的可读名称
 * @param type - 触发器类型枚举值
 * @returns 可读的触发器类型名称
 * @example
 * ```ts
 * const name = getTriggerTypeName(TriggerType.LEVEL)
 * // returns: '电平'
 * ```
 */
export function getTriggerTypeName(type: TriggerType): string {
  const names: Record<TriggerType, string> = {
    [TriggerType.NONE]: '无',
    [TriggerType.EDGE]: '边沿',
    [TriggerType.LEVEL]: '电平',
    [TriggerType.TIMER]: '定时器',
    [TriggerType.MANUAL]: '手动',
  };
  return getReadableName(names, type, '未知');
}

/**
 * 获取逻辑类型的可读名称
 * @param type - 逻辑类型枚举值
 * @returns 可读的逻辑类型名称
 * @example
 * ```ts
 * const name = getLogicTypeName(LogicType.AND)
 * // returns: 'AND'
 * ```
 */
export function getLogicTypeName(type: LogicType): string {
  const names: Record<LogicType, string> = {
    [LogicType.AND]: 'AND',
    [LogicType.OR]: 'OR',
  };
  return getReadableName(names, type, 'AND');
}

/**
 * 获取运算符的可读名称
 * @param operator - 运算符枚举值
 * @returns 可读的运算符名称
 * @example
 * ```ts
 * const name = getOperatorName(0)
 * // returns: '=='
 * ```
 */
export function getOperatorName(operator: number): string {
  const names: Record<number, string> = {
    0: '==',
    1: '<',
    2: '>',
    3: '<=',
    4: '>=',
    5: '!=',
  };
  return getReadableName(names, operator, '==');
}

/**
 * 状态机逻辑工厂函数
 * 创建一个包含状态机业务逻辑方法的对象
 * @param config - 状态机配置
 * @returns 状态机逻辑对象
 * @example
 * ```ts
 * const logic = useStateMachineLogic(config)
 * const states = logic.getAvailableStates()
 * ```
 */
export function useStateMachineLogic(config: StateMachineConfig) {
  return {
    getAvailableStates: () => getAvailableStates(config),
    evaluateTriggers: (gpioStatus: GPIOStatus[]) =>
      evaluateTriggers(config.transitions, gpioStatus),
  };
}
