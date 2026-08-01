import type { DeviceInfo, DeviceStatus, DeviceMetrics } from '@/generated/api';

export const mockDeviceInfo: DeviceInfo = {
  deviceId: 'esp32-dev-001',
  name: 'ESP32 State Machine',
  type: 'esp32',
  status: 'online',
  lastSeen: new Date().toISOString(),
  firmware: 'v3.0.0',
  ipAddress: '192.168.3.55',
  macAddress: 'AA:BB:CC:DD:EE:FF',
  version: '3.0.0',
  capabilities: ['gpio', 'wifi', 'state-machine'],
};

export const mockDeviceStatus: DeviceStatus = {
  status: 'running',
  uptime: 3600,
};

export const mockDeviceMetrics: DeviceMetrics = {
  deviceId: 'esp32-dev-001',
  timestamp: new Date().toISOString(),
  cpuUsage: 15.5,
  memoryUsage: 45.2,
  temperature: 42.3,
  uptime: 3600,
  errorCount: 0,
  lastReset: new Date(Date.now() - 86400000).toISOString(),
};
