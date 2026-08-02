/**
 * 应用入口文件
 *
 * 【功能说明】
 * 创建 Vue 应用实例，配置全局插件和错误处理
 * 这是应用启动的起点
 *
 * 【核心概念】
 * - createApp：创建 Vue 应用实例
 * - Pinia：Vue 3 官方推荐的状态管理库
 * - VueQueryPlugin：TanStack Query 的 Vue 插件，用于数据获取和缓存
 * - Element Plus：Vue 3 的 UI 组件库
 * - Vue Router：Vue 官方路由管理器
 *
 * 【插件配置】
 * 1. Pinia：状态管理
 * 2. Vue Router：页面路由
 * 3. Element Plus：UI 组件和图标
 * 4. VueQueryPlugin：数据查询和缓存
 *
 * 【错误处理】
 * - Vue 全局错误处理器：捕获组件内的错误
 * - unhandledrejection：捕获未处理的 Promise 错误
 */
import { createApp } from 'vue';
import { createPinia } from 'pinia';
import { VueQueryPlugin, QueryClient, QueryCache, MutationCache } from '@tanstack/vue-query';
import ElementPlus from 'element-plus';
import 'element-plus/dist/index.css'; // Element Plus 样式
import './styles/global.css'; // 全局样式
import * as ElementPlusIconsVue from '@element-plus/icons-vue'; // Element Plus 图标
import App from './App.vue'; // 根组件
import router from './router'; // 路由配置
import { errorHandlerService } from './services/notification/errorHandler'; // 错误处理服务
import { worker } from './mocks/browser'; // MSW Service Worker

// ========== 创建 Vue 应用实例 ==========
const app = createApp(App);

// ========== 注册 Element Plus 图标 ==========
/**
 * 全局注册所有 Element Plus 图标组件
 * 这样在模板中可以直接使用 <el-icon-search /> 等图标
 */
for (const [key, component] of Object.entries(ElementPlusIconsVue)) {
  app.component(key, component);
}

// ========== 配置 TanStack Query ==========
/**
 * QueryClient 配置
 *
 * 【说明】
 * TanStack Query 用于管理 API 数据查询和缓存
 * 所有 composables（如 useDeviceInfo）都使用此 QueryClient
 *
 * 【配置选项】
 * - staleTime：数据新鲜时间，5分钟内不会重新请求
 * - retry：请求失败后重试次数
 * - retryDelay：重试延迟，指数增长
 * - queryCache：查询缓存，配置全局错误处理
 * - mutationCache：变更缓存，配置全局错误处理
 */
const queryClient = new QueryClient({
  queryCache: new QueryCache({
    onError: (error) => {
      errorHandlerService.handleApiError(error, '查询请求失败');
    },
  }),
  mutationCache: new MutationCache({
    onError: (error) => {
      errorHandlerService.handleApiError(error, '变更请求失败');
    },
  }),
  defaultOptions: {
    queries: {
      // 5分钟内数据保持新鲜（不会重新请求）
      staleTime: 1000 * 60 * 5,
      // 失败后最多重试3次
      retry: 3,
      // 重试间隔：指数增长，最大30秒
      // 第1次：1秒，第2次：2秒，第3次：4秒...
      retryDelay: (attemptIndex) => Math.min(1000 * 2 ** attemptIndex, 30000),
    },
  },
});

// ========== 注册全局插件 ==========
/**
 * 注册顺序很重要：
 * 1. Pinia：状态管理（必须先注册）
 * 2. Vue Router：路由管理
 * 3. Element Plus：UI 组件库
 * 4. VueQueryPlugin：数据查询
 */
const pinia = createPinia();
app.use(pinia); // 状态管理
app.use(router); // 路由
app.use(ElementPlus); // UI组件
app.use(VueQueryPlugin, { queryClient }); // 数据查询

// ========== 注册全局错误处理器 ==========
/**
 * Vue 全局错误处理器
 *
 * 【说明】
 * 捕获 Vue 组件生命周期、事件处理器中的错误
 * 防止错误导致应用崩溃
 *
 * @param err - 错误对象
 * @param _instance - 发生错误的组件实例
 * @param info - 错误来源信息（如生命周期名称）
 */
app.config.errorHandler = (err, _instance, info) => {
  // 使用错误处理服务统一处理
  errorHandlerService.handleError(err, `Vue 应用错误: ${info}`);

  // 开发环境打印详细错误信息
  if (import.meta.env.DEV) {
    console.error('[Vue Error]', err, info);
  }
};

// ========== 捕获未处理的 Promise 拒绝 ==========
/**
 * 全局 Promise 错误处理
 *
 * 【说明】
 * 捕获未被 catch 的 Promise rejection
 * 防止异步错误被忽略
 */
window.addEventListener('unhandledrejection', (event) => {
  errorHandlerService.handleError(event.reason, '未处理的 Promise 拒绝');
  // 阻止默认行为（控制台打印警告）
  event.preventDefault();

  if (import.meta.env.DEV) {
    console.error('[Unhandled Rejection]', event.reason);
  }
});

// ========== 启动 MSW Service Worker ==========
/**
 * 在开发环境启动 MSW Service Worker
 * 用于拦截和模拟 API 请求
 */
if (import.meta.env.DEV) {
  worker
    .start({
      onUnhandledRequest: 'bypass',
    })
    .then(() => {
      console.log('MSW worker started successfully');
      app.mount('#app');
    })
    .catch((err) => {
      console.error('Failed to start MSW worker:', err);
      app.mount('#app');
    });
} else {
  app.mount('#app');
}
