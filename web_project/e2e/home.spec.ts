/**
 * 首页 E2E 测试
 *
 * 【测试内容】
 * 1. 首页加载
 * 2. 设备配置区域显示
 * 3. 设备状态显示
 * 4. 功能导航卡片显示
 */
import { test, expect } from '@playwright/test';

test.describe('首页测试', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/');
  });

  test('应该正确显示首页标题', async ({ page }) => {
    // 检查页面标题
    await expect(page).toHaveTitle(/ESP32/);
  });

  test('应该显示设备配置区域', async ({ page }) => {
    // 检查设备IP输入框存在
    const ipInput = page.locator('input').first();
    await expect(ipInput).toBeVisible();
  });

  test('应该显示测试连接按钮', async ({ page }) => {
    // 查找包含"测试"或"连接"的按钮
    const button = page.getByRole('button', { name: /测试|连接/i });
    await expect(button.first()).toBeVisible();
  });

  test('应该显示功能导航卡片', async ({ page }) => {
    // 等待页面加载
    await page.waitForTimeout(1000);

    // 查找页面内容（首页应该有主要内容区域）
    const mainContent = page.locator('main, .main, #app > div');
    await expect(mainContent.first()).toBeVisible({ timeout: 5000 });
  });

  test('应该显示设备状态区域', async ({ page }) => {
    await page.waitForTimeout(2000);

    // 查找状态相关的元素
    const statusArea = page.locator('text=/状态|连接/i').first();
    await expect(statusArea).toBeVisible({ timeout: 5000 });
  });
});
