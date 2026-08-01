/**
 * Mock API 数据和处理器
 * 用于 Vitest 测试中模拟 ESP32 设备 API 响应
 */
import { http, HttpHandler, HttpResponse, delay } from 'msw';

// 模拟数据
export const mockDeviceInfo = {
  deviceId: 'esp32-001',
  name: 'ESP32-DevKit',
  type: 'ESP32-WROOM',
  status: 'online',
  lastSeen: new Date().toISOString(),
  firmware: 'v1.2.3',
  ipAddress: '192.168.3.55',
  macAddress: 'AA:BB:CC:DD:EE:FF',
  version: '3.0.0',
  capabilities: ['gpio', 'wifi', 'bluetooth'],
};

export const mockDeviceStatus = {
  status: 'running',
  uptime: 3600,
};

export const mockStateMachineConfig = {
  initialState: 'IDLE',
  transitions: [
    {
      currentState: 'IDLE',
      nextState: 'MOVING_UP',
      triggers: [
        {
          triggerType: 2,
          triggerSource: 'P0',
          operator: 0,
          triggerValue: 1,
        },
      ],
      triggerCount: 1,
      triggerLogic: 0,
    },
  ],
  stateEffects: [],
  groupModes: [0, 0, 0, 0],
  inputModuleTypes: [0, 0, 0, 0],
  edgeTypes: [0, 0, 0, 0],
  outputModuleTypes: [0, 0, 0, 0],
};

export const mockStateMachineStatus = {
  currentState: 'IDLE',
  lastTriggerTime: Date.now() - 5000,
  triggerCount: 15,
  errorCount: 0,
  uptime: 3600,
};

export const mockGPIOStatus = [
  { pin: 0, mode: 'output', value: 0, lastUpdated: new Date().toISOString() },
  { pin: 1, mode: 'output', value: 1, lastUpdated: new Date().toISOString() },
  { pin: 2, mode: 'input', value: 0, lastUpdated: new Date().toISOString() },
  { pin: 3, mode: 'output', value: 0, lastUpdated: new Date().toISOString() },
  { pin: 4, mode: 'input_pullup', value: 1, lastUpdated: new Date().toISOString() },
  { pin: 5, mode: 'output', value: 0, lastUpdated: new Date().toISOString() },
];

export const mockWiFiConfig = {
  ssid: 'TestNetwork',
  password: '',
  ip: '192.168.3.100',
  gateway: '192.168.3.1',
  subnet: '255.255.255.0',
  dns: '8.8.8.8',
  dhcp: false,
};

export const mockWiFiStatus = {
  connected: true,
  ssid: 'TestNetwork',
  ip: '192.168.3.100',
  gateway: '192.168.3.1',
  signalStrength: -45,
  lastConnected: new Date().toISOString(),
};

export const mockWiFiNetworks = [
  { ssid: 'Network1', rssi: -45, encryption: 'WPA2' },
  { ssid: 'Network2', rssi: -65, encryption: 'WPA3' },
  { ssid: 'Network3', rssi: -80, encryption: 'OPEN' },
];

// 创建所有 API mock 处理器
export const createMockHandlers = (): HttpHandler[] => [
  // 设备信息
  http.get('/api/v3/device/info', async () => {
    await delay(100);
    return HttpResponse.json(mockDeviceInfo);
  }),

  // 设备状态
  http.get('/api/v3/device/status', async () => {
    await delay(100);
    return HttpResponse.json(mockDeviceStatus);
  }),

  // 设备重启
  http.post('/api/v3/device/reboot', async () => {
    await delay(200);
    return HttpResponse.json({ success: true });
  }),

  // 设备恢复出厂设置
  http.post('/api/v3/device/factory-reset', async () => {
    await delay(300);
    return HttpResponse.json({ success: true });
  }),

  // 状态机配置获取
  http.get('/api/v3/state-machine/config', async () => {
    await delay(100);
    return HttpResponse.json(mockStateMachineConfig);
  }),

  // 状态机配置保存
  http.post('/api/v3/state-machine/config', async ({ request }) => {
    await delay(150);
    const body = await request.json();
    return HttpResponse.json({ success: true, data: body });
  }),

  // 状态机状态
  http.get('/api/v3/state-machine/status', async () => {
    await delay(100);
    return HttpResponse.json(mockStateMachineStatus);
  }),

  // 状态机重置
  http.post('/api/v3/state-machine/reset', async () => {
    await delay(200);
    return HttpResponse.json({ success: true });
  }),

  // GPIO 状态获取 - 正确路径
  http.get('/api/v3/device/gpio', async () => {
    await delay(100);
    return HttpResponse.json(mockGPIOStatus);
  }),

  // GPIO 值设置
  http.post('/api/v3/device/gpio', async ({ request }) => {
    await delay(50);
    const body = (await request.json()) as { pin: number; value: number };
    return HttpResponse.json({ success: true, pin: body.pin, value: body.value });
  }),

  // GPIO 配置更新
  http.put('/api/v3/device/gpio/:pin', async ({ params }) => {
    await delay(100);
    const pin = params.pin;
    return HttpResponse.json({ success: true, pin });
  }),

  // WiFi 配置获取 - 正确路径
  http.get('/api/v3/device/wifi/config', async () => {
    await delay(100);
    return HttpResponse.json(mockWiFiConfig);
  }),

  // WiFi 配置保存 - 正确路径
  http.post('/api/v3/device/wifi/config', async ({ request }) => {
    await delay(150);
    const body = await request.json();
    return HttpResponse.json({ success: true, data: body });
  }),

  // WiFi 状态获取 - 正确路径
  http.get('/api/v3/device/wifi/status', async () => {
    await delay(100);
    return HttpResponse.json(mockWiFiStatus);
  }),

  // WiFi 网络扫描 - 正确路径
  http.get('/api/v3/device/wifi/scan', async () => {
    await delay(500);
    return HttpResponse.json(mockWiFiNetworks);
  }),
];

// 默认导出的处理器
export const handlers = createMockHandlers();
