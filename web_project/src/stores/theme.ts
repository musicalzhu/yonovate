/**
 * Vue 3.5+ 主题管理 Store (Pinia Setup 语法)
 *
 * 管理应用的主题配置，包括颜色、间距、字体等
 */
import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import { setStorage, getStorage } from '@/services/storage/storageService';

/**
 * 主题配置接口
 */
export interface ThemeConfig {
  primary: string;
  secondary: string;
  accent: string;
  background: string;
  surface: string;
  text: string;
  border: string;
  shadow: string;
}

/**
 * 默认主题配置
 */
const DEFAULT_THEME: ThemeConfig = {
  primary: '#409eff',
  secondary: '#909399',
  accent: '#67c23a',
  background: '#ffffff',
  surface: '#f5f5f5',
  text: '#303133',
  border: '#e5e7eb',
  shadow: 'rgba(0, 0, 0, 0.1)',
};

/**
 * 主题管理 Store
 */
export const useThemeStore = defineStore('theme', () => {
  const theme = ref<ThemeConfig>({ ...DEFAULT_THEME });

  const currentThemeColors = computed(() => theme.value);

  const applyTheme = () => {
    const root = document.documentElement;

    root.style.setProperty('--color-primary', theme.value.primary);
    root.style.setProperty('--color-secondary', theme.value.secondary);
    root.style.setProperty('--color-accent', theme.value.accent);
    root.style.setProperty('--color-background', theme.value.background);
    root.style.setProperty('--color-surface', theme.value.surface);
    root.style.setProperty('--color-text', theme.value.text);
    root.style.setProperty('--color-border', theme.value.border);
    root.style.setProperty('--color-shadow', theme.value.shadow);
  };

  const setTheme = (newTheme: Partial<ThemeConfig>) => {
    theme.value = { ...theme.value, ...newTheme };
    saveToStorage();
    applyTheme();
  };

  const saveToStorage = () => {
    setStorage('app_theme', theme.value);
  };

  const loadFromStorage = () => {
    const savedTheme = getStorage<ThemeConfig>('app_theme');

    if (savedTheme) theme.value = savedTheme;

    applyTheme();
  };

  const init = () => {
    loadFromStorage();
  };

  return {
    theme,
    currentThemeColors,
    setTheme,
    applyTheme,
    init,
  };
});