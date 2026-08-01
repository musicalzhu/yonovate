import { describe, it, expect } from 'vitest';
import { useGPIOStatus } from '../useGPIO';
import { mount } from '@vue/test-utils';
import { createMemoryHistory, createRouter } from 'vue-router';
import { VueQueryPlugin, QueryClient } from '@tanstack/vue-query';

describe('useGPIOStatus', () => {
  it('应该返回 GPIO 状态列表', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/', component: { template: '<div>Home</div>' } }],
    });

    const queryClient = new QueryClient();

    let result: any = null;

    const wrapper = mount(
      {
        template: '<div>{{ gpioData?.length }}</div>',
        setup() {
          result = useGPIOStatus();
          return {
            gpioData: result.gpioStatus?.value,
          };
        },
      },
      {
        global: {
          plugins: [router, [VueQueryPlugin, { queryClient }]],
        },
      }
    );

    await new Promise((resolve) => setTimeout(resolve, 100));

    expect(result.gpioStatus).toBeDefined();
    expect(result.gpioStatus.value).toBeDefined();
    expect(result.gpioStatus.value.length).toBeGreaterThan(0);
    expect(result.gpioStatus.value[0].pin).toBeDefined();

    wrapper.unmount();
  });
});
