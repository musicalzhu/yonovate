// Vue 3.5+ 状态机验证工具（纯函数）
import type { StateMachineConfig, Transition, Trigger, ValidationResult } from '@/types';
import { TriggerType, LogicType } from '@/types';

const VALID_TRIGGER_SOURCES = /^(P[0-9]|T[0-2]|MANUAL)$/;
const MAX_TRIGGER_COUNT = 3;
const MIN_TRIGGER_COUNT = 1;
const MAX_STATE_NAME_LENGTH = 50;
const VALID_STATE_NAME_PATTERN = /^[A-Z_][A-Z0-9_]*$/;

/**
 * 验证状态机配置的完整性和有效性
 * @param config - 状态机配置对象
 * @returns 验证结果，包含 valid 标志和错误信息数组
 * @example
 * ```ts
 * const result = validateConfig(config)
 * // returns: { valid: true, errors: [] }
 * ```
 */
export function validateConfig(config: StateMachineConfig): ValidationResult {
  const errors: string[] = [];

  // 验证初始状态
  if (!config.initialState.trim()) {
    errors.push('初始状态不能为空');
  } else if (!VALID_STATE_NAME_PATTERN.test(config.initialState)) {
    errors.push('初始状态格式不正确，必须是大写字母、数字和下划线，且以字母开头');
  } else if (config.initialState.length > MAX_STATE_NAME_LENGTH) {
    errors.push(`初始状态名称过长，最大长度为${MAX_STATE_NAME_LENGTH}个字符`);
  }

  // 验证转换
  if (!config.transitions || config.transitions.length === 0) {
    errors.push('至少需要一个转换');
  }

  const stateNames = new Set<string>();
  stateNames.add(config.initialState);
  const transitions = config.transitions ? [...config.transitions] : [];

  if (config.transitions && config.transitions.length > 0) {
    transitions.forEach((transition, index) => {
      const transitionResult = validateTransition(transition);
      if (!transitionResult.valid) {
        errors.push(...transitionResult.errors.map((err) => `转换${index + 1}: ${err}`));
      }

      // 收集状态名称
      if (transition.currentState) stateNames.add(transition.currentState);
      if (transition.nextState) stateNames.add(transition.nextState);
    });

    // 验证状态引用完整性
    transitions.forEach((transition, index) => {
      if (!stateNames.has(transition.currentState)) {
        errors.push(`转换${index + 1}: 当前状态"${transition.currentState}"未在任何转换中定义`);
      }
      if (!stateNames.has(transition.nextState)) {
        errors.push(`转换${index + 1}: 下一状态"${transition.nextState}"未在任何转换中定义`);
      }
    });

    // 检查循环引用
    const hasCycle = hasStateCycle(transitions);
    if (hasCycle) {
      errors.push('状态机存在循环引用，这会导致无限循环');
    }
  }

  // 验证状态效果
  if (config.stateEffects) {
    config.stateEffects.forEach((effect, index) => {
      if (!effect.stateName.trim()) {
        errors.push(`状态效果${index + 1}: 状态名称不能为空`);
      } else if (!stateNames.has(effect.stateName)) {
        errors.push(`状态效果${index + 1}: 状态"${effect.stateName}"未在转换中定义`);
      }
    });
  }

  return {
    valid: errors.length === 0,
    errors,
  };
}

/**
 * 验证单个转换的有效性
 * @param transition - 转换对象
 * @returns 验证结果，包含 valid 标志和错误信息数组
 * @example
 * ```ts
 * const result = validateTransition(transition)
 * // returns: { valid: true, errors: [] }
 * ```
 */
