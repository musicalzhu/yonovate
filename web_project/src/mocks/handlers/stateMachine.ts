import { http, HttpResponse } from 'msw';
import { mockStateMachineConfig, mockStateMachineStatus } from '../data';

export const stateMachineHandlers = [
  http.get('/api/v3/state-machine/config', () => {
    return HttpResponse.json(mockStateMachineConfig);
  }),

  http.post('/api/v3/state-machine/config', async ({ request }) => {
    const body = await request.json();
    return HttpResponse.json(body);
  }),

  http.get('/api/v3/state-machine/status', () => {
    return HttpResponse.json(mockStateMachineStatus);
  }),

  http.post('/api/v3/state-machine/status', async ({ request }) => {
    const body = await request.json();
    return HttpResponse.json(body);
  }),

  http.post('/api/v3/state-machine/reset', () => {
    return HttpResponse.json({ success: true });
  }),
];
