import { deviceHandlers } from './device';
import { gpioHandlers } from './gpio';
import { wifiHandlers } from './wifi';
import { stateMachineHandlers } from './stateMachine';
import { commandsHandlers } from './commands';
import { legacyHandlers } from './legacy';

export const handlers = [
  ...deviceHandlers,
  ...gpioHandlers,
  ...wifiHandlers,
  ...stateMachineHandlers,
  ...commandsHandlers,
  ...legacyHandlers,
];
