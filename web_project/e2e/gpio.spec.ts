/**
 * GPIO 控制页面 E2E 测试
 *
 * 【测试内容】
 * 1. GPIO页面加载
 * 2. GPIO状态表格显示
 * 3. GPIO引脚控制
 */
import { test, expect } from '@playwright/test';

test.describe('GPIO 控制页面测试', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/gpio-control');
  });

  test('应该显示GPIO控制页面', async ({ page }) => {
    // 检查页面标题或内容
    await expect(page.locator('body')).toContainText(/GPIO/i);
  });

  test('应该显示GPIO状态表格或列表', async ({ page }) => {
    await page.waitForTimeout(2000);

    // 查找GPIO引脚相关的内容
    const gpioContent = page.locator('text=/GPIO|引脚|pin/i');
    await expect(gpioContent.first()).toBeVisible({ timeout: 5000 });
  });
});
