import type { DeviceCommand } from '@/generated/api';

export const mockCommands: DeviceCommand[] = [
  {
    id: 'cmd-001',
    deviceId: 'esp32-dev-001',
    type: 'reboot',
    parameters: {},
    status: 'completed',
    createdAt: new Date(Date.now() - 3600000).toISOString(),
    executedAt: new Date(Date.now() - 3500000).toISOString(),
    result: { success: true },
    error: undefined,
  },
];