export function validateTransition(transition: Transition): ValidationResult {
  const errors: string[] = [];

  // 验证当前状态
  if (!transition.currentState.trim()) {
    errors.push('当前状态不能为空');
  } else if (!VALID_STATE_NAME_PATTERN.test(transition.currentState)) {
    errors.push('当前状态格式不正确，必须是大写字母、数字和下划线，且以字母开头');
  }

  // 验证下一状态
  if (!transition.nextState.trim()) {
    errors.push('下一状态不能为空');
  } else if (!VALID_STATE_NAME_PATTERN.test(transition.nextState)) {
    errors.push('下一状态格式不正确，必须是大写字母、数字和下划线，且以字母开头');
  }

  // 验证触发器数量
  if (transition.triggerCount < MIN_TRIGGER_COUNT || transition.triggerCount > MAX_TRIGGER_COUNT) {
    errors.push(`触发器数量必须在${MIN_TRIGGER_COUNT}到${MAX_TRIGGER_COUNT}之间`);
  }

  // 验证触发器逻辑
  if (!validateLogicType(transition.triggerLogic)) {
    errors.push('触发器逻辑必须是AND或OR');
  }

  // 验证触发器数组
  if (!transition.triggers || transition.triggers.length === 0) {
    errors.push('至少需要一个触发器');
  } else {
    // 检查触发器数量是否匹配
    if (transition.triggers.length !== transition.triggerCount) {
      errors.push(
        `触发器数组长度(${transition.triggers.length})与触发器数量(${transition.triggerCount})不匹配`
      );
    }

    // 验证每个触发器
    transition.triggers.forEach((trigger, index) => {
      const triggerResult = validateTrigger(trigger);
      if (!triggerResult.valid) {
        errors.push(...triggerResult.errors.map((err) => `触发器${index + 1}: ${err}`));
      }
    });
  }

  return {
    valid: errors.length === 0,
    errors,
  };
}

/**
 * 验证单个触发器的有效性
 * @param trigger - 触发器对象
 * @returns 验证结果，包含 valid 标志和错误信息数组
 * @example
 * ```ts
 * const result = validateTrigger(trigger)
 * // returns: { valid: true, errors: [] }
 * ```
 */
export function validateTrigger(trigger: Trigger): ValidationResult {
  const errors: string[] = [];

  // 验证触发器类型
  if (!validateTriggerType(trigger.triggerType)) {
    errors.push('触发器类型无效');
  }

  // 验证触发源
  if (!validateTriggerSource(trigger.triggerSource)) {
    errors.push('触发源格式不正确，必须是P0-P11、T0-T2或MANUAL');
  }

  // 验证运算符
  if (trigger.operator < 0 || trigger.operator > 5) {
    errors.push('运算符必须是0-5之间的整数（0:==, 1:<, 2:>, 3:<=, 4:>=, 5:!=）');
  }

  // 验证触发值
  if (typeof trigger.triggerValue !== 'number') {
    errors.push('触发值必须是数字');
  }

  return {
    valid: errors.length === 0,
    errors,
  };
}

/**
 * 验证触发器类型是否有效
 * @param type - 触发器类型枚举值
 * @returns 类型是否有效
 * @private
 */
function validateTriggerType(type: TriggerType): boolean {
  return Object.values(TriggerType).includes(type);
}

/**
 * 验证触发源格式是否有效
 * @param source - 触发源字符串
 * @returns 触发源是否有效
 * @private
 */
function validateTriggerSource(source: string): boolean {
  return VALID_TRIGGER_SOURCES.test(source);
}

/**
 * 验证逻辑类型是否有效
 * @param type - 逻辑类型枚举值
 * @returns 类型是否有效
 * @private
 */
function validateLogicType(type: LogicType): boolean {
  return Object.values(LogicType).includes(type);
}

/**
 * 检查状态机转换中是否存在循环引用
 * 使用深度优先搜索（DFS）算法检测循环
 * @param transitions - 转换数组
 * @returns 是否存在循环引用
 * @private
 */
function hasStateCycle(transitions: Transition[]): boolean {
  const visited = new Set<string>();
  const recursionStack = new Set<string>();

  const hasCycleUtil = (state: string): boolean => {
    if (recursionStack.has(state)) {
      return true;
    }
    if (visited.has(state)) {
      return false;
    }

    visited.add(state);
    recursionStack.add(state);

    for (const transition of transitions) {
      if (transition.currentState === state) {
        if (hasCycleUtil(transition.nextState)) {
          return true;
        }
      }
    }

    recursionStack.delete(state);
    return false;
  };

  // 检查每个状态是否有循环
  for (const transition of transitions) {
    if (hasCycleUtil(transition.currentState)) {
      return true;
    }
  }

  return false;
}
