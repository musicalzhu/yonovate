/**
 * Vue 3.5+ 设备类型定义
 * 定义设备相关的枚举、接口和类型守卫函数
 */

// ============ 设备信息接口 ============
/**
 * 设备信息接口
 * 定义设备的基本信息和状态
 */
export interface DeviceInfo {
  /** 设备 ID */
  id: string;
  /** 设备名称 */
  name: string;
  /** 设备类型 */
  type: DeviceType;
  /** 设备状态 */
  status: DeviceStatus;
  /** 最后在线时间 */
  lastSeen: string;
  /** 固件版本 */
  firmware: string;
  /** IP 地址 */
  ipAddress: string;
  /** MAC 地址（可选） */
  macAddress?: string;
  /** 版本号 */
  version: string;
  /** 设备能力列表 */
  capabilities: DeviceCapability[];
}

// ============ 设备类型枚举 ============
/**
 * 设备类型枚举
 * 定义支持的设备类型
 */
export enum DeviceType {
  /** ESP32 */
  ESP32 = 'esp32',
  /** ESP8266 */
  ESP8266 = 'esp8266',
  /** Arduino */
  ARDUINO = 'arduino',
  /** Raspberry Pi */
  RASPBERRY_PI = 'raspberry_pi',
}

// ============ 设备状态枚举 ============
/**
 * 设备状态枚举
 * 定义设备的运行状态
 */
export enum DeviceStatus {
  /** 在线 */
  ONLINE = 'online',
  /** 离线 */
  OFFLINE = 'offline',
  /** 错误 */
  ERROR = 'error',
  /** 更新中 */
  UPDATING = 'updating',
  /** 重启中 */
  REBOOTING = 'rebooting',
}

// ============ 设备能力枚举 ============
/**
 * 设备能力枚举
 * 定义设备支持的功能
 */
export enum DeviceCapability {
  /** GPIO 控制 */
  GPIO = 'gpio',
  /** WiFi */
  WIFI = 'wifi',
  /** 蓝牙 */
  BLUETOOTH = 'bluetooth',
  /** 状态机 */
  STATE_MACHINE = 'state_machine',
  /** 逻辑电路 */
  LOGIC_CIRCUIT = 'logic_circuit',
}

// ============ GPIO 状态接口 ============
/**
 * GPIO 状态接口
 * 定义 GPIO 引脚的状态
 */
export interface GPIOStatus {
  /** 引脚编号 */
  pin: number;
  /** 引脚模式 */
  mode: GPIOMode;
  /** 引脚值 */
  value: number;
  /** 引脚标签（可选） */
  label?: string;
  /** 引脚描述（可选） */
  description?: string;
  /** 最后更新时间 */
  lastUpdated: string;
}

// ============ GPIO 模式枚举 ============
/**
 * GPIO 模式枚举
 * 定义 GPIO 引脚的工作模式
 */
export enum GPIOMode {
  /** 输入模式 */
  INPUT = 'input',
  /** 输出模式 */
  OUTPUT = 'output',
  /** 上拉输入模式 */
  INPUT_PULLUP = 'input_pullup',
  /** 下拉输入模式 */
  INPUT_PULLDOWN = 'input_pulldown',
}

// ============ WiFi 配置接口 ============
/**
 * WiFi 配置接口
 * 定义 WiFi 连接配置
 */
export interface WiFiConfig {
  /** WiFi 名称 */
  ssid: string;
  /** WiFi 密码（可选） */
  password?: string;
  /** IP 地址（可选） */
  ip?: string;
  /** 网关（可选） */
  gateway?: string;
  /** 子网掩码（可选） */
  subnet?: string;
  /** DNS（可选） */
  dns?: string;
  /** 是否使用 DHCP（可选） */
  dhcp?: boolean;
}

// ============ WiFi 状态接口 ============
/**
 * WiFi 状态接口
 * 定义 WiFi 连接状态
 */
export interface WiFiStatus {
  /** 是否已连接 */
  connected: boolean;
  /** WiFi 名称（可选） */
  ssid?: string;
  /** IP 地址（可选） */
  ip?: string;
  /** 网关（可选） */
  gateway?: string;
  /** 子网掩码（可选） */
  subnet?: string;
  /** 信号强度（可选） */
  signalStrength?: number;
  /** 最后连接时间（可选） */
  lastConnected?: string;
}

