/**
 * WiFi 配置页面 E2E 测试
 *
 * 【测试内容】
 * 1. WiFi页面加载
 * 2. WiFi状态显示
 * 3. WiFi配置表单
 */
import { test, expect } from '@playwright/test';

test.describe('WiFi 配置页面测试', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/wifi-config');
  });

  test('应该显示WiFi配置页面', async ({ page }) => {
    // 检查页面内容
    await expect(page.locator('body')).toContainText(/WiFi|无线/i);
  });

  test('应该显示WiFi状态区域', async ({ page }) => {
    await page.waitForTimeout(2000);

    // 查找WiFi状态相关内容
    const wifiStatus = page.locator('text=/WiFi|SSID|信号/i');
    await expect(wifiStatus.first()).toBeVisible({ timeout: 5000 });
  });
});
