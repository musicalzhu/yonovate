/**
 * useTemplates Composable 单元测试
 */
import { describe, it, expect, beforeEach, afterEach } from 'vitest';
import { useTemplates } from '../useTemplates';
import { mount } from '@vue/test-utils';
import { VueQueryPlugin, QueryClient } from '@tanstack/vue-query';
import { ref } from 'vue';

describe('useTemplates', () => {
  beforeEach(() => {
    localStorage.clear();
    vi.clearAllMocks();
  });

  afterEach(() => {
    localStorage.clear();
  });

  it('应该返回默认模板列表', async () => {
    const queryClient = new QueryClient();

    const templatesRef = ref<any>(null);

    const wrapper = mount(
      {
        template: '<div>{{ templatesList?.length }}</div>',
        setup() {
          const { templates } = useTemplates();
          templatesRef.value = templates;
          return {
            templatesList: templates,
          };
        },
      },
      {
        global: {
          plugins: [[VueQueryPlugin, { queryClient }]],
        },
      }
    );

    await new Promise((resolve) => setTimeout(resolve, 100));

    expect(templatesRef.value.value.length).toBeGreaterThan(0);

    const elevatorTemplate = templatesRef.value.value.find((t: any) => t.name === '电梯控制');
    expect(elevatorTemplate).toBeDefined();
    expect(elevatorTemplate?.category).toBe('电梯');

    wrapper.unmount();
  });

  it('应该能够保存新模板', async () => {
    const queryClient = new QueryClient();

    const templatesRef = ref<any>(null);
    let saveTemplateFn: any;

    const wrapper = mount(
      {
        template: '<div>{{ templatesList?.length }}</div>',
        setup() {
          const { templates, saveTemplate } = useTemplates();
          templatesRef.value = templates;
          saveTemplateFn = saveTemplate;
          return {
            templatesList: templates,
          };
        },
      },
      {
        global: {
          plugins: [[VueQueryPlugin, { queryClient }]],
        },
      }
    );

    await new Promise((resolve) => setTimeout(resolve, 100));

    const initialCount = templatesRef.value.value.length;

    saveTemplateFn({
      name: '测试模板',
      description: '这是一个测试模板',
      category: '测试',
      config: {
        initialState: 'TEST',
        transitions: [],
        stateEffects: [],
        groupModes: [0, 0, 0, 0],
        inputModuleTypes: [0, 0, 0, 0],
        edgeTypes: [0, 0, 0, 0],
        outputModuleTypes: [0, 0, 0, 0],
      },
    });

    expect(templatesRef.value.value.length).toBe(initialCount + 1);

    wrapper.unmount();
  });

  it('应该能够删除模板', async () => {
    const queryClient = new QueryClient();

    const templatesRef = ref<any>(null);
    let deleteTemplateFn: any;

    const wrapper = mount(
      {
        template: '<div>{{ templatesList?.length }}</div>',
        setup() {
          const { templates, deleteTemplate } = useTemplates();
          templatesRef.value = templates;
          deleteTemplateFn = deleteTemplate;
          return {
            templatesList: templates,
          };
        },
      },
      {
        global: {
          plugins: [[VueQueryPlugin, { queryClient }]],
        },
      }
    );

    await new Promise((resolve) => setTimeout(resolve, 100));

    const templateToDelete = templatesRef.value.value[0].name;
    const initialCount = templatesRef.value.value.length;

    const deleted = deleteTemplateFn(templateToDelete);

    expect(deleted).toBe(true);
    expect(templatesRef.value.value.length).toBe(initialCount - 1);

    wrapper.unmount();
  });

  it('应该能够获取指定模板', async () => {
    const queryClient = new QueryClient();

    const templatesRef = ref<any>(null);
    let getTemplateFn: any;

    const wrapper = mount(
      {
        template: '<div>{{ templatesList?.length }}</div>',
        setup() {
          const { templates, getTemplate } = useTemplates();
          templatesRef.value = templates;
          getTemplateFn = getTemplate;
          return {
            templatesList: templates,
          };
        },
      },
      {
        global: {
          plugins: [[VueQueryPlugin, { queryClient }]],
        },
      }
    );

    await new Promise((resolve) => setTimeout(resolve, 100));

    const templateName = templatesRef.value.value[0].name;

    const template = getTemplateFn(templateName);

    expect(template).toBeDefined();
    expect(template?.name).toBe(templateName);

    wrapper.unmount();
  });

  it('应该返回分类列表', async () => {
    const queryClient = new QueryClient();

    const categoriesRef = ref<any>(null);

    const wrapper = mount(
      {
        template: '<div>{{ categoriesList?.length }}</div>',
        setup() {
          const { categories } = useTemplates();
          categoriesRef.value = categories;
          return {
            categoriesList: categories,
          };
        },
      },
      {
        global: {
          plugins: [[VueQueryPlugin, { queryClient }]],
        },
      }
    );

    await new Promise((resolve) => setTimeout(resolve, 100));

    expect(categoriesRef.value.value).toContain('all');

    wrapper.unmount();
  });
});
