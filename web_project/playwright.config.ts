/**
 * Playwright E2E 测试配置
 *
 * 【功能说明】
 * 配置 Playwright 测试运行器
 * 用于端到端测试，验证页面功能是否正常
 */
import { defineConfig, devices } from '@playwright/test';

export default defineConfig({
  testDir: './e2e',
  fullyParallel: true,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 2 : 0,
  workers: process.env.CI ? 1 : undefined,
  reporter: 'html',

  use: {
    baseURL: 'http://localhost:5173',
    trace: 'on-first-retry',
    channel: 'msedge', // 使用系统 Edge 浏览器
  },

  projects: [
    {
      name: 'msedge',
      use: { ...devices['Desktop Edge'] },
    },
  ],

  webServer: {
    command: 'npm run dev',
    url: 'http://localhost:5173',
    reuseExistingServer: !process.env.CI,
  },
});
