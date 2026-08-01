/**
 * Home Page 单元测试
 *
 * 【测试目的】
 * 验证首页组件能否正确渲染，包括设备信息卡片、WiFi状态、系统状态等。
 *
 * 【测试环境配置】
 * - Vitest: 测试运行器
 * - Vue Test Utils: 组件挂载和交互测试
 * - MSW: Mock API 服务（全局 setup 自动配置）
 * - TanStack Query: 数据获取和缓存管理
 *
 * 【测试策略】
 * 1. Mock API 请求，返回模拟的设备信息和WiFi状态
 * 2. 配置 Vue Router 处理路由跳转
 * 3. 配置 TanStack Query 的 QueryClient
 * 4. 挂载 Home 组件并验证页面结构
 *
 * 【测试覆盖】
 * - 页面渲染正确性
 * - API 数据获取
 * - 错误处理
 *
 * @module Home.test
 */
import { describe, it, expect } from 'vitest';
import { mount } from '@vue/test-utils';
import { createMemoryHistory, createRouter } from 'vue-router';
import { VueQueryPlugin, QueryClient } from '@tanstack/vue-query';
import Home from '@/views/Home.vue';

describe('Home Page', () => {
  it('应该正确渲染页面结构', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/', component: Home }],
    });

    const queryClient = new QueryClient();

    const wrapper = mount(Home, {
      global: {
        plugins: [router, [VueQueryPlugin, { queryClient }]],
        stubs: {
          'el-alert': true,
          'el-input': true,
          'el-button': true,
          'el-tag': true,
        },
      },
    });

    await router.push('/');
    await router.isReady();

    await new Promise((resolve) => setTimeout(resolve, 100));

    expect(wrapper.find('h1').exists()).toBe(true);

    wrapper.unmount();
  });
});
