import { AxiosError } from 'axios';
import type { Notification } from '../../types/common';
import { ElMessage, ElNotification } from 'element-plus';

export interface ErrorHandlerOptions {
  showToast?: boolean;
  logError?: boolean;
}

export interface ApiError {
  status: number;
  message: string;
  code?: string;
  details?: unknown;
}

export function isApiError(error: unknown): error is ApiError {
  return error !== null && typeof error === 'object' && 'status' in error && 'message' in error;
}

export function getErrorMessage(error: unknown): string {
  if (isApiError(error)) {
    return error.message;
  }
  if (error instanceof Error) {
    return error.message;
  }
  return String(error);
}

function getDefaultMessage(status: number): string {
  return (
    {
      400: '请求参数错误',
      401: '未授权，请重新登录',
      403: '禁止访问',
      404: '资源未找到',
      500: '服务器内部错误',
      502: '网关错误',
      503: '服务不可用',
      504: '网关超时',
    }[status] || '请求失败'
  );
}

export interface ErrorHandlerService {
  handleError(error: unknown, context?: string): void;
  handleApiError(error: any, context?: string): void;
  handleValidationError(error: any, context?: string): void;
  showNotification(notification: Notification): void;
  showError(message: string, duration?: number): void;
  showSuccess(message: string, duration?: number): void;
  showWarning(message: string, duration?: number): void;
  showInfo(message: string, duration?: number): void;
}

export class ErrorHandlerServiceImpl implements ErrorHandlerService {
  private readonly defaultDuration = 3000;
  private readonly errorDuration = 5000;
  private notificationQueue: Notification[] = [];
  private maxNotifications = 5;

  handleError(error: unknown, context = '未知错误'): void {
    console.error(`[${context}]`, error);

    let message = '发生未知错误';

    if (error instanceof Error) {
      message = error.message || error.name || '发生错误';
    } else if (typeof error === 'string') {
      message = error;
    } else if (error && typeof error === 'object') {
      message = (error as any).message || (error as any).error || '发生错误';
    }

    this.showError(`${context}: ${message}`);
  }

  handleApiError(error: any, context = 'API请求'): void {
    console.error(`[${context}]`, error);

    let message = '网络请求失败';

    if (error.response) {
      const status = error.response.status;
      const data = error.response.data;

      switch (status) {
        case 400:
          message = data?.message || '请求参数错误';
          break;
        case 401:
          message = '未授权，请重新登录';
          break;
        case 403:
          message = '权限不足';
          break;
        case 404:
          message = '请求的资源不存在';
          break;
        case 422:
          message = data?.message || '数据验证失败';
          break;
        case 500:
          message = '服务器内部错误';
          break;
        case 502:
          message = '网关错误';
          break;
        case 503:
          message = '服务不可用';
          break;
        case 504:
          message = '请求超时';
          break;
        default:
          message = data?.message || `请求失败 (${status})`;
      }
    } else if (error.request) {
      message = '网络连接失败，请检查网络设置';
    } else if (error.message) {
      message = error.message;
    }

    this.showError(`${context}: ${message}`);
  }

  handleValidationError(error: any, context = '数据验证'): void {
    console.error(`[${context}]`, error);

    let message = '数据验证失败';

    if (error.errors && Array.isArray(error.errors)) {
      message = error.errors.join('; ');
    } else if (error.message) {
      message = error.message;
    }

    this.showError(`${context}: ${message}`);
  }

  showNotification(notification: Notification): void {
    this.notificationQueue.push(notification);

    if (this.notificationQueue.length > this.maxNotifications) {
      this.notificationQueue.shift();
    }

    ElNotification({
      type: notification.type,
      title: notification.title,
      message: notification.message,
      duration: notification.duration || this.defaultDuration,
      position: 'top-right',
      showClose: true,
      onClose: () => {
        this.removeNotification(notification.id);
      },
    });
  }

  private showMessage(
    type: 'success' | 'warning' | 'info' | 'error',
    message: string,
    duration: number
  ): void {
    ElMessage({
      type,
      message,
      duration,
      showClose: true,
    });
  }

  showError(message: string, duration = this.errorDuration): void {
    this.showMessage('error', message, duration);
  }

  showSuccess(message: string, duration = this.defaultDuration): void {
    this.showMessage('success', message, duration);
  }

  showWarning(message: string, duration = this.defaultDuration): void {
    this.showMessage('warning', message, duration);
  }

  showInfo(message: string, duration = this.defaultDuration): void {
    this.showMessage('info', message, duration);
  }

  private removeNotification(id: string): void {
    const index = this.notificationQueue.findIndex((n) => n.id === id);
    if (index > -1) {
      this.notificationQueue.splice(index, 1);
    }
  }

  clearAllNotifications(): void {
    this.notificationQueue = [];
    ElNotification.closeAll();
  }

  getNotifications(): Notification[] {
    return [...this.notificationQueue];
  }

  private generateNotificationId(): string {
    return `notification_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  createNotification(
    type: Notification['type'],
    title: string,
    message?: string,
    duration?: number
  ): Notification {
    return {
      id: this.generateNotificationId(),
      type,
      title,
      message: message || '',
      timestamp: Date.now(),
      duration,
    };
  }
}

export const errorHandlerService = new ErrorHandlerServiceImpl();

export function handleError(error: unknown, options: ErrorHandlerOptions = {}): ApiError {
  const { showToast = true, logError = true } = options;

  let apiError: ApiError;

  if (error instanceof AxiosError) {
    const status = error.response?.status ?? 0;
    const message =
      error.response?.data?.message || error.response?.data?.error || error.message || '请求失败';

    apiError = {
      status,
      message,
      code: error.response?.data?.code,
      details: error.response?.data,
    };
  } else if (error instanceof Error) {
    apiError = {
      status: -1,
      message: error.message,
    };
  } else {
    apiError = {
      status: -1,
      message: String(error) || '未知错误',
    };
  }

  if (logError) {
    console.error('[API Error]', apiError);
  }

  if (showToast) {
    const message = getDefaultMessage(apiError.status) || apiError.message;
    try {
      const ElMessage = (window as any).$message || require('element-plus').ElMessage;
      ElMessage.error(message);
    } catch {
      console.warn('ElMessage not available, using alert');
      alert(message);
    }
  }

  return apiError;
}