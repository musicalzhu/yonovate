/**
 * WiFi 信号强度相关的公共工具函数。
 *
 * 统一处理 RSSI 数值到 UI 展示状态的映射，避免页面之间重复实现
 * 相同的信号强度判断逻辑。
 */

export type WifiSignalTagType = 'success' | 'warning' | 'danger' | 'info';

/**
 * 根据 WiFi 信号强度返回对应的 Element Plus 标签样式。
 *
 * @param rssi - WiFi 信号强度，单位为 dBm
 * @returns 对应的标签类型
 */
export function getSignalTagType(rssi?: number): WifiSignalTagType {
  if (typeof rssi !== 'number' || Number.isNaN(rssi)) {
    return 'info';
  }

  if (rssi >= -50) {
    return 'success';
  }

  if (rssi >= -70) {
    return 'warning';
  }

  return 'danger';
}

/**
 * 将 WiFi 信号强度格式化为可直接展示的文本。
 *
 * @param rssi - WiFi 信号强度，单位为 dBm
 * @returns 格式化后的展示文本
 */
export function formatSignalStrength(rssi?: number): string {
  if (typeof rssi !== 'number' || Number.isNaN(rssi)) {
    return '未知';
  }

  return `${rssi} dBm`;
}
