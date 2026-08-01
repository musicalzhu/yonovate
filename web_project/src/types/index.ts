/**
 * 统一导出项目内所有类型定义，便于各层模块通过单一入口消费。
 */
export * from './generated/api';
export * from './domain/stateMachine';
export * from './domain/device';
export * from './common/common';
export * from './common/exportable';

declare global {
  interface Window {
    __VUE_OPTIONS_API__: boolean;
    __VUE_PROD_DEVTOOLS__: boolean;
    __VUE_PROD_HYDRATION_MISMATCH_DETAILS__: boolean;
    __APP_CONFIG__: import('./common/common').AppConfig;
    __VUE_DEVTOOLS_GLOBAL_HOOK__?: any;
    $message?: typeof import('element-plus').ElMessage;
  }

  namespace NodeJS {
    interface ProcessEnv {
      NODE_ENV: 'development' | 'production' | 'test';
      VITE_API_BASE_URL?: string;
      VITE_APP_TITLE?: string;
      VITE_DEBUG?: string;
    }
  }
}

declare module 'vitest' {
  export interface TestContext {}
}

declare module '@element-plus/icons-vue' {}

declare module '@vueuse/core' {}
