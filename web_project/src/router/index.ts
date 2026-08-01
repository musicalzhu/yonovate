/**
 * Vue Router 路由配置
 *
 * 【功能说明】
 * 定义应用的路由规则，管理页面之间的导航
 * 使用懒加载优化性能，减少首屏加载时间
 *
 * 【核心概念】
 * - Vue Router：Vue 官方路由管理器
 * - RouteRecordRaw：路由配置的类型定义
 * - 懒加载：动态导入组件，实现代码分割
 * - History 模式：URL 看起来像普通路径（无 # 号）
 *
 * 【路由列表】
 * | 路径            | 页面             | 功能             |
 * |-----------------|------------------|------------------|
 * | /               | Home             | 首页/设备控制    |
 * | /state-machine  | StateMachineV3   | 状态机配置       |
 * | /gpio-control   | GpioControl      | GPIO引脚控制     |
 * | /logic-circuit  | LogicCircuit     | 逻辑电路配置     |
 * | /wifi-config    | WifiConfig       | WiFi网络配置     |
 *
 * 【懒加载原理】
 * () => import('../views/Home.vue')
 * - 首次访问该路由时才加载组件代码
 * - 减少首屏 JS 文件大小
 * - 提高应用加载速度
 */
import { createRouter, createWebHistory, RouteRecordRaw } from 'vue-router';

/**
 * 路由配置数组
 *
 * 【说明】
 * 每个路由配置包含：
 * - path：URL 路径
 * - name：路由名称（用于编程式导航）
 * - component：页面组件
 */
const routes: RouteRecordRaw[] = [
  {
    /** 首页路径 */
    path: '/',
    /** 路由名称（可用于 router.push({ name: 'Home' })） */
    name: 'Home',
    /**
     * 组件懒加载
     * 【说明】
     * 使用动态 import，Webpack/Vite 会自动进行代码分割
     * 只有访问该路由时才加载对应的 JS 文件
     */
    component: () => import('../views/Home.vue'),
  },
  {
    /** 状态机配置页面路径 */
    path: '/state-machine-v3',
    name: 'StateMachineV3',
    component: () => import('../views/StateMachineV3Page.vue'),
  },
  {
    /** GPIO 控制页面路径 */
    path: '/gpio-control',
    name: 'GpioControl',
    component: () => import('../views/GpioControl.vue'),
  },
  {
    /** 逻辑电路配置页面路径 */
    path: '/logic-circuit',
    name: 'LogicCircuit',
    component: () => import('../views/LogicCircuit.vue'),
  },
  {
    /** WiFi 配置页面路径 */
    path: '/wifi-config',
    name: 'WifiConfig',
    component: () => import('../views/WifiConfig.vue'),
  },
];

/**
 * Vue Router 实例
 *
 * 【说明】
 * 创建路由实例，配置路由模式和路由表
 * 在 main.ts 中通过 app.use(router) 注册
 *
 * 【History 模式】
 * - createWebHistory：使用 HTML5 History API
 * - URL 看起来干净（如 /gpio-control，没有 # 号）
 * - 需要服务器配置支持（否则刷新会 404）
 */
const router = createRouter({
  /** 使用 HTML5 History 模式 */
  history: createWebHistory(),
  /** 路由配置数组 */
  routes,
});

// 导出路由实例，供 main.ts 使用
export default router;
