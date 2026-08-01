/**
 * TypeScript 环境类型声明文件
 *
 * 【功能说明】
 * 为项目提供全局类型声明，确保 TypeScript 编译器能够正确识别：
 * 1. Vite 客户端类型（热更新、环境变量等）
 * 2. .vue 文件的类型定义
 *
 * 【声明说明】
 * 1. /// <reference types="vite/client" />: 引入 Vite 提供的类型定义
 *    - ImportMeta: 环境变量类型
 *    - ViteHot: 热更新 API 类型
 *
 * 2. declare module '*.vue': 为 Vue 单文件组件提供类型声明
 *    - DefineComponent: Vue 3 组件类型
 *    - 允许直接 import 导入 .vue 文件
 *
 * @module env.d.ts
 */

/// <reference types="vite/client" />

declare module '*.vue' {
  import type { DefineComponent } from 'vue';
  const component: DefineComponent<{}, {}, any>;
  export default component;
}
