import { http, HttpResponse } from 'msw';
import { mockDeviceInfo, mockDeviceStatus, mockDeviceMetrics } from '../data';

export const deviceHandlers = [
  http.get('/api/v3/device/info', () => {
    return HttpResponse.json(mockDeviceInfo);
  }),

  http.get('/api/v3/device/status', () => {
    return HttpResponse.json(mockDeviceStatus);
  }),

  http.post('/api/v3/device/reboot', () => {
    return HttpResponse.json({ success: true });
  }),

  http.post('/api/v3/device/factory-reset', () => {
    return HttpResponse.json({ success: true });
  }),

  http.get('/api/v3/metrics', () => {
    return HttpResponse.json(mockDeviceMetrics);
  }),

  http.get('/api/v3/device/metrics', () => {
    return HttpResponse.json(mockDeviceMetrics);
  }),
];
