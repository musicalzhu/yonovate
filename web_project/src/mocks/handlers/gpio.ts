import { http, HttpResponse } from 'msw';
import { mockGPIOStatus } from '../data';

export const gpioHandlers = [
  http.get('/api/v3/gpio/status', () => {
    return HttpResponse.json(mockGPIOStatus);
  }),

  http.get('/api/v3/gpio/status/:pin', ({ params }) => {
    const pin = parseInt(params.pin as string, 10);
    const gpio = mockGPIOStatus.find((g) => g.pin === pin);
    if (gpio) {
      return HttpResponse.json(gpio);
    }
    return HttpResponse.json({ error: 'GPIO pin not found' }, { status: 404 });
  }),

  http.post('/api/v3/gpio/set', () => {
    return HttpResponse.json({ success: true });
  }),

  http.put('/api/v3/gpio/config/:pin', () => {
    return HttpResponse.json({ success: true });
  }),

  http.get('/api/v3/device/gpio', () => {
    return HttpResponse.json(mockGPIOStatus);
  }),

  http.post('/api/v3/device/gpio', async ({ request }) => {
    const body = (await request.json()) as Record<string, unknown>;
    return HttpResponse.json({ ...(body as Record<string, unknown>), success: true });
  }),
];
