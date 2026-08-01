/**
 * Vitest 测试环境初始化文件
 *
 * 【功能说明】
 * 在所有测试运行前初始化测试环境，包括：
 * - Pinia 状态管理初始化
 * - TanStack Query 配置
 * - localStorage Mock
 * - MSW (Mock Service Worker) 配置
 *
 * @module vitest.setup
 */
import { createPinia } from 'pinia';
import { setActivePinia } from 'pinia';
import { VueQueryPlugin, QueryClient } from '@tanstack/vue-query';
import { createApp } from 'vue';
import { server } from '@/mocks/server';
import { setBaseURL } from '@/services/httpClient';

// 初始化 Pinia 状态管理
setActivePinia(createPinia());

/**
 * TanStack Query 客户端配置
 * 配置查询的默认行为：缓存策略、重试次数、重新获取时机等
 */
const queryClient = new QueryClient({
  defaultOptions: {
    queries: {
      staleTime: 1000 * 60 * 5, // 5分钟内数据视为新鲜
      retry: 0, // 不自动重试失败的请求
      refetchOnMount: true, // 组件挂载时重新获取数据
    },
  },
});

// 创建临时 Vue 应用实例，用于注册 VueQueryPlugin
const app = createApp({ template: '<div></div>' });
app.use(VueQueryPlugin, { queryClient });

/**
 * localStorage Mock 数据存储
 * 使用内存对象模拟 localStorage 的持久化存储
 */
const storageData: Record<string, string> = {};

/**
 * localStorage Mock 实现
 * 提供与浏览器 localStorage 相同的 API 接口
 */
const localStorageMock = {
  getItem: vi.fn((key: string) => storageData[key] || null),
  setItem: vi.fn((key: string, value: string) => {
    storageData[key] = value;
  }),
  removeItem: vi.fn((key: string) => {
    delete storageData[key];
  }),
  clear: vi.fn(() => {
    Object.keys(storageData).forEach((key) => delete storageData[key]);
  }),
};

// 将 localStorage Mock 绑定到 window 对象
Object.defineProperty(window, 'localStorage', {
  value: localStorageMock,
});

storageData['api_base_url'] = '/';
storageData['esp32-ip'] = '';
setBaseURL('/');

beforeAll(() => {
  server.listen({
    onUnhandledRequest: 'error',
  });
});

afterAll(() => {
  server.close();
});

beforeEach(() => {
  server.resetHandlers();
});
