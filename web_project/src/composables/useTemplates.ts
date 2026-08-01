/**
 * 状态机模板管理组合式函数
 *
 * 管理状态机模板的存储、加载、保存和应用
 * 模板数据持久化到 localStorage
 */
import { ref, computed, onMounted, type Ref, type ComputedRef } from 'vue';
import { LogicType, TriggerType } from '@/types';
import { setStorage, getStorage } from '@/services/storage/storageService';
import type { StateMachineConfig } from '@/types';

/**
 * 状态机模板接口
 */
export interface StateMachineTemplate {
  /** 模板名称 */
  name: string;
  /** 模板描述 */
  description: string;
  /** 模板分类 */
  category: string;
  /** 状态机配置 */
  config: StateMachineConfig;
  /** 模板元数据 */
  metadata: {
    /** 版本号 */
    version: string;
    /** 作者 */
    author: string;
    /** 创建时间 */
    createdAt: string;
    /** 标签数组 */
    tags: string[];
  };
}

/**
 * 默认模板数据
 */
const DEFAULT_TEMPLATES: StateMachineTemplate[] = [
  {
    name: '电梯控制',
    description: '基础电梯状态机控制模板',
    category: '电梯',
    config: {
      initialState: 'IDLE',
      transitions: [
        {
          currentState: 'IDLE',
          nextState: 'MOVING_UP',
          triggers: [
            {
              triggerSource: 'P0',
              triggerType: TriggerType.LEVEL,
              operator: 0,
              triggerValue: 1,
            },
          ],
          triggerLogic: LogicType.AND,
          triggerCount: 1,
        },
        {
          currentState: 'MOVING_UP',
          nextState: 'STOPPED',
          triggers: [
            {
              triggerSource: 'P1',
              triggerType: TriggerType.LEVEL,
              operator: 0,
              triggerValue: 1,
            },
          ],
          triggerLogic: LogicType.AND,
          triggerCount: 1,
        },
      ],
      stateEffects: [],
      groupModes: [0, 0, 0, 0],
      inputModuleTypes: [0, 0, 0, 0],
      edgeTypes: [0, 0, 0, 0],
      outputModuleTypes: [0, 0, 0, 0],
    },
    metadata: {
      version: '1.0.0',
      author: 'system',
      createdAt: new Date().toISOString(),
      tags: ['elevator', 'basic'],
    },
  },
  {
    name: '红绿灯控制',
    description: '交通信号灯控制模板',
    category: '交通',
    config: {
      initialState: 'RED',
      transitions: [
        {
          currentState: 'RED',
          nextState: 'GREEN',
          triggers: [
            {
              triggerSource: 'P0',
              triggerType: TriggerType.LEVEL,
              operator: 0,
              triggerValue: 1,
            },
          ],
          triggerLogic: LogicType.AND,
          triggerCount: 1,
        },
        {
          currentState: 'GREEN',
          nextState: 'YELLOW',
          triggers: [
            {
              triggerSource: 'P0',
              triggerType: TriggerType.LEVEL,
              operator: 0,
              triggerValue: 0,
            },
          ],
          triggerLogic: LogicType.AND,
          triggerCount: 1,
        },
      ],
      stateEffects: [],
      groupModes: [0, 0, 0, 0],
      inputModuleTypes: [0, 0, 0, 0],
      edgeTypes: [0, 0, 0, 0],
      outputModuleTypes: [0, 0, 0, 0],
    },
    metadata: {
      version: '1.0.0',
      author: 'system',
      createdAt: new Date().toISOString(),
      tags: ['traffic', 'light'],
    },
  },
];

/**
 * 深拷贝工具函数
 */
const deepClone = <T>(obj: T): T => JSON.parse(JSON.stringify(obj));

/**
 * 模板管理组合式函数返回值
 */
export interface UseTemplatesReturn {
  /** 模板列表 */
  templates: Ref<StateMachineTemplate[]>;
  /** 所有分类 */
  categories: ComputedRef<string[]>;
  /** 获取所有模板 */
  getTemplates: () => StateMachineTemplate[];
  /** 获取指定模板 */
  getTemplate: (name: string) => StateMachineTemplate | undefined;
  /** 保存模板 */
  saveTemplate: (
    template: Omit<StateMachineTemplate, 'metadata'> & {
      metadata?: Partial<StateMachineTemplate['metadata']>;
    }
  ) => void;
  /** 删除模板 */
  deleteTemplate: (name: string) => boolean;
}

/**
 * 模板管理组合式函数
 */
export function useTemplates(): UseTemplatesReturn {
  const templates = ref<StateMachineTemplate[]>([]);

  const categories = computed(() => {
    const cats = new Set(templates.value.map((t) => t.category));
    return ['all', ...Array.from(cats).sort()];
  });

  const loadDefaultTemplates = () => {
    templates.value = deepClone(DEFAULT_TEMPLATES);
    saveToStorage();
  };

  const getTemplates = () => templates.value;

  const getTemplate = (name: string): StateMachineTemplate | undefined => {
    return templates.value.find((t) => t.name === name);
  };

  const saveTemplate = (
    template: Omit<StateMachineTemplate, 'metadata'> & {
      metadata?: Partial<StateMachineTemplate['metadata']>;
    }
  ) => {
    const fullTemplate: StateMachineTemplate = {
      ...template,
      metadata: {
        version: '1.0.0',
        author: 'user',
        createdAt: new Date().toISOString(),
        tags: ['custom'],
        ...template.metadata,
      },
    };

    const existingIndex = templates.value.findIndex((t) => t.name === fullTemplate.name);

    if (existingIndex !== -1) {
      templates.value[existingIndex] = fullTemplate;
    } else {
      templates.value.push(fullTemplate);
    }

    saveToStorage();
  };

  const deleteTemplate = (name: string): boolean => {
    const index = templates.value.findIndex((t) => t.name === name);
    if (index !== -1) {
      templates.value.splice(index, 1);
      saveToStorage();
      return true;
    }
    return false;
  };

  const saveToStorage = () => {
    setStorage('state_machine_templates', templates.value);
  };

  const loadFromStorage = () => {
    const savedTemplates = getStorage<StateMachineTemplate[]>('state_machine_templates');
    if (savedTemplates && savedTemplates.length > 0) {
      templates.value = savedTemplates;
    } else {
      loadDefaultTemplates();
    }
  };

  onMounted(() => {
    loadFromStorage();
  });

  return {
    templates,
    categories,
    getTemplates,
    getTemplate,
    saveTemplate,
    deleteTemplate,
  };
}
