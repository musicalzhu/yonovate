/**
 * Vue 3.5+ 通用类型定义
 * 提供项目中常用的基础类型、工具类型和类型守卫函数
 */

import type { PaginationParams } from './api';

/**
 * 统一封装常见的基础类型判断逻辑，避免各处重复编写相似的条件表达。
 */
function isObjectLike(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null;
}

// ============ 基础类型 ============
/**
 * 原始类型联合
 * 包含所有 JavaScript 原始类型
 */
export type Primitive = string | number | boolean | null | undefined;

/**
 * 非空原始类型
 * 排除 null 和 undefined
 */
export type NonNullablePrimitive = Exclude<Primitive, null | undefined>;

// ============ 对象类型 ============
/**
 * 任意对象类型
 * 键为字符串，值为任意类型
 */
export type AnyObject = Record<string, any>;

/**
 * 判断值是否为对象字面量，便于类型守卫和通用工具函数复用。
 */
function isObjectLikeValue(value: unknown): value is AnyObject {
  return isObjectLike(value);
}

/**
 * 空对象类型
 * 不包含任何键的对象
 */
export type EmptyObject = Record<string, never>;

/**
 * 判断对象是否为空对象。
 */
export function isEmptyObject(value: unknown): value is EmptyObject {
  return isObjectLikeValue(value) && Object.keys(value).length === 0;
}

// ============ 数组类型 ============
/**
 * 数组元素类型
 * 提取数组元素的类型
 */
export type ArrayElement<T> = T extends (infer U)[] ? U : never;

/**
 * 非空数组类型
 * 至少包含一个元素的数组
 */
export type NonEmptyArray<T> = [T, ...T[]];

// ============ 函数类型 ============
/**
 * 任意函数类型
 * 接受任意参数，返回任意类型
 */
export type AnyFunction = (...args: any[]) => any;

/**
 * 异步函数类型
 * 返回 Promise 的函数
 */
export type AsyncFunction<T extends any[] = any[], R = any> = (...args: T) => Promise<R>;

/**
 * 无返回值函数类型
 */
export type VoidFunction = (...args: any[]) => void;

// ============ ID 类型 ============
/**
 * ID 类型
 * 可以是字符串或数字
 */
export type ID = string | number;

/**
 * UUID 类型
 * 标准的 UUID 字符串
 */
export type UUID = string;

// ============ 时间类型 ============
/**
 * 时间戳类型
 * Unix 时间戳（毫秒）
 */
export type Timestamp = number;

/**
 * ISO 8601 字符串类型
 * 标准的日期时间字符串格式
 */
export type ISOString = string;

/**
 * 日期字符串类型
 */
export type DateString = string;

// ============ 分页类型 ============
/**
 * 分页信息
 * 包含当前页、每页数量、总数等信息
 */
export interface Pagination {
  /** 当前页码 */
  page: number;
  /** 每页数量 */
  limit: number;
  /** 总记录数 */
  total: number;
  /** 总页数 */
  totalPages: number;
}

// ============ 排序类型 ============
/**
 * 排序选项
 */
export interface SortOption {
  /** 排序字段 */
  field: string;
  /** 排序方向 */
  order: 'asc' | 'desc';
}

// ============ 过滤类型 ============
/**
 * 过滤选项
 */
export interface FilterOption {
  /** 过滤字段 */
  field: string;
  /** 比较操作符 */
  operator: 'eq' | 'ne' | 'gt' | 'gte' | 'lt' | 'lte' | 'in' | 'nin' | 'like';
  /** 过滤值 */
  value: any;
}

// ============ 搜索类型 ============
/**
 * 搜索参数
 * 组合查询、过滤、排序和分页
 */
export interface SearchParams {
  /** 查询字符串 */
  query?: string;
  /** 过滤条件 */
  filters?: FilterOption[];
  /** 排序条件 */
  sort?: SortOption[];
  /** 分页参数 */
  pagination?: PaginationParams;
}

// ============ 键值对类型 ============
/**
 * 键值对类型
 */
export type KeyValuePair<T = any> = {
  key: string;
  value: T;
};

// ============ 可选类型 ============
/**
 * 可选类型
 * 使指定键变为可选
 */
export type Optional<T, K extends keyof T> = Omit<T, K> & Partial<Pick<T, K>>;

/**
 * 必需类型
 * 使指定键变为必需
 */
export type RequiredBy<T, K extends keyof T> = T & Required<Pick<T, K>>;

// ============ 深度类型 ============
/**
 * 深度可选类型
 * 递归地将所有嵌套属性变为可选
 */
