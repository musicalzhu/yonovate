import { useMutation, useQuery } from '@tanstack/vue-query';
import type { QueryKey, UseMutationOptions, UseQueryOptions } from '@tanstack/vue-query';

import { customInstance } from '../services/httpClient';

export interface Trigger {
  triggerType: number;
  triggerSource: string;
  operator: number;
  triggerValue: number;
}

export interface Transition {
  currentState: string;
  nextState: string;
  triggers: Trigger[];
  triggerCount: number;
  triggerLogic: number;
}

export interface StateEffect {
  stateName: string;
  effectType: number;
  target: string;
  value: number;
  duration?: number;
}

export interface StateMachineConfig {
  initialState: string;
  transitions: Transition[];
  stateEffects: StateEffect[];
  groupModes?: number[];
  inputModuleTypes?: number[];
  edgeTypes?: number[];
  outputModuleTypes?: number[];
}

export interface StateMachineStatus {
  currentState?: string;
  lastTriggerTime?: number;
  triggerCount?: number;
  errorCount?: number;
  uptime?: number;
}

export interface DeviceInfo {
  deviceId?: string;
  name?: string;
  type?: string;
  status?: string;
  lastSeen?: string;
  firmware?: string;
  ipAddress?: string;
  macAddress?: string;
  version?: string;
  capabilities?: string[];
}

export interface DeviceStatus {
  status?: string;
  uptime?: number;
}

export interface GPIOStatus {
  pin: number;
  mode: string;
  value: number;
  label?: string;
  description?: string;
  lastUpdated: string;
}

export interface GPIOSetRequest {
  pin: number;
  value: number;
}

export interface GPIOConfigRequest {
  mode: number;
  value?: number;
}

export interface WiFiConfig {
  ssid: string;
  password?: string;
  ip?: string;
  gateway?: string;
  subnet?: string;
  dns?: string;
  dhcp?: boolean;
}

export interface WiFiStatus {
  connected?: boolean;
  ssid?: string;
  ip?: string;
  gateway?: string;
  subnet?: string;
  signalStrength?: number;
  lastConnected?: string;
}

export interface WiFiNetwork {
  ssid?: string;
  rssi?: number;
  encryption?: string;
}

export interface DeviceCommand {
  id?: string;
  deviceId?: string;
  type?: string;
  parameters?: object;
  status?: string;
  createdAt?: string;
  executedAt?: string;
  result?: object;
  error?: string;
}

export interface DeviceCommandRequest {
  type: string;
  parameters?: object;
}

export interface DeviceMetrics {
  deviceId?: string;
  timestamp?: string;
  cpuUsage?: number;
  memoryUsage?: number;
  temperature?: number;
  uptime?: number;
  errorCount?: number;
  lastReset?: string;
}

const QUERY_KEYS = {
  stateMachineConfig: ['stateMachine', 'config'] as const,
  stateMachineStatus: ['stateMachine', 'status'] as const,
  deviceInfo: ['device', 'info'] as const,
  deviceStatus: ['device', 'status'] as const,
  gpioStatus: ['device', 'gpio'] as const,
  wifiConfig: ['device', 'wifi', 'config'] as const,
  wifiStatus: ['device', 'wifi', 'status'] as const,
  wifiNetworks: ['device', 'wifi', 'networks'] as const,
  deviceCommands: ['device', 'commands'] as const,
  deviceMetrics: ['device', 'metrics'] as const,
};

export { QUERY_KEYS };

export const getStateMachineConfig = () => {
  return customInstance<StateMachineConfig>({ url: '/api/v3/state-machine/config', method: 'GET' });
};

export const saveStateMachineConfig = (data: StateMachineConfig) => {
  return customInstance<StateMachineConfig>({
    url: '/api/v3/state-machine/config',
    method: 'POST',
    data,
  });
};

export const getStateMachineStatus = () => {
  return customInstance<StateMachineStatus>({ url: '/api/v3/state-machine/status', method: 'GET' });
};

export const resetStateMachine = () => {
  return customInstance<void>({ url: '/api/v3/state-machine/reset', method: 'POST' });
};

export const getDeviceInfo = () => {
  return customInstance<DeviceInfo>({ url: '/api/v3/device/info', method: 'GET' });
};

export const getDeviceStatus = () => {
  return customInstance<DeviceStatus>({ url: '/api/v3/device/status', method: 'GET' });
};

export const rebootDevice = () => {
  return customInstance<void>({ url: '/api/v3/device/reboot', method: 'POST' });
};

