import { http, HttpResponse } from 'msw';
import { mockWiFiConfig, mockWiFiStatus, mockWiFiNetworks } from '../data';

export const wifiHandlers = [
  http.get('/api/v3/wifi/config', () => {
    return HttpResponse.json(mockWiFiConfig);
  }),

  http.put('/api/v3/wifi/config', async ({ request }) => {
    const body = (await request.json()) as Partial<typeof mockWiFiConfig>;
    const mergedBody = {
      ssid: body.ssid ?? mockWiFiConfig.ssid,
      password: body.password ?? mockWiFiConfig.password,
      ip: body.ip ?? mockWiFiConfig.ip,
      gateway: body.gateway ?? mockWiFiConfig.gateway,
      subnet: body.subnet ?? mockWiFiConfig.subnet,
      dns: body.dns ?? mockWiFiConfig.dns,
      dhcp: body.dhcp ?? mockWiFiConfig.dhcp,
    };
    return HttpResponse.json(mergedBody);
  }),

  http.post('/api/v3/wifi/config', async ({ request }) => {
    const body = (await request.json()) as Partial<typeof mockWiFiConfig>;
    const mergedBody = {
      ssid: body.ssid ?? mockWiFiConfig.ssid,
      password: body.password ?? mockWiFiConfig.password,
      ip: body.ip ?? mockWiFiConfig.ip,
      gateway: body.gateway ?? mockWiFiConfig.gateway,
      subnet: body.subnet ?? mockWiFiConfig.subnet,
      dns: body.dns ?? mockWiFiConfig.dns,
      dhcp: body.dhcp ?? mockWiFiConfig.dhcp,
    };
    return HttpResponse.json(mergedBody);
  }),

  http.get('/api/v3/wifi/status', () => {
    return HttpResponse.json(mockWiFiStatus);
  }),

  http.post('/api/v3/wifi/scan', () => {
    return HttpResponse.json(mockWiFiNetworks);
  }),

  http.get('/api/v3/wifi/scan', () => {
    return HttpResponse.json(mockWiFiNetworks);
  }),

  http.get('/api/v3/device/wifi/config', () => {
    return HttpResponse.json(mockWiFiConfig);
  }),

  http.put('/api/v3/device/wifi/config', async ({ request }) => {
    const body = (await request.json()) as Partial<typeof mockWiFiConfig>;
    const mergedBody = {
      ssid: body.ssid ?? mockWiFiConfig.ssid,
      password: body.password ?? mockWiFiConfig.password,
      ip: body.ip ?? mockWiFiConfig.ip,
      gateway: body.gateway ?? mockWiFiConfig.gateway,
      subnet: body.subnet ?? mockWiFiConfig.subnet,
      dns: body.dns ?? mockWiFiConfig.dns,
      dhcp: body.dhcp ?? mockWiFiConfig.dhcp,
    };
    return HttpResponse.json(mergedBody);
  }),

  http.post('/api/v3/device/wifi/config', async ({ request }) => {
    const body = (await request.json()) as Partial<typeof mockWiFiConfig>;
    const mergedBody = {
      ssid: body.ssid ?? mockWiFiConfig.ssid,
      password: body.password ?? mockWiFiConfig.password,
      ip: body.ip ?? mockWiFiConfig.ip,
      gateway: body.gateway ?? mockWiFiConfig.gateway,
      subnet: body.subnet ?? mockWiFiConfig.subnet,
      dns: body.dns ?? mockWiFiConfig.dns,
      dhcp: body.dhcp ?? mockWiFiConfig.dhcp,
    };
    return HttpResponse.json(mergedBody);
  }),

  http.get('/api/v3/device/wifi/status', () => {
    return HttpResponse.json(mockWiFiStatus);
  }),

  http.post('/api/v3/device/wifi/scan', () => {
    return HttpResponse.json(mockWiFiNetworks);
  }),

  http.get('/api/v3/device/wifi/scan', () => {
    return HttpResponse.json(mockWiFiNetworks);
  }),
];
