import { http, HttpResponse } from 'msw';
import { mockDeviceInfo, mockGPIOStatus } from '../data';

export const legacyHandlers = [
  http.get('/api/device/info', () => {
    return HttpResponse.json(mockDeviceInfo);
  }),

  http.get('/api/gpio/status', () => {
    return HttpResponse.json(mockGPIOStatus);
  }),

  http.post('/api/gpio/set', () => {
    return HttpResponse.json({ success: true });
  }),
];
