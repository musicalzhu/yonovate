/**
 * Vitest 测试配置文件
 */
import { defineConfig } from 'vitest/config';
import vue from '@vitejs/plugin-vue';
import { fileURLToPath, URL } from 'node:url';

export default defineConfig({
  plugins: [vue()],
  test: {
    globals: true,
    environment: 'jsdom',
    setupFiles: ['./tests/setup/vitest.setup.ts'],
    exclude: ['**/node_modules/**', '**/e2e/**', '**/*.d.ts', '**/*.config.*'],
    coverage: {
      provider: 'v8',
      reporter: ['text', 'json', 'html'],
      exclude: ['**/node_modules/**', '**/e2e/**', '**/*.d.ts', '**/*.config.*'],
      thresholds: {
        global: {
          branches: 85,
          functions: 95,
          lines: 90,
          statements: 90,
        },
      },
    },
  },
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
    },
  },
});
