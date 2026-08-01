/**
 * MSW Server Setup (for Node.js Testing)
 *
 * 【功能说明】
 * 为 Node.js 测试环境配置 MSW
 * 在测试文件中使用
 *
 * @module server
 */
import { setupServer } from 'msw/node';
import { handlers } from './handlers';

// 创建 MSW 服务器实例
export const server = setupServer(...handlers);