// ============ 逻辑电路配置接口 ============
/**
 * 逻辑电路配置接口
 * 定义逻辑电路的完整配置
 */
export interface LogicCircuitConfig {
  /** 电路 ID */
  id: string;
  /** 电路名称 */
  name: string;
  /** 输入列表 */
  inputs: LogicInput[];
  /** 输出列表 */
  outputs: LogicOutput[];
  /** 逻辑门列表 */
  gates: LogicGate[];
  /** 连接列表 */
  connections: LogicConnection[];
}

// ============ 逻辑输入接口 ============
/**
 * 逻辑输入接口
 * 定义逻辑电路的输入
 */
export interface LogicInput {
  /** 输入 ID */
  id: string;
  /** 输入名称 */
  name: string;
  /** 输入类型 */
  type: LogicInputType;
  /** GPIO 引脚编号（可选） */
  gpioPin?: number;
  /** 默认值 */
  defaultValue: boolean;
}

// ============ 逻辑输入类型枚举 ============
/**
 * 逻辑输入类型枚举
 * 定义逻辑输入的来源类型
 */
export enum LogicInputType {
  /** GPIO 输入 */
  GPIO = 'gpio',
  /** 开关 */
  SWITCH = 'switch',
  /** 按钮 */
  BUTTON = 'button',
  /** 传感器 */
  SENSOR = 'sensor',
}

// ============ 逻辑输出接口 ============
/**
 * 逻辑输出接口
 * 定义逻辑电路的输出
 */
export interface LogicOutput {
  /** 输出 ID */
  id: string;
  /** 输出名称 */
  name: string;
  /** 输出类型 */
  type: LogicOutputType;
  /** GPIO 引脚编号（可选） */
  gpioPin?: number;
  /** 是否反转 */
  inverted: boolean;
}

// ============ 逻辑输出类型枚举 ============
/**
 * 逻辑输出类型枚举
 * 定义逻辑输出的目标类型
 */
export enum LogicOutputType {
  /** GPIO 输出 */
  GPIO = 'gpio',
  /** LED */
  LED = 'led',
  /** 继电器 */
  RELAY = 'relay',
  /** 蜂鸣器 */
  BUZZER = 'buzzer',
}

// ============ 逻辑门接口 ============
/**
 * 逻辑门接口
 * 定义逻辑电路中的逻辑门
 */
export interface LogicGate {
  /** 逻辑门 ID */
  id: string;
  /** 逻辑门类型 */
  type: LogicGateType;
  /** 输入 ID 列表 */
  inputs: string[];
  /** 输出 ID */
  output: string;
  /** 延迟（毫秒，可选） */
  delay?: number;
}

// ============ 逻辑门类型枚举 ============
/**
 * 逻辑门类型枚举
 * 定义支持的逻辑门类型
 */
export enum LogicGateType {
  /** 与门 */
  AND = 'and',
  /** 或门 */
  OR = 'or',
  /** 非门 */
  NOT = 'not',
  /** 与非门 */
  NAND = 'nand',
  /** 或非门 */
  NOR = 'nor',
  /** 异或门 */
  XOR = 'xor',
  /** 同或门 */
  XNOR = 'xnor',
}

// ============ 逻辑连接接口 ============
/**
 * 逻辑连接接口
 * 定义逻辑电路中的连接关系
 */
export interface LogicConnection {
  /** 源 ID */
  from: string;
  /** 目标 ID */
  to: string;
  /** 连接类型 */
  type: 'input' | 'output';
}

// ============ 设备事件接口 ============
/**
 * 设备事件接口
 * 定义设备产生的事件
 */
export interface DeviceEvent {
  /** 事件 ID */
  id: string;
  /** 设备 ID */
  deviceId: string;
  /** 事件类型 */
  type: DeviceEventType;
  /** 事件数据 */
  data: any;
  /** 事件时间戳 */
  timestamp: string;
  /** 事件严重程度 */
  severity: EventSeverity;
}

// ============ 设备事件类型枚举 ============
/**
 * 设备事件类型枚举
 * 定义设备可能产生的事件类型
 */
export enum DeviceEventType {
  /** GPIO 变化 */
  GPIO_CHANGE = 'gpio_change',
  /** 状态变化 */
  STATE_CHANGE = 'state_change',
  /** WiFi 连接 */
  WIFI_CONNECT = 'wifi_connect',
  /** WiFi 断开 */
  WIFI_DISCONNECT = 'wifi_disconnect',
  /** 错误 */
  ERROR = 'error',
  /** 警告 */
  WARNING = 'warning',
  /** 信息 */
  INFO = 'info',
}

