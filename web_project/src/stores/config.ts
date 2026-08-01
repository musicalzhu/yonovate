/**
 * Vue 3.5+ 应用配置管理 Store (Pinia Setup 语法)
 *
 * 管理应用的全局配置，包括 API、特性开关等
 * 主题配置由 theme.ts 单独管理
 */
import { defineStore } from 'pinia';
import { ref } from 'vue';
import type { AppConfig } from '@/types';
import { setStorage, getStorage } from '@/services/storage/storageService';

/**
 * 默认应用配置
 */
const DEFAULT_CONFIG: AppConfig = {
  api: {
    baseURL: 'http://192.168.3.55',
    timeout: 10000,
    retries: 3,
  },
  features: {},
  debug: false,
};

/**
 * 应用配置管理 Store
 */
export const useConfigStore = defineStore('config', () => {
  // 状态
  const config = ref<AppConfig>({ ...DEFAULT_CONFIG });

  /**
   * 统一把当前配置写入本地存储，避免各处重复调用持久化逻辑。
   */
  const persistConfig = () => {
    saveToStorage();
  };

  // 方法
  /**
   * 设置应用配置
   * @param newConfig - 部分应用配置
   */
  const setConfig = (newConfig: Partial<AppConfig>) => {
    config.value = { ...config.value, ...newConfig };
    persistConfig();
  };

  /**
   * 更新 API 配置
   * @param apiConfig - 部分 API 配置
   */
  const updateApiConfig = (apiConfig: Partial<AppConfig['api']>) => {
    config.value.api = { ...config.value.api, ...apiConfig };
    persistConfig();
  };

  /**
   * 切换调试模式
   */
  const toggleDebug = () => {
    config.value.debug = !config.value.debug;
    persistConfig();
  };

  /**
   * 设置特性开关
   * @param featureName - 特性名称
   * @param enabled - 是否启用
   */
  const setFeature = (featureName: string, enabled: boolean) => {
    config.value.features = {
      ...config.value.features,
      [featureName]: enabled,
    };
    persistConfig();
  };

  /**
   * 检查特性是否启用
   * @param featureName - 特性名称
   * @returns 是否启用
   */
  const isFeatureEnabled = (featureName: string): boolean => {
    return config.value.features[featureName] ?? false;
  };

  // 私有方法
  /**
   * 保存到本地存储
   */
  const saveToStorage = () => {
    setStorage('app_config', config.value);
  };

  /**
   * 从本地存储加载
   */
  const loadFromStorage = () => {
    const savedConfig = getStorage<AppConfig>('app_config');
    if (savedConfig) {
      config.value = savedConfig;
    }
  };

  /**
   * 初始化
   */
  const init = () => {
    loadFromStorage();
  };

  // 返回 store 接口
  return {
    // 状态
    config,

    // 方法
    setConfig,
    updateApiConfig,
    toggleDebug,
    setFeature,
    isFeatureEnabled,
    init,
  };
});
