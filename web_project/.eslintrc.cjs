/**
 * ESLint 配置文件
 * 
 * ESLint 是一个代码检查工具，用于发现代码中的错误和潜在问题。
 * 这个配置文件定义了项目的代码检查规则。
 * 
 * 配置说明:
 * - root: true 表示这是根配置文件，ESLint 不会向上查找其他配置
 * - env: 定义代码运行的环境，启用对应的全局变量
 * - extends: 继承的规则集，推荐使用官方配置
 * - parser: 解析器，用于解析 Vue 单文件组件
 * - parserOptions: 解析器选项
 * - plugins: 使用的插件
 * - rules: 自定义规则，覆盖或扩展继承的规则
 */
module.exports = {
  /** 标记为根配置文件 */
  root: true,
  
  /** 定义代码运行环境，启用对应的全局变量 */
  env: {
    node: true,      // Node.js 环境（如 require, module 等）
    browser: true,   // 浏览器环境（如 window, document 等）
    es2022: true     // ES2022 新特性（如 top-level await 等）
  },
  
  /** 继承的规则集 */
  extends: [
    'eslint:recommended',              // ESLint 官方推荐规则
    'plugin:@typescript-eslint/recommended',  // TypeScript 规则
    'plugin:vue/vue3-recommended',    // Vue 3 官方推荐规则
    'plugin:prettier/recommended'     // Prettier 规则（必须放在最后）
  ],
  
  /** 解析器：使用 vue-eslint-parser 解析 Vue 单文件组件 */
  parser: 'vue-eslint-parser',
  
  /** 解析器选项 */
  parserOptions: {
    /** 使用 TypeScript 解析器解析 <script> 标签中的代码 */
    parser: '@typescript-eslint/parser',
    /** ECMAScript 版本 */
    ecmaVersion: 2022,
    /** 源码类型：模块化（使用 import/export） */
    sourceType: 'module'
  },
  
  /** 使用的插件 */
  plugins: ['@typescript-eslint'],
  
  /** 自定义规则 */
  rules: {
    /** 关闭 Vue 组件必须使用多单词命名的规则 */
    'vue/multi-word-component-names': 'off',

    /** 未使用的变量警告（而不是错误） */
    '@typescript-eslint/no-unused-vars': ['warn', {
      /** 忽略以下划线 _ 开头的参数（表示有意不使用） */
      argsIgnorePattern: '^_',
      /** 忽略以下划线 _ 开头的变量 */
      varsIgnorePattern: '^_'
    }],

    /** any 类型警告（而不是错误） */
    '@typescript-eslint/no-explicit-any': 'warn',

    /** 不安全的函数类型警告 */
    '@typescript-eslint/no-unsafe-function-type': 'warn',

    /** 原型方法警告 */
    'no-prototype-builtins': 'warn',

    /** 允许 namespace */
    '@typescript-eslint/no-namespace': 'off',

    /** 允许空接口 */
    '@typescript-eslint/no-empty-object-type': 'off',

    /** console 警告（但允许 console.warn 和 console.error 用于调试） */
    'no-console': ['warn', { allow: ['warn', 'error'] }],

    /** debugger 警告（防止遗留调试代码） */
    'no-debugger': 'warn'
  }
}
