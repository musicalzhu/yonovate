import { describe, it, expect } from 'vitest';
import { useStateMachineConfig } from '../useStateMachineConfig';
import { mount } from '@vue/test-utils';
import { createMemoryHistory, createRouter } from 'vue-router';
import { VueQueryPlugin, QueryClient } from '@tanstack/vue-query';

describe('useStateMachineConfig', () => {
  it('应该返回状态机配置', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/', component: { template: '<div>Home</div>' } }],
    });

    const queryClient = new QueryClient();

    let result: any = null;

    const wrapper = mount(
      {
        template: '<div>{{ configData?.initialState }}</div>',
        setup() {
          result = useStateMachineConfig();
          return {
            configData: result.config?.value,
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

    expect(result.config).toBeDefined();
    expect(result.config.value).toBeDefined();
    expect(result.config.value.initialState).toBe('Idle');

    wrapper.unmount();
  });

  it('应该返回状态机状态', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/', component: { template: '<div>Home</div>' } }],
    });

    const queryClient = new QueryClient();

    let result: any = null;

    const wrapper = mount(
      {
        template: '<div>{{ statusData?.currentState }}</div>',
        setup() {
          result = useStateMachineConfig();
          return {
            statusData: result.status?.value,
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

    expect(result.status).toBeDefined();
    expect(result.status.value).toBeDefined();
    expect(result.status.value.currentState).toBe('Idle');

    wrapper.unmount();
  });

  it('应该提供转换管理方法', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/', component: { template: '<div>Home</div>' } }],
    });

    const queryClient = new QueryClient();

    let result: any = null;

    const wrapper = mount(
      {
        template: '<div>Test</div>',
        setup() {
          result = useStateMachineConfig();
          return {};
        },
      },
      {
        global: {
          plugins: [router, [VueQueryPlugin, { queryClient }]],
        },
      }
    );

    await new Promise((resolve) => setTimeout(resolve, 100));

    expect(result.addTransition).toBeDefined();
    expect(result.removeTransition).toBeDefined();
    expect(result.updateTransition).toBeDefined();
    expect(result.transitions).toBeDefined();
    expect(result.transitionsCount).toBeDefined();

    wrapper.unmount();
  });

  it('应该提供触发器管理方法', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/', component: { template: '<div>Home</div>' } }],
    });

    const queryClient = new QueryClient();

    let result: any = null;

    const wrapper = mount(
      {
        template: '<div>Test</div>',
        setup() {
          result = useStateMachineConfig();
          return {};
        },
      },
      {
        global: {
          plugins: [router, [VueQueryPlugin, { queryClient }]],
        },
      }
    );

    await new Promise((resolve) => setTimeout(resolve, 100));

    expect(result.addTrigger).toBeDefined();
    expect(result.removeTrigger).toBeDefined();
    expect(result.updateTrigger).toBeDefined();

    wrapper.unmount();
  });

  it('应该提供状态效果管理方法', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/', component: { template: '<div>Home</div>' } }],
    });

    const queryClient = new QueryClient();

    let result: any = null;

    const wrapper = mount(
      {
        template: '<div>Test</div>',
        setup() {
          result = useStateMachineConfig();
          return {};
        },
      },
      {
        global: {
          plugins: [router, [VueQueryPlugin, { queryClient }]],
        },
      }
    );

    await new Promise((resolve) => setTimeout(resolve, 100));

    expect(result.addStateEffect).toBeDefined();
    expect(result.removeStateEffect).toBeDefined();
    expect(result.updateStateEffect).toBeDefined();
    expect(result.stateEffects).toBeDefined();
    expect(result.stateEffectsCount).toBeDefined();

    wrapper.unmount();
  });

  it('应该提供保存和重置方法', async () => {
    const router = createRouter({
      history: createMemoryHistory(),
      routes: [{ path: '/', component: { template: '<div>Home</div>' } }],
    });

    const queryClient = new QueryClient();

    let result: any = null;

    const wrapper = mount(
      {
        template: '<div>Test</div>',
        setup() {
          result = useStateMachineConfig();
          return {};
        },
      },
      {
        global: {
          plugins: [router, [VueQueryPlugin, { queryClient }]],
        },
      }
    );

    await new Promise((resolve) => setTimeout(resolve, 100));

    expect(result.saveNow).toBeDefined();
    expect(result.updateConfig).toBeDefined();
    expect(result.partialUpdateConfig).toBeDefined();
    expect(result.reboot).toBeDefined();
    expect(result.refetchConfig).toBeDefined();
    expect(result.refetchStatus).toBeDefined();

    wrapper.unmount();
  });
});
