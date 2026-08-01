/**
 * MSW Browser Worker Setup
 *
 * 【功能说明】
 * 为浏览器环境配置 MSW Service Worker
 * 在 main.ts 中初始化
 *
 * @module browser
 */
import { setupWorker } from 'msw/browser';
import { handlers } from './handlers';

export const worker = setupWorker(...handlers);
