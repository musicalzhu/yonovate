/**
 * 统一验证层入口
 *
 * 提供所有验证功能的统一导出
 * 包括状态机验证、设备验证、数据转换等
 */

// ========== 状态机验证 ==========
export { validateConfig, validateTransition, validateTrigger } from './validators/stateMachine';

// ========== 数据转换 ==========
export { toBackend, toFrontend, deepConvertKeys } from './dataConverter';

// ========== 类型导出 ==========
export type { ValidationResult } from '@/types';
export type { AnyObject } from '@/types';
