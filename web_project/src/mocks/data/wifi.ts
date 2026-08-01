import type { WiFiConfig, WiFiStatus, WiFiNetwork } from '@/generated/api';

export const mockWiFiConfig: WiFiConfig = {
  ssid: 'TestWiFi',
  password: 'password123',
  ip: '192.168.3.55',
  gateway: '192.168.3.1',
  subnet: '255.255.255.0',
  dns: '8.8.8.8',
  dhcp: false,
};

export const mockWiFiStatus: WiFiStatus = {
  connected: true,
  ssid: 'TestWiFi',
  ip: '192.168.3.55',
  gateway: '192.168.3.1',
  subnet: '255.255.255.0',
  signalStrength: -55,
  lastConnected: new Date().toISOString(),
};

export const mockWiFiNetworks: WiFiNetwork[] = [
  { ssid: 'TestWiFi', rssi: -55, encryption: 'WPA2' },
  { ssid: 'HomeWiFi', rssi: -65, encryption: 'WPA2' },
  { ssid: 'OfficeWiFi', rssi: -75, encryption: 'WPA3' },
  { ssid: 'PublicWiFi', rssi: -80, encryption: 'None' },
];
