// Vue 3.5+ 数据转换工具（纯函数）
import type { StateMachineConfig } from '@/types/domain/stateMachine';
import type { AnyObject } from '@/types/common/common';

/**
 * 字段名映射表，用于在前后端数据结构之间做键名转换。
 */
const FIELD_MAPPINGS: Record<string, string> = {
  // 前端到后端的字段映射
  initialState: 'initial_state',
  stateEffects: 'state_effects',
  triggerCount: 'trigger_count',
  triggerLogic: 'trigger_logic',
  triggerType: 'trigger_type',
  triggerSource: 'trigger_source',
  triggerValue: 'trigger_value',
  currentState: 'current_state',
  nextState: 'next_state',
  groupModes: 'group_modes',
  inputModuleTypes: 'input_module_types',
  edgeTypes: 'edge_types',
  outputModuleTypes: 'output_module_types',
};

/**
 * 将前端状态机配置转换为后端格式（snake_case）
 * @param config - 前端格式的状态机配置
 * @returns 后端格式的配置对象
 * @example
 * ```ts
 * const backendConfig = toBackend({ initialState: 'IDLE', ... })
 * // returns: { initial_state: 'IDLE', ... }
 * ```
 */
export function toBackend(config: StateMachineConfig): AnyObject {
  return deepConvertKeys(config, camelToSnake);
}

/**
 * 将后端状态机配置转换为前端格式（camelCase）
 * @param data - 后端格式的配置对象
 * @returns 前端格式的状态机配置
 * @example
 * ```ts
 * const frontendConfig = toFrontend({ initial_state: 'IDLE', ... })
 * // returns: { initialState: 'IDLE', ... }
 * ```
 */
export function toFrontend(data: AnyObject): StateMachineConfig {
  const converted = deepConvertKeys(data, snakeToCamel);

  // 确保必需字段存在
  return {
    initialState: converted.initialState || 'IDLE',
    transitions: converted.transitions || [],
    stateEffects: converted.stateEffects || [],
    groupModes: converted.groupModes,
    inputModuleTypes: converted.inputModuleTypes,
    edgeTypes: converted.edgeTypes,
    outputModuleTypes: converted.outputModuleTypes,
  };
}

/**
 * 深度转换对象的所有键名
 * @param obj - 要转换的对象
 * @param converter - 键名转换函数
 * @returns 转换后的对象
 * @example
 * ```ts
 * const converted = deepConvertKeys({ firstName: 'John' }, camelToSnake)
 * // returns: { first_name: 'John' }
 * ```
 */
export function deepConvertKeys(obj: AnyObject, converter: (key: string) => string): AnyObject {
  if (obj === null || typeof obj !== 'object') {
    return obj;
  }

  if (Array.isArray(obj)) {
    return obj.map((item) => deepConvertKeys(item, converter));
  }

  const result: AnyObject = {};
  for (const key in obj) {
    if (Object.prototype.hasOwnProperty.call(obj, key)) {
      const newKey = FIELD_MAPPINGS[key] || converter(key);
      result[newKey] = deepConvertKeys(obj[key], converter);
    }
  }

  return result;
}

/**
 * 将 snake_case 字符串转换为 camelCase
 * @param str - snake_case 格式的字符串
 * @returns camelCase 格式的字符串
 * @private
 */
function snakeToCamel(str: string): string {
  return str.replace(/_([a-z])/g, (_, letter) => letter.toUpperCase());
}

/**
 * 将 camelCase 字符串转换为 snake_case
 * @param str - camelCase 格式的字符串
 * @returns snake_case 格式的字符串
 * @private
 */
function camelToSnake(str: string): string {
  return str.replace(/[A-Z]/g, (letter) => `_${letter.toLowerCase()}`);
}