export const factoryResetDevice = () => {
  return customInstance<void>({ url: '/api/v3/device/factory-reset', method: 'POST' });
};

export const getAllGPIOStatus = () => {
  return customInstance<GPIOStatus[]>({ url: '/api/v3/device/gpio', method: 'GET' });
};

export const setGPIOValue = (data: GPIOSetRequest) => {
  return customInstance<void>({ url: '/api/v3/device/gpio', method: 'POST', data });
};

export const getGPIOStatus = (pin: number) => {
  return customInstance<GPIOStatus>({ url: `/api/v3/device/gpio/${pin}`, method: 'GET' });
};

export const updateGPIOConfig = (pin: number, data: GPIOConfigRequest) => {
  return customInstance<void>({ url: `/api/v3/device/gpio/${pin}`, method: 'PUT', data });
};

export const getWiFiConfig = () => {
  return customInstance<WiFiConfig>({ url: '/api/v3/device/wifi/config', method: 'GET' });
};

export const saveWiFiConfig = (data: WiFiConfig) => {
  return customInstance<WiFiConfig>({ url: '/api/v3/device/wifi/config', method: 'POST', data });
};

export const getWiFiStatus = () => {
  return customInstance<WiFiStatus>({ url: '/api/v3/device/wifi/status', method: 'GET' });
};

export const scanWiFiNetworks = () => {
  return customInstance<WiFiNetwork[]>({ url: '/api/v3/device/wifi/scan', method: 'GET' });
};

export const getDeviceCommands = () => {
  return customInstance<DeviceCommand[]>({ url: '/api/v3/device/commands', method: 'GET' });
};

export const sendDeviceCommand = (data: DeviceCommandRequest) => {
  return customInstance<DeviceCommand>({ url: '/api/v3/device/commands', method: 'POST', data });
};

export const getCommand = (id: string) => {
  return customInstance<DeviceCommand>({ url: `/api/v3/device/commands/${id}`, method: 'GET' });
};

export const deleteCommand = (id: string) => {
  return customInstance<void>({ url: `/api/v3/device/commands/${id}`, method: 'DELETE' });
};

export const getDeviceMetrics = () => {
  return customInstance<DeviceMetrics>({ url: '/api/v3/device/metrics', method: 'GET' });
};

export const useGetStateMachineConfigQuery = (
  options: Omit<
    UseQueryOptions<StateMachineConfig, unknown, StateMachineConfig, QueryKey>,
    'queryKey' | 'queryFn'
  > = {}
) => {
  return useQuery({
    queryKey: QUERY_KEYS.stateMachineConfig,
    queryFn: () => getStateMachineConfig().then((r) => r.data),
    ...options,
  });
};

export const useGetStateMachineStatusQuery = (
  options: Omit<
    UseQueryOptions<StateMachineStatus, unknown, StateMachineStatus, QueryKey>,
    'queryKey' | 'queryFn'
  > = {}
) => {
  return useQuery({
    queryKey: QUERY_KEYS.stateMachineStatus,
    queryFn: () => getStateMachineStatus().then((r) => r.data),
    ...options,
  });
};

export const useGetDeviceInfoQuery = (
  options: Omit<
    UseQueryOptions<DeviceInfo, unknown, DeviceInfo, QueryKey>,
    'queryKey' | 'queryFn'
  > = {}
) => {
  return useQuery({
    queryKey: QUERY_KEYS.deviceInfo,
    queryFn: () => getDeviceInfo().then((r) => r.data),
    ...options,
  });
};

export const useGetDeviceStatusQuery = (
  options: Omit<
    UseQueryOptions<DeviceStatus, unknown, DeviceStatus, QueryKey>,
    'queryKey' | 'queryFn'
  > = {}
) => {
  return useQuery({
    queryKey: QUERY_KEYS.deviceStatus,
    queryFn: () => getDeviceStatus().then((r) => r.data),
    ...options,
  });
};

export const useGetAllGPIOStatusQuery = (
  options: Omit<
    UseQueryOptions<GPIOStatus[], unknown, GPIOStatus[], QueryKey>,
    'queryKey' | 'queryFn'
  > = {}
) => {
  return useQuery({
    queryKey: QUERY_KEYS.gpioStatus,
    queryFn: () => getAllGPIOStatus().then((r) => r.data),
    ...options,
  });
};

