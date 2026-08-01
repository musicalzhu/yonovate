/**
 * 状态机配置工具函数
 *
 * 【功能说明】
 * 提供状态机配置的导入、导出、克隆和脏检查功能。
 * 使用通用 exportHelper 工具进行导入导出。
 *
 * 【核心概念】
 * 1. 导出配置：将配置对象序列化为JSON字符串（带元数据）
 * 2. 导入配置：将JSON字符串解析为配置对象，并验证有效性
 * 3. 克隆配置：创建配置的深拷贝
 * 4. 脏检查：检测当前配置是否与原始配置有差异
 */
import type { StateMachineConfig } from '@/types';
import { validateConfig } from '@/services/validation';
import { exportToJson, importFromJson, cloneData } from '@/utils/exportHelper';

/**
 * 导出状态机配置为JSON字符串
 * @param config - 状态机配置对象
 * @returns 格式化的JSON字符串（包含元数据）
 * @example
 * ```ts
 * const json = exportConfig(config)
 * // returns: '{"data":{"initialState":"IDLE",...},"metadata":{"version":"1.0.0",...}}'
 * ```
 */
export function exportConfig(config: StateMachineConfig): string {
  return exportToJson(config, 'state-machine-config');
}

/**
 * 从JSON字符串导入状态机配置
 * @param json - JSON字符串
 * @returns 导入结果，包含成功标志、配置对象和错误信息
 * @example
 * ```ts
 * const result = importConfig(jsonString)
 * if (result.success) {
 *   console.log(result.data)
 * } else {
 *   console.error(result.error)
 * }
 * ```
 */
export function importConfig(json: string): {
  success: boolean;
  config?: StateMachineConfig;
  error?: string;
} {
  const result = importFromJson<StateMachineConfig>(json, {
    validate: true,
    validator: (data: unknown): data is StateMachineConfig => {
      const validation = validateConfig(data as StateMachineConfig);
      return validation.valid;
    },
  });

  return {
    success: result.success,
    config: result.data ?? undefined,
    error: result.error,
  };
}

/**
 * 深克隆状态机配置对象
 * @param config - 要克隆的配置对象
 * @returns 配置对象的深拷贝
 * @example
 * ```ts
 * const configCopy = cloneConfig(config)
 * // configCopy 与 config 互不影响
 * ```
 */
export function cloneConfig(config: StateMachineConfig): StateMachineConfig {
  return cloneData(config);
}

/**
 * 检测配置是否被修改（脏检查）
 * @param currentConfig - 当前配置对象
 * @param originalConfig - 原始配置对象（可为null）
 * @returns 是否有修改
 * @example
 * ```ts
 * const isDirty = isConfigDirty(currentConfig, originalConfig)
 * // returns: true 或 false
 * ```
 */
export function isConfigDirty(
  currentConfig: StateMachineConfig,
  originalConfig: StateMachineConfig | null
): boolean {
  if (!originalConfig) return false;
  return JSON.stringify(currentConfig) !== JSON.stringify(originalConfig);
}
