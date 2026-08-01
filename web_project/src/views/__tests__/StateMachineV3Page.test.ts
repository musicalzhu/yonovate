/**
 * StateMachineV3Page 单元测试
 */
import { describe, it, expect } from 'vitest';
import { shallowMount } from '@vue/test-utils';
import { createMemoryHistory, createRouter } from 'vue-router';
import { VueQueryPlugin, QueryClient } from '@tanstack/vue-query';
import ElementPlus from 'element-plus';
import StateMachineV3Page from '@/views/StateMachineV3Page.vue';

describe('StateMachineV3Page', () => {
  it('应该正确渲染页面', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/state-machine', component: StateMachineV3Page }],
    });

    const queryClient = new QueryClient();

    const wrapper = shallowMount(StateMachineV3Page, {
      global: {
        plugins: [ElementPlus, router, [VueQueryPlugin, { queryClient }]],
      },
    });

    await new Promise((resolve) => setTimeout(resolve, 100));

    expect(wrapper.exists()).toBe(true);

    wrapper.unmount();
  });
});