export const useGetWiFiConfigQuery = (
  options: Omit<
    UseQueryOptions<WiFiConfig, unknown, WiFiConfig, QueryKey>,
    'queryKey' | 'queryFn'
  > = {}
) => {
  return useQuery({
    queryKey: QUERY_KEYS.wifiConfig,
    queryFn: () => getWiFiConfig().then((r) => r.data),
    ...options,
  });
};

export const useGetWiFiStatusQuery = (
  options: Omit<
    UseQueryOptions<WiFiStatus, unknown, WiFiStatus, QueryKey>,
    'queryKey' | 'queryFn'
  > = {}
) => {
  return useQuery({
    queryKey: QUERY_KEYS.wifiStatus,
    queryFn: () => getWiFiStatus().then((r) => r.data),
    ...options,
  });
};

export const useScanWiFiNetworksQuery = (
  options: Omit<
    UseQueryOptions<WiFiNetwork[], unknown, WiFiNetwork[], QueryKey>,
    'queryKey' | 'queryFn'
  > = {}
) => {
  return useQuery({
    queryKey: QUERY_KEYS.wifiNetworks,
    queryFn: () => scanWiFiNetworks().then((r) => r.data),
    ...options,
  });
};

export const useGetDeviceCommandsQuery = (
  options: Omit<
    UseQueryOptions<DeviceCommand[], unknown, DeviceCommand[], QueryKey>,
    'queryKey' | 'queryFn'
  > = {}
) => {
  return useQuery({
    queryKey: QUERY_KEYS.deviceCommands,
    queryFn: () => getDeviceCommands().then((r) => r.data),
    ...options,
  });
};

export const useGetDeviceMetricsQuery = (
  options: Omit<
    UseQueryOptions<DeviceMetrics, unknown, DeviceMetrics, QueryKey>,
    'queryKey' | 'queryFn'
  > = {}
) => {
  return useQuery({
    queryKey: QUERY_KEYS.deviceMetrics,
    queryFn: () => getDeviceMetrics().then((r) => r.data),
    ...options,
  });
};

export const useSaveStateMachineConfigMutation = (
  options: Omit<
    UseMutationOptions<StateMachineConfig, unknown, StateMachineConfig>,
    'mutationFn'
  > = {}
) => {
  return useMutation({
    mutationFn: saveStateMachineConfig,
    ...options,
  });
};

export const useResetStateMachineMutation = (
  options: Omit<UseMutationOptions<void, unknown, void>, 'mutationFn'> = {}
) => {
  return useMutation({
    mutationFn: resetStateMachine,
    ...options,
  });
};

export const useRebootDeviceMutation = (
  options: Omit<UseMutationOptions<void, unknown, void>, 'mutationFn'> = {}
) => {
  return useMutation({
    mutationFn: rebootDevice,
    ...options,
  });
};

export const useFactoryResetDeviceMutation = (
  options: Omit<UseMutationOptions<void, unknown, void>, 'mutationFn'> = {}
) => {
  return useMutation({
    mutationFn: factoryResetDevice,
    ...options,
  });
};

export const useSetGPIOValueMutation = (
  options: Omit<UseMutationOptions<void, unknown, GPIOSetRequest>, 'mutationFn'> = {}
) => {
  return useMutation({
    mutationFn: setGPIOValue,
    ...options,
  });
};

export const useUpdateGPIOConfigMutation = (
  options: Omit<
    UseMutationOptions<void, unknown, { pin: number; data: GPIOConfigRequest }>,
    'mutationFn'
  > = {}
) => {
  return useMutation({
    mutationFn: ({ pin, data }: { pin: number; data: GPIOConfigRequest }) =>
      updateGPIOConfig(pin, data),
    ...options,
  });
};

export const useSaveWiFiConfigMutation = (
  options: Omit<UseMutationOptions<WiFiConfig, unknown, WiFiConfig>, 'mutationFn'> = {}
) => {
  return useMutation({
    mutationFn: saveWiFiConfig,
    ...options,
  });
};

export const useSendDeviceCommandMutation = (
  options: Omit<UseMutationOptions<DeviceCommand, unknown, DeviceCommandRequest>, 'mutationFn'> = {}
) => {
  return useMutation({
    mutationFn: sendDeviceCommand,
    ...options,
  });
};

export const useDeleteCommandMutation = (
  options: Omit<UseMutationOptions<void, unknown, string>, 'mutationFn'> = {}
) => {
  return useMutation({
    mutationFn: deleteCommand,
    ...options,
  });
};