// ============ 事件严重程度枚举 ============
/**
 * 事件严重程度枚举
 * 定义事件的严重程度级别
 */
export enum EventSeverity {
  /** 低 */
  LOW = 'low',
  /** 中 */
  MEDIUM = 'medium',
  /** 高 */
  HIGH = 'high',
  /** 严重 */
  CRITICAL = 'critical',
}

// ============ 设备指标接口 ============
/**
 * 设备指标接口
 * 定义设备的运行指标
 */
export interface DeviceMetrics {
  /** 设备 ID */
  deviceId: string;
  /** 时间戳 */
  timestamp: string;
  /** CPU 使用率（可选） */
  cpuUsage?: number;
  /** 内存使用率（可选） */
  memoryUsage?: number;
  /** 温度（可选） */
  temperature?: number;
  /** 运行时间（可选） */
  uptime?: number;
  /** 错误计数（可选） */
  errorCount?: number;
  /** 最后重置时间（可选） */
  lastReset?: string;
}

// ============ 设备命令接口 ============
/**
 * 设备命令接口
 * 定义发送给设备的命令
 */
export interface DeviceCommand {
  /** 命令 ID */
  id: string;
  /** 设备 ID */
  deviceId: string;
  /** 命令类型 */
  type: DeviceCommandType;
  /** 命令参数 */
  parameters: Record<string, any>;
  /** 命令状态 */
  status: CommandStatus;
  /** 创建时间 */
  createdAt: string;
  /** 执行时间（可选） */
  executedAt?: string;
  /** 执行结果（可选） */
  result?: any;
  /** 错误信息（可选） */
  error?: string;
}

// ============ 设备命令类型枚举 ============
/**
 * 设备命令类型枚举
 * 定义支持的设备命令类型
 */
export enum DeviceCommandType {
  /** 重启 */
  REBOOT = 'reboot',
  /** 重置 */
  RESET = 'reset',
  /** 更新固件 */
  UPDATE_FIRMWARE = 'update_firmware',
  /** 设置 GPIO */
  SET_GPIO = 'set_gpio',
  /** 获取状态 */
  GET_STATUS = 'get_status',
  /** 设置 WiFi */
  SET_WIFI = 'set_wifi',
  /** 触发状态 */
  TRIGGER_STATE = 'trigger_state',
}

// ============ 命令状态枚举 ============
/**
 * 命令状态枚举
 * 定义命令的执行状态
 */
export enum CommandStatus {
  /** 等待中 */
  PENDING = 'pending',
  /** 执行中 */
  EXECUTING = 'executing',
  /** 已完成 */
  COMPLETED = 'completed',
  /** 失败 */
  FAILED = 'failed',
  /** 超时 */
  TIMEOUT = 'timeout',
}

// ============ 类型守卫函数 ============
/**
 * 检查值是否为有效的设备类型。
 */
export function isValidDeviceType(value: unknown): value is DeviceType {
  return typeof value === 'string' && Object.values(DeviceType).includes(value as DeviceType);
}

/**
 * 检查值是否为有效的设备状态。
 */
export function isValidDeviceStatus(value: unknown): value is DeviceStatus {
  return typeof value === 'string' && Object.values(DeviceStatus).includes(value as DeviceStatus);
}

/**
 * 检查值是否为有效的 GPIO 模式
 */
export function isValidGPIOMode(value: unknown): value is GPIOMode {
  return typeof value === 'string' && Object.values(GPIOMode).includes(value as GPIOMode);
}

/**
 * 检查值是否为有效的 GPIO 状态
 */
export function isValidGPIOStatus(value: unknown): value is GPIOStatus {
  if (!value || typeof value !== 'object') return false;

  const gpio = value as GPIOStatus;
  return (
    typeof gpio.pin === 'number' &&
    isValidGPIOMode(gpio.mode) &&
    typeof gpio.value === 'number' &&
    typeof gpio.lastUpdated === 'string'
  );
}

/**
 * 检查值是否为有效的 WiFi 配置。
 */
export function isValidWiFiConfig(value: unknown): value is WiFiConfig {
  if (!value || typeof value !== 'object') return false;

  const config = value as WiFiConfig;
  return (
    typeof config.ssid === 'string' &&
    (config.password === undefined || typeof config.password === 'string')
  );
}
