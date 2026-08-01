import { describe, it, expect, beforeEach, vi } from 'vitest';
import { useConfigStorage } from '../useConfigStorage';

// Mock useUiStore
vi.mock('../../stores/ui', () => ({
  useUiStore: () => ({
    addLog: vi.fn(),
  }),
}));

describe('useConfigStorage', () => {
  const mockConfigKey = 'test-config';
  const defaultConfig = {
    theme: 'light',
    language: 'zh-CN',
    fontSize: 16,
  };

  beforeEach(() => {
    // 清除localStorage
    localStorage.clear();
    vi.clearAllMocks();
  });

  it('应该初始化为默认配置', () => {
    const { config } = useConfigStorage(mockConfigKey, defaultConfig);

    expect(config.value).toEqual(defaultConfig);
  });

  it('应该能够保存配置到localStorage', async () => {
    const { config, saveConfig } = useConfigStorage(mockConfigKey, defaultConfig);

    config.value = { ...defaultConfig, theme: 'dark' };
    await saveConfig();

    const savedData = localStorage.getItem(mockConfigKey);
    expect(savedData).toBe(JSON.stringify({ ...defaultConfig, theme: 'dark' }));
  });

  it('应该能够从localStorage加载配置', async () => {
    const testConfig = { ...defaultConfig, theme: 'dark' };
    localStorage.setItem(mockConfigKey, JSON.stringify(testConfig));

    const { config, loadConfig } = useConfigStorage(mockConfigKey, defaultConfig);
    await loadConfig();

    expect(config.value).toEqual(testConfig);
  });

  it('当localStorage中没有配置时，应该使用默认配置', async () => {
    const { config, loadConfig } = useConfigStorage(mockConfigKey, defaultConfig);
    await loadConfig();

    expect(config.value).toEqual(defaultConfig);
  });

  it('应该能够重置配置为默认值', () => {
    const { config, resetConfig } = useConfigStorage(mockConfigKey, defaultConfig);

    config.value = { ...defaultConfig, theme: 'dark' };
    resetConfig();

    expect(config.value).toEqual(defaultConfig);
  });

  it('应该能够清除错误状态', () => {
    const { error, clearError } = useConfigStorage(mockConfigKey, defaultConfig);

    // 模拟错误状态
    error.value = 'Test error';
    expect(error.value).toBe('Test error');

    clearError();
    expect(error.value).toBeNull();
  });

  it('应该处理JSON解析错误', async () => {
    localStorage.setItem(mockConfigKey, 'invalid json');

    const { config, loadConfig, error } = useConfigStorage(mockConfigKey, defaultConfig);
    await loadConfig();

    expect(error.value).toBeTruthy();
    expect(config.value).toEqual(defaultConfig);
  });

  it('应该正确设置loading状态', async () => {
    const { loading, saveConfig } = useConfigStorage(mockConfigKey, defaultConfig);

    expect(loading.value).toBe(false);

    await saveConfig();
    expect(loading.value).toBe(false);
  });
});
