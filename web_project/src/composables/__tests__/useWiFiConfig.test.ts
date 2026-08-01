import { describe, it, expect } from 'vitest';
import { useWiFiConfig } from '../useWiFiConfig';
import { mount } from '@vue/test-utils';
import { createMemoryHistory, createRouter } from 'vue-router';
import { VueQueryPlugin, QueryClient } from '@tanstack/vue-query';

describe('useWiFiConfig', () => {
  it('应该返回 WiFi 配置', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/', component: { template: '<div>Home</div>' } }],
    });

    const queryClient = new QueryClient();

    let result: any = null;

    const wrapper = mount(
      {
        template: '<div>{{ configData?.ssid }}</div>',
        setup() {
          result = useWiFiConfig();
          return {
            configData: result.wifiConfigData?.value,
          };
        },
      },
      {
        global: {
          plugins: [router, [VueQueryPlugin, { queryClient }]],
        },
      }
    );

    await new Promise((resolve) => setTimeout(resolve, 500));

    expect(result.wifiConfigData).toBeDefined();
    expect(result.wifiConfigData.value).toBeDefined();
    expect(result.wifiConfigData.value.ssid).toBe('TestWiFi');

    wrapper.unmount();
  });

  it('应该返回 WiFi 状态', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/', component: { template: '<div>Home</div>' } }],
    });

    const queryClient = new QueryClient();

    let result: any = null;

    const wrapper = mount(
      {
        template: '<div>{{ statusData?.connected }}</div>',
        setup() {
          result = useWiFiConfig();
          return {
            statusData: result.wifiStatusData?.value,
          };
        },
      },
      {
        global: {
          plugins: [router, [VueQueryPlugin, { queryClient }]],
        },
      }
    );

    await new Promise((resolve) => setTimeout(resolve, 500));

    expect(result.wifiStatusData).toBeDefined();
    expect(result.wifiStatusData.value).toBeDefined();
    expect(result.wifiStatusData.value.connected).toBe(true);

    wrapper.unmount();
  });

  it('应该返回扫描到的网络列表', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/', component: { template: '<div>Home</div>' } }],
    });

    const queryClient = new QueryClient();

    let result: any = null;

    const wrapper = mount(
      {
        template: '<div>{{ networks?.length }}</div>',
        setup() {
          result = useWiFiConfig();
          return {
            networks: result.networksData?.value,
          };
        },
      },
      {
        global: {
          plugins: [router, [VueQueryPlugin, { queryClient }]],
        },
      }
    );

    await result.refetchNetworks();
    await new Promise((resolve) => setTimeout(resolve, 500));

    expect(result.networksData).toBeDefined();
    expect(result.networksData.value).toBeDefined();
    expect(result.networksData.value.length).toBeGreaterThan(0);

    wrapper.unmount();
  });
});
