import { ref } from 'vue';

/**
 * Vue 3.5+ 通用配置存储组合式函数
 *
 * 提供配置的加载、保存功能，支持本地存储。
 * 该组合式函数封装了常见的配置管理逻辑，包括：
 * - 从localStorage加载配置
 * - 保存配置到localStorage
 * - 错误处理和日志记录
 * - 加载状态管理
 *
 * @template T - 配置数据的类型
 * @param configKey - localStorage中存储配置的键名
 * @param defaultConfig - 配置的默认值
 * @returns 包含配置状态和操作方法的对象
 *
 * @example
 * ```typescript
 * const { config, loading, error, loadConfig, saveConfig, resetConfig } = useConfigStorage(
 *   'my-config',
 *   { theme: 'dark', language: 'zh-CN' }
 * )
 * ```
 */
export function useConfigStorage<T>(configKey: string, defaultConfig: T) {
  const loading = ref(false);
  const error = ref<string | null>(null);
  const config = ref<T>(defaultConfig);

  /**
   * 从本地存储加载配置
   *
   * 该函数会尝试从localStorage中读取配置数据。
   * 如果找到配置，则解析并更新config响应式变量；
   * 如果未找到配置，则使用默认配置并记录日志。
   *
   * @throws 当JSON解析失败时抛出错误
   */
  const loadConfig = async (): Promise<void> => {
    loading.value = true;
    error.value = null;

    try {
      const savedConfig = localStorage.getItem(configKey);
      if (savedConfig) {
        const parsedConfig = JSON.parse(savedConfig);
        config.value = parsedConfig;
        console.info(`${configKey}配置加载成功`);
      } else {
        console.info(`未找到保存的${configKey}配置，使用默认配置`);
      }
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : String(err);
      error.value = errorMessage;
      console.error(`${configKey}配置加载失败: ${errorMessage}`);
    } finally {
      loading.value = false;
    }
  };

  /**
   * 保存配置到本地存储
   *
   * 该函数会将当前配置序列化为JSON字符串并保存到localStorage。
   * 保存成功后会记录日志，失败时会记录错误信息。
   *
   * @throws 当localStorage操作失败时抛出错误
   */
  const saveConfig = async (): Promise<void> => {
    loading.value = true;
    error.value = null;

    try {
      localStorage.setItem(configKey, JSON.stringify(config.value));
      console.info(`${configKey}配置保存成功`);
    } catch (err) {
      const errorMessage = err instanceof Error ? err.message : String(err);
      error.value = errorMessage;
      console.error(`${configKey}配置保存失败: ${errorMessage}`);
    } finally {
      loading.value = false;
    }
  };

  /**
   * 重置配置为默认值
   *
   * 该函数会将配置恢复为传入的默认值，并记录日志。
   * 适用于需要清除用户自定义配置的场景。
   */
  const resetConfig = (): void => {
    config.value = defaultConfig;
    console.info(`${configKey}配置已重置为默认值`);
  };

  /**
   * 清除错误状态
   *
   * 该函数会将error状态重置为null，用于在用户操作后清除错误提示。
   */
  const clearError = (): void => {
    error.value = null;
  };

  return {
    config,
    loading,
    error,
    loadConfig,
    saveConfig,
    resetConfig,
    clearError,
  };
}