export type DeepPartial<T> = {
  [P in keyof T]?: T[P] extends object ? DeepPartial<T[P]> : T[P];
};

/**
 * 深度必需类型
 * 递归地将所有嵌套属性变为必需
 */
export type DeepRequired<T> = {
  [P in keyof T]-?: T[P] extends object ? DeepRequired<T[P]> : T[P];
};

/**
 * 深度只读类型
 * 递归地将所有嵌套属性变为只读
 */
export type DeepReadonly<T> = {
  readonly [P in keyof T]: T[P] extends object ? DeepReadonly<T[P]> : T[P];
};

// ============ 条件类型 ============
/**
 * 条件类型
 * 根据条件 C 选择类型 T 或 F
 */
export type If<C extends boolean, T, F> = C extends true ? T : F;

/**
 * 判断是否为 never 类型
 */
export type IsNever<T> = [T] extends [never] ? true : false;

/**
 * 判断是否为 unknown 类型
 */
export type IsUnknown<T> = [T] extends [unknown] ? ([unknown] extends [T] ? true : false) : false;

// ============ 字符串类型 ============
/**
 * 首字母大写每个单词
 */
export type CapitalizeWords<T extends string> = T extends `${infer First} ${infer Rest}`
  ? `${Capitalize<First>} ${CapitalizeWords<Rest>}`
  : Capitalize<T>;

/**
 * 转换为 snake_case
 */
export type SnakeCase<T extends string> = T extends `${infer First}${infer Rest}`
  ? First extends Uppercase<First>
    ? `_${Lowercase<First>}${SnakeCase<Rest>}`
    : `${First}${SnakeCase<Rest>}`
  : T;

/**
 * 转换为 camelCase
 */
export type CamelCase<T extends string> = T extends `${infer First}_${infer Rest}`
  ? `${First}${Capitalize<CamelCase<Rest>>}`
  : T;

// ============ 联合类型工具 ============
/**
 * 联合类型转交叉类型
 */
export type UnionToIntersection<U> = (U extends any ? (k: U) => void : never) extends (
  k: infer I
) => void
  ? I
  : never;

/**
 * 非空联合类型
 */
export type NonEmptyUnion<T> = T extends never ? never : T;

// ============ 元组类型 ============
/**
 * 获取元组头部元素
 */
export type Head<T extends any[]> = T extends [infer H, ...any[]] ? H : never;

/**
 * 获取元组尾部元素
 */
export type Tail<T extends any[]> = T extends [any, ...infer T] ? T : never;

/**
 * 获取元组长度
 */
export type Length<T extends any[]> = T['length'];

// ============ 事件类型 ============
/**
 * 基础事件接口
 */
export interface BaseEvent {
  /** 事件 ID */
  id: string;
  /** 事件类型 */
  type: string;
  /** 事件时间戳 */
  timestamp: number;
  /** 事件数据 */
  data?: any;
}

/**
 * 事件监听器类型
 */
export interface EventListener<T extends BaseEvent = BaseEvent> {
  (event: T): void;
}

// ============ 请求状态类型 ============
/**
 * 请求状态枚举
 * 判别联合确保类型安全
 */
export type RequestStatus = 'idle' | 'loading' | 'success' | 'error';

/**
 * 空闲状态
 */
interface IdleState {
  status: 'idle';
  data: null;
  error: null;
}

/**
 * 加载中状态
 */
interface RequestLoadingState {
  status: 'loading';
  data: null;
  error: null;
}

/**
 * 成功状态
 */
interface SuccessState<T> {
  status: 'success';
  data: T;
  error: null;
}

/**
 * 错误状态
 */
interface ErrorState {
  status: 'error';
  data: null;
  error: string;
}

/**
 * 请求状态 - 判别联合类型
 * 确保每个状态下的 data 和 error 字段类型是安全的
 * @example
 * // 类型守卫示例
 * if (state.status === 'success') {
 *   // 这里 state.data 必定是 T，不会是 null
 *   console.log(state.data)
 * }
 */
export type RequestState<T> = IdleState | RequestLoadingState | SuccessState<T> | ErrorState;

/**
 * 向后兼容别名 - 用于包含加载状态和数据的对象
 */
export type StateWithLoading<T> = RequestState<T>;

/**
 * 向后兼容别名 - 用于加载状态字符串
 */
export type LoadingState = RequestStatus;

// ============ 表单类型 ============
/**
 * 表单字段配置
 */
