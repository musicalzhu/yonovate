import { describe, it, expect } from 'vitest';
import { useDeviceStatus } from '../useDeviceStatus';
import { mount } from '@vue/test-utils';
import { createMemoryHistory, createRouter } from 'vue-router';
import { VueQueryPlugin, QueryClient } from '@tanstack/vue-query';
import { computed } from 'vue';

describe('useDeviceStatus', () => {
  it('应该返回设备信息', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/', component: { template: '<div>Home</div>' } }],
    });

    const queryClient = new QueryClient();

    let result: any = null;

    const wrapper = mount(
      {
        template: '<div>{{ deviceInfoData?.deviceId }}</div>',
        setup() {
          const deviceIP = computed(() => '');
          result = useDeviceStatus(deviceIP);
          return {
            deviceInfoData: result.deviceInfoData?.value,
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

    expect(result.deviceInfoData).toBeDefined();
    expect(result.deviceInfoData.value).toBeDefined();
    expect(result.deviceInfoData.value.deviceId).toBe('esp32-dev-001');

    wrapper.unmount();
  });
});