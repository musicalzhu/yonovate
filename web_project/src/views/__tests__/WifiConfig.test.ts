/**
 * WifiConfig Page 单元测试
 */
import { describe, it, expect } from 'vitest';
import { mount } from '@vue/test-utils';
import { createMemoryHistory, createRouter } from 'vue-router';
import { VueQueryPlugin, QueryClient } from '@tanstack/vue-query';
import ElementPlus from 'element-plus';
import WifiConfig from '@/views/WifiConfig.vue';

describe('WifiConfig Page', () => {
  it('应该正确渲染页面', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/wifi', component: WifiConfig }],
    });

    const queryClient = new QueryClient();

    const wrapper = mount(WifiConfig, {
      global: {
        plugins: [ElementPlus, router, [VueQueryPlugin, { queryClient }]],
      },
    });

    await new Promise((resolve) => setTimeout(resolve, 100));

    expect(wrapper.exists()).toBe(true);

    wrapper.unmount();
  });
});