export interface FormField {
  /** 字段名称 */
  name: string;
  /** 字段标签 */
  label: string;
  /** 字段类型 */
  type: 'text' | 'number' | 'email' | 'password' | 'select' | 'checkbox' | 'radio';
  /** 是否必填 */
  required?: boolean;
  /** 占位符 */
  placeholder?: string;
  /** 选项（用于 select、checkbox、radio） */
  options?: Array<{ label: string; value: any }>;
  /** 验证规则 */
  validation?: ValidationRule[];
}

/**
 * 验证规则
 */
export interface ValidationRule {
  /** 规则类型 */
  type: 'required' | 'email' | 'min' | 'max' | 'pattern' | 'custom';
  /** 规则值 */
  value?: any;
  /** 错误消息 */
  message: string;
}

/**
 * 表单状态
 */
export interface FormState<T = AnyObject> {
  /** 表单值 */
  values: T;
  /** 错误信息 */
  errors: Partial<Record<keyof T, string>>;
  /** 触摸状态 */
  touched: Partial<Record<keyof T, boolean>>;
  /** 是否有效 */
  isValid: boolean;
}

// ============ 通知类型 ============
/**
 * 通知消息
 */
export interface Notification {
  /** 通知 ID */
  id: string;
  /** 通知类型 */
  type: 'success' | 'error' | 'warning' | 'info';
  /** 通知标题 */
  title: string;
  /** 通知消息 */
  message?: string;
  /** 持续时间（毫秒） */
  duration?: number;
  /** 时间戳 */
  timestamp?: number;
  /** 是否已读 */
  read?: boolean;
}

// ============ 配置类型 ============
/**
 * 应用配置
 * 主题配置由 theme.ts 单独管理，此处不再包含
 */
export interface AppConfig {
  /** API 配置 */
  api: {
    baseURL: string;
    timeout: number;
    retries: number;
  };
  /** 功能开关 */
  features: Record<string, boolean>;
  /** 调试模式 */
  debug: boolean;
}

// ============ 类型守卫 ============
/**
 * 检查值是否为字符串。
 */
export function isString(value: unknown): value is string {
  return typeof value === 'string';
}

/**
 * 检查值是否为数字。
 */
export function isNumber(value: unknown): value is number {
  return typeof value === 'number' && !isNaN(value);
}

/**
 * 检查值是否为布尔值。
 */
export function isBoolean(value: unknown): value is boolean {
  return typeof value === 'boolean';
}

/**
 * 检查值是否为对象
 */
export function isObject(value: unknown): value is object {
  return value !== null && typeof value === 'object' && !Array.isArray(value);
}

/**
 * 检查值是否为数组
 */
export function isArray<T = any>(value: unknown): value is T[] {
  return Array.isArray(value);
}

/**
 * 检查值是否为函数
 */
export function isFunction(value: unknown): value is Function {
  return typeof value === 'function';
}

/**
 * 检查值是否为日期
 */
export function isDate(value: unknown): value is Date {
  return value instanceof Date && !isNaN(value.getTime());
}

/**
 * 检查值是否为空，包含 null、undefined、空字符串、空数组和空对象。
 */
export function isEmpty(value: unknown): boolean {
  if (value === null || value === undefined) return true;
  if (isString(value) || isArray(value)) return value.length === 0;
  if (isObject(value)) return Object.keys(value).length === 0;
  return false;
}

// ============ 工具函数 ============
/**
 * 断言永不发生
 * 用于类型检查，确保所有分支都被处理
 */
export function assertNever(value: never): never {
  throw new Error(`Unexpected value: ${value}`);
}

/**
 * 类型断言
 * 使用类型守卫函数进行类型断言
 */
export function assertType<T>(value: unknown, guard: (value: unknown) => value is T): T {
  if (!guard(value)) {
    throw new Error('Type assertion failed');
  }
  return value;
}

/**
 * 创建类型化对象
 */
export function createTypedObject<T extends Record<string, any>>(obj: T): T {
  return obj;
}

/**
 * 深度克隆对象
 * 递归克隆对象及其嵌套属性
 */
export function deepClone<T>(obj: T): T {
  if (obj === null || typeof obj !== 'object') return obj;
  if (obj instanceof Date) return new Date(obj.getTime()) as unknown as T;
  if (obj instanceof Array) return obj.map((item) => deepClone(item)) as unknown as T;
  if (typeof obj === 'object') {
    const cloned = {} as T;
    for (const key in obj) {
      if (obj.hasOwnProperty(key)) {
        cloned[key] = deepClone(obj[key]);
      }
    }
    return cloned;
  }
  return obj;
}
