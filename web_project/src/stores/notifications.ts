/**
 * Vue 3.5+ 通知管理 Store (Pinia Setup 语法)
 *
 * 管理应用的全局通知系统
 */
import { defineStore } from 'pinia';
import { ref, computed } from 'vue';
import type { Notification } from '@/types';
import { errorHandlerService } from '@/services/notification/errorHandler';

/**
 * 通知管理 Store
 */
export const useNotificationsStore = defineStore('notifications', () => {
  // 状态
  const notifications = ref<Notification[]>([]);

  // 计算属性
  /** 未读通知数量 */
  const unreadNotifications = computed(() => notifications.value.filter((n) => !n.read).length);

  // 方法
  /**
   * 统一生成通知对象的 ID、时间戳和默认已读状态。
   */
  const buildNotification = (notification: Omit<Notification, 'id'>) => ({
    ...notification,
    id: `notification_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`,
    timestamp: notification.timestamp ?? Date.now(),
    read: notification.read ?? false,
  });

  /**
   * 添加通知
   * @param notification - 通知对象（不包含 id）
   */
  const addNotification = (notification: Omit<Notification, 'id'>) => {
    const newNotification = buildNotification(notification);

    notifications.value.unshift(newNotification);

    // 限制通知数量
    if (notifications.value.length > 10) {
      notifications.value = notifications.value.slice(0, 10);
    }

    // 显示通知
    errorHandlerService.showNotification(newNotification);
  };

  /**
   * 移除通知
   * @param id - 通知 ID
   */
  const removeNotification = (id: string) => {
    const index = notifications.value.findIndex((n) => n.id === id);
    if (index > -1) {
      notifications.value.splice(index, 1);
    }
  };

  /**
   * 清除所有通知
   */
  const clearNotifications = () => {
    notifications.value = [];
  };

  /**
   * 标记通知为已读
   * @param id - 通知 ID
   */
  const markNotificationAsRead = (id: string) => {
    const notification = notifications.value.find((n) => n.id === id);
    if (notification) {
      notification.read = true;
    }
  };

  /**
   * 标记所有通知为已读
   */
  const markAllAsRead = () => {
    notifications.value.forEach((n) => {
      n.read = true;
    });
  };

  /**
   * 显示成功通知
   * @param message - 消息内容
   * @param title - 标题（可选）
   */
  const showSuccess = (message: string, title?: string) => {
    addNotification({
      type: 'success',
      title: title || '成功',
      message,
    });
  };

  /**
   * 显示错误通知
   * @param message - 消息内容
   * @param title - 标题（可选）
   */
  const showError = (message: string, title?: string) => {
    addNotification({
      type: 'error',
      title: title || '错误',
      message,
    });
  };

  /**
   * 显示警告通知
   * @param message - 消息内容
   * @param title - 标题（可选）
   */
  const showWarning = (message: string, title?: string) => {
    addNotification({
      type: 'warning',
      title: title || '警告',
      message,
    });
  };

  /**
   * 显示信息通知
   * @param message - 消息内容
   * @param title - 标题（可选）
   */
  const showInfo = (message: string, title?: string) => {
    addNotification({
      type: 'info',
      title: title || '信息',
      message,
    });
  };

  // 返回 store 接口
  return {
    // 状态
    notifications,

    // 计算属性
    unreadNotifications,

    // 方法
    addNotification,
    removeNotification,
    clearNotifications,
    markNotificationAsRead,
    markAllAsRead,
    showSuccess,
    showError,
    showWarning,
    showInfo,
  };
});
