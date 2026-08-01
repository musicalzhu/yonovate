import { http, HttpResponse } from 'msw';
import type { DeviceCommand } from '@/generated/api';
import { mockCommands } from '../data';

export const commandsHandlers = [
  http.get('/api/v3/commands', () => {
    return HttpResponse.json(mockCommands);
  }),

  http.post('/api/v3/commands', async ({ request }) => {
    const body = (await request.json()) as DeviceCommand;
    const newCommand: DeviceCommand = {
      ...body,
      id: `cmd-${Date.now()}`,
      status: 'pending',
      createdAt: new Date().toISOString(),
    };
    return HttpResponse.json(newCommand);
  }),

  http.get('/api/v3/commands/:id', ({ params }) => {
    const command = mockCommands.find((c) => c.id === params.id);
    if (command) {
      return HttpResponse.json(command);
    }
    return HttpResponse.json({ error: 'Command not found' }, { status: 404 });
  }),

  http.delete('/api/v3/commands/:id', () => {
    return HttpResponse.json({ success: true });
  }),

  http.get('/api/v3/device/commands', () => {
    return HttpResponse.json(mockCommands);
  }),

  http.post('/api/v3/device/commands', async ({ request }) => {
    const body = (await request.json()) as DeviceCommand;
    const newCommand: DeviceCommand = {
      ...body,
      id: `cmd-${Date.now()}`,
      status: 'pending',
      createdAt: new Date().toISOString(),
    };
    return HttpResponse.json(newCommand);
  }),
];
