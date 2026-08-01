/**
 * Vue 3.5+ UI 状态管理 Store (Pinia Setup 语法)
 *
 * 管理应用的 UI 状态，包括侧边栏、语言、加载状态等
 */
import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import { setStorage, getStorage } from '@/services/storage/storageService';
import { useNotificationsStore } from './notifications';

/**
 * 日志条目类型
 */
export interface LogEntry {
  id: string;
  timestamp: string;
  level: 'INFO' | 'WARN' | 'ERROR';
  message: string;
}

/**
 * UI 状态管理 Store
 */
export const useUIStore = defineStore('ui', () => {
  // 状态
  /** 是否正在加载 */
  const isLoading = ref(false);
  /** 侧边栏是否折叠 */
  const sidebarCollapsed = ref(false);
  /** 当前语言 */
  const language = ref('zh-CN');
  /** 加载提示文字 */
  const loadingText = ref('加载中...');
  /** 系统日志 */
  const logs = ref<LogEntry[]>([]);

  // 计算属性
  /** 是否为移动设备 */
  const isMobile = computed(() => typeof window !== 'undefined' && window.innerWidth < 768);

  /** 是否在线 */
  const isOnline = computed(() => typeof navigator !== 'undefined' && navigator.onLine);

  // 方法
  /**
   * 统一把当前 UI 状态持久化到本地存储，避免散落在多个 setter 中。
   */
  const persistUiState = () => {
    setStorage('app_sidebarCollapsed', sidebarCollapsed.value);
    setStorage('app_language', language.value);
  };

  /**
   * 设置加载状态
   * @param loading - 是否正在加载
   * @param text - 加载提示文字
   */
  const setLoading = (loading: boolean, text?: string) => {
    isLoading.value = loading;
    if (text) {
      loadingText.value = text;
    }
  };

  /**
   * 切换侧边栏
   */
  const toggleSidebar = () => {
    sidebarCollapsed.value = !sidebarCollapsed.value;
    persistUiState();
  };

  /**
   * 设置侧边栏状态
   * @param collapsed - 是否折叠
   */
  const setSidebarCollapsed = (collapsed: boolean) => {
    sidebarCollapsed.value = collapsed;
    persistUiState();
  };

  /**
   * 设置语言
   * @param lang - 语言代码
   */
  const setLanguage = (lang: string) => {
    language.value = lang;
    persistUiState();
  };

  /**
   * 添加日志
   * @param level - 日志级别
   * @param message - 日志消息
   */
  const addLog = (level: 'INFO' | 'WARN' | 'ERROR', message: string) => {
    logs.value.unshift({
      id: Date.now().toString(),
      timestamp: new Date().toLocaleTimeString(),
      level,
      message,
    });

    // 限制日志数量为100条
    if (logs.value.length > 100) {
      logs.value = logs.value.slice(0, 100);
    }
  };

  /**
   * 清空日志
   */
  const clearLogs = () => {
    logs.value = [];
  };

  // 私有方法
  /**
   * 从本地存储加载
   */
  const loadFromStorage = () => {
    const savedSidebarCollapsed = getStorage<boolean>('app_sidebarCollapsed');
    const savedLanguage = getStorage<string>('app_language');

    if (savedSidebarCollapsed !== null) sidebarCollapsed.value = savedSidebarCollapsed;
    if (savedLanguage) language.value = savedLanguage;
  };

  /**
   * 初始化
   */
  const init = () => {
    loadFromStorage();

    // 监听网络状态
    if (typeof window !== 'undefined') {
      const notificationsStore = useNotificationsStore();

      window.addEventListener('online', () => {
        notificationsStore.showSuccess('网络连接恢复', '网络连接已恢复');
      });

      window.addEventListener('offline', () => {
        notificationsStore.showWarning('网络连接断开', '网络连接已断开，部分功能可能不可用');
      });
    }
  };

  // 返回 store 接口
  return {
    // 状态
    isLoading,
    sidebarCollapsed,
    language,
    loadingText,
    logs,

    // 计算属性
    isMobile,
    isOnline,

    // 方法
    setLoading,
    toggleSidebar,
    setSidebarCollapsed,
    setLanguage,
    addLog,
    clearLogs,
    init,
  };
});
