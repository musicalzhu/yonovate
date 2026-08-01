/**
 * GpioControl Page 单元测试
 */
import { describe, it, expect } from 'vitest';
import { mount } from '@vue/test-utils';
import { createMemoryHistory, createRouter } from 'vue-router';
import { VueQueryPlugin, QueryClient } from '@tanstack/vue-query';
import ElementPlus from 'element-plus';
import GpioControl from '@/views/GpioControl.vue';

describe('GpioControl Page', () => {
  it('应该正确渲染页面', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/gpio', component: GpioControl }],
    });

    const queryClient = new QueryClient();

    const wrapper = mount(GpioControl, {
      global: {
        plugins: [ElementPlus, router, [VueQueryPlugin, { queryClient }]],
      },
    });

    await new Promise((resolve) => setTimeout(resolve, 100));

    expect(wrapper.exists()).toBe(true);

    wrapper.unmount();
  });
});
