import { http, HttpResponse } from 'msw';
import { mockWiFiConfig, mockWiFiStatus, mockWiFiNetworks } from '../data';

export const wifiHandlers = [
  http.get('/api/v3/wifi/config', () => {
    return HttpResponse.json(mockWiFiConfig);
  }),

  http.put('/api/v3/wifi/config', async ({ request }) => {
    const body = await request.json();
    return HttpResponse.json({ ...mockWiFiConfig, ...body });
  }),

  http.post('/api/v3/wifi/config', async ({ request }) => {
    const body = await request.json();
    return HttpResponse.json({ ...mockWiFiConfig, ...body });
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
    const body = await request.json();
    return HttpResponse.json({ ...mockWiFiConfig, ...body });
  }),

  http.post('/api/v3/device/wifi/config', async ({ request }) => {
    const body = await request.json();
    return HttpResponse.json({ ...mockWiFiConfig, ...body });
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
