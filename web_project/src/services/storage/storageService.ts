// Vue 3.5+ 本地存储和会话管理工具（纯函数）
import { errorHandlerService } from '../notification/errorHandler';

/**
 * 本地存储的统一前缀，避免与第三方键名冲突。
 */
const PREFIX = 'app_';

/**
 * 过期项的后缀，用于标记带有效期的数据。
 */
const EXPIRY_SUFFIX = '_expiry';

/**
 * 收集当前所有以应用前缀开头的存储项，统一处理遍历逻辑，减少重复代码。
 *
 * @returns 包含键名和原始值的数组
 */
function getPrefixedStorageEntries(): Array<{ key: string; value: string | null }> {
  const entries: Array<{ key: string; value: string | null }> = [];

  for (let i = 0; i < localStorage.length; i++) {
    const key = localStorage.key(i);
    if (key && key.startsWith(PREFIX)) {
      entries.push({ key, value: localStorage.getItem(key) });
    }
  }

  return entries;
}

// LocalStorage 函数
/**
 * 设置 localStorage 中的值
 * @param key - 存储键名（会自动添加 'app_' 前缀）
 * @param value - 要存储的值
 * @example
 * ```ts
 * setStorage('userSettings', { theme: 'dark' })
 * ```
 */
export function setStorage<T>(key: string, value: T): void {
  try {
    const serializedValue = JSON.stringify(value);
    localStorage.setItem(PREFIX + key, serializedValue);
  } catch (error) {
    errorHandlerService.handleError(error, 'Failed to set localStorage');
  }
}

/**
 * 从 localStorage 获取值
 * @param key - 存储键名（会自动添加 'app_' 前缀）
 * @returns 存储的值，如果不存在则返回 null
 * @example
 * ```ts
 * const settings = getStorage<{ theme: string }>('userSettings')
 * // returns: { theme: 'dark' } or null
 * ```
 */
export function getStorage<T>(key: string): T | null {
  try {
    const item = localStorage.getItem(PREFIX + key);
    if (item === null) return null;
    return JSON.parse(item) as T;
  } catch (error) {
    errorHandlerService.handleError(error, 'Failed to get from localStorage');
    return null;
  }
}

/**
 * 从 localStorage 删除指定的键
 * @param key - 要删除的键名（会自动添加 'app_' 前缀）
 * @example
 * ```ts
 * removeStorage('userSettings')
 * ```
 */
export function removeStorage(key: string): void {
  try {
    localStorage.removeItem(PREFIX + key);
  } catch (error) {
    errorHandlerService.handleError(error, 'Failed to remove from localStorage');
  }
}

/**
 * 清空所有带 'app_' 前缀的 localStorage 项
 * @example
 * ```ts
 * clearStorage()
 * ```
 */
export function clearStorage(): void {
  try {
    getPrefixedStorageEntries().forEach(({ key }) => localStorage.removeItem(key));
  } catch (error) {
    errorHandlerService.handleError(error, 'Failed to clear localStorage');
  }
}

/**
 * 获取所有带 'app_' 前缀的 localStorage 键名
 * @returns 键名数组
 * @example
 * ```ts
 * const keys = getStorageKeys()
 * // returns: ['app_userSettings', 'app_theme', ...]
 * ```
 */
export function getStorageKeys(): string[] {
  return getPrefixedStorageEntries().map(({ key }) => key);
}

/**
 * 计算所有带 'app_' 前缀的 localStorage 项的总大小（字节）
 * @returns 总大小（字节）
 * @example
 * ```ts
 * const size = getStorageSize()
 * // returns: 1024
 * ```
 */
export function getStorageSize(): number {
  return getPrefixedStorageEntries().reduce((size, { key, value }) => {
    if (value) {
      return size + key.length + value.length;
    }
    return size;
  }, 0);
}

/**
 * 清理所有已过期的 localStorage 项。
 *
 * 该函数会扫描带过期后缀的键，并同步移除对应的数据项。
 * @example
 * ```ts
 * cleanupExpiredStorage()
 * ```
 */
export function cleanupExpiredStorage(): void {
  const now = Date.now();
  const keysToRemove: string[] = [];

  for (let i = 0; i < localStorage.length; i++) {
    const key = localStorage.key(i);
    if (key && key.endsWith(EXPIRY_SUFFIX)) {
      const expiryTime = localStorage.getItem(key);
      if (expiryTime && now > parseInt(expiryTime)) {
        keysToRemove.push(key);
      }
    }
  }

  keysToRemove.forEach((key) => {
    localStorage.removeItem(key);
    // 同时删除对应的数据项
    const dataKey = key.replace(EXPIRY_SUFFIX, '');
    if (dataKey.startsWith(PREFIX)) {
      localStorage.removeItem(dataKey);
    }
  });
}

// 定期清理过期项
if (typeof window !== 'undefined') {
  setInterval(() => {
    cleanupExpiredStorage();
  }, 60000); // 每分钟清理一次
}
