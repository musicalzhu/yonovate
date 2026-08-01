/**
 * Vite 构建配置文件
 *
 * 本配置针对 Vue 3.5+ 项目优化，包含：
 * - 代码分割策略（避免循环依赖）
 * - 开发服务器代理配置
 * - 生产构建优化
 * - 依赖预构建优化
 */
import { defineConfig } from 'vite';
import vue from '@vitejs/plugin-vue';
import { fileURLToPath, URL } from 'node:url';

export default defineConfig({
  /* ========== 插件配置 ========== */
  plugins: [vue()],

  /* ========== 路径别名配置 ========== */
  resolve: {
    alias: {
      // 主源码目录别名
      '@': fileURLToPath(new URL('./src', import.meta.url)),
    },
  },

  /* ========== 开发服务器配置 ========== */
  server: {
    // 开发服务器端口
    port: 5173,
    // API 代理配置 - 将 /api 请求转发到后端设备
    // 注意：开发环境中如果启用了 MSW Mock 服务，代理会被禁用
    // 取消注释下方配置以启用真实后端代理
    /*
    proxy: {
      '/api': {
        // 后端设备地址（ESP32）
        target: 'http://192.168.3.55',
        // 修改请求头中的 Origin，避免跨域问题
        changeOrigin: true,
        // 重写路径：移除 /api 前缀
        rewrite: (path) => path.replace(/^\/api/, '')
      }
    }
    */
  },

  /* ========== 生产构建配置 ========== */
  build: {
    // 输出目录
    outDir: 'dist-vue',
    // 构建前清空输出目录（默认值，可省略）
    emptyOutDir: true,
    // 构建目标版本
    target: 'ES2022',

    // Rollup 打包配置
    rollupOptions: {
      // 入口文件配置
      input: {
        main: fileURLToPath(new URL('./index.html', import.meta.url)),
      },

      // 输出配置
      output: {
        /**
         * 代码分割策略 - 使用函数写法
         *
         * 优势：
         * 1. 避免 Rollup 循环依赖报错
         * 2. 正确匹配绝对路径（Rollup 传入的是绝对路径）
         * 3. 自动处理依赖关系
         *
         * 注意：
         * - 函数必须返回 string 或 undefined
         * - 返回 undefined 表示不分割，保持默认行为
         * - 使用 id.includes() 进行路径匹配，兼容不同操作系统
         * - 匹配顺序很重要：先匹配具体模块，再匹配通用目录
         */
        manualChunks(id) {
          // ========== 第三方依赖分割 ==========
          if (id.includes('node_modules')) {
            // Vue 核心生态（vue、pinia、vue-router）
            if (id.includes('vue') || id.includes('pinia') || id.includes('vue-router')) {
              return 'vue-vendor';
            }
            // Element Plus UI 组件库
            if (id.includes('element-plus')) {
              return 'ui-vendor';
            }
            // 工具库（axios、dayjs）
            if (id.includes('axios') || id.includes('dayjs')) {
              return 'utils-vendor';
            }
            // 其他第三方库统一打包
            return 'vendor';
          }

          // ========== 业务代码分割 ==========
          // 注意：匹配顺序很重要，先匹配具体模块，再匹配通用目录

          // 状态机相关模块（优先匹配，避免被通用规则覆盖）
          if (
            id.includes('/src/types/stateMachine') ||
            id.includes('/src/services/stateMachine') ||
            id.includes('/src/composables/useStateMachine') ||
            id.includes('/src/components/state-machine') ||
            id.includes('/src/stores/stateMachine')
          ) {
            return 'state-machine';
          }

          // 设备控制相关模块（优先匹配）
          if (
            id.includes('/src/types/device') ||
            id.includes('/src/services/api/device') ||
            id.includes('/src/composables/useDeviceConnection') ||
            id.includes('/src/components/device') ||
            id.includes('/src/views/DeviceControlPage')
          ) {
            return 'device-control';
          }

          // 组合式函数（通用匹配，在具体模块之后）
          if (id.includes('/src/composables/')) {
            return 'composables';
          }

          // 服务层（通用匹配）
          if (id.includes('/src/services/')) {
            return 'services';
          }

          // Store 状态管理
          if (id.includes('/src/stores/')) {
            return 'stores';
          }

          // 类型定义文件
          if (id.includes('/src/types/')) {
            return 'types';
          }

          // 未匹配的模块保持默认行为（不分割）
          return undefined;
        },

        // 文件命名策略
        chunkFileNames: 'assets/js/[name]-[hash].js',
        entryFileNames: 'assets/js/[name]-[hash].js',
        assetFileNames: 'assets/[ext]/[name]-[hash].[ext]',
      },
    },

    // ========== 资源优化配置 ==========
    // 小于 4KB 的资源内联为 base64（默认值 4096）
    assetsInlineLimit: 4096,
    // chunk 大小警告阈值（单位：KB）
    chunkSizeWarningLimit: 1000,
    // CSS 代码分割（默认值 true）
    cssCodeSplit: true,
    // 不生成 sourcemap（生产环境）
    sourcemap: false,
  },

  /* ========== 依赖预构建配置 ========== */
  optimizeDeps: {
    // 强制预构建的依赖
    // 这些依赖会被 Vite 预先打包，加快开发时的加载速度
    include: ['vue', 'pinia', 'vue-router', 'element-plus', 'axios'],
  },

  /* ========== esbuild 配置 ========== */
  esbuild: {
    // 构建目标
    target: 'es2022',
    // 生产环境移除 console 和 debugger 语句
    drop: ['console', 'debugger'],
  },

  /* ========== 环境变量配置 ========== */
  // 环境变量前缀（默认值 'VITE_'）
  envPrefix: 'VITE_',

  /* ========== 日志配置 ========== */
  logLevel: 'info',
});
