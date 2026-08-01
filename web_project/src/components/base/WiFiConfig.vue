<template>
  <div class="container">
    <div class="form">
      <div class="field">
        <label class="label">网络名称 (SSID)</label>
        <input
          v-model="config.ssid"
          type="text"
          class="input"
          placeholder="请输入WiFi名称"
          :disabled="readonly"
        />
      </div>

      <div class="field">
        <label class="label">密码</label>
        <div class="password">
          <input
            v-model="config.password"
            :type="showPassword ? 'text' : 'password'"
            class="input"
            placeholder="请输入WiFi密码"
            :disabled="readonly"
          />
          <button
            class="toggle"
            :disabled="readonly"
            type="button"
            @click="showPassword = !showPassword"
          >
            <svg
              v-if="!showPassword"
              width="20"
              height="20"
              viewBox="0 0 20 20"
              fill="currentColor"
            >
              <path d="M10 12a2 2 0 100-4 2 2 0 000 4z" />
              <path
                fill-rule="evenodd"
                d="M.458 10C1.732 5.943 5.522 3 10 3s8.268 2.943 9.542 7c-1.274 4.057-5.064 7-9.542 7S1.732 14.057.458 10zM14 10a4 4 0 11-8 0 4 4 0 018 0z"
                clip-rule="evenodd"
              />
            </svg>
            <svg v-else width="20" height="20" viewBox="0 0 20 20" fill="currentColor">
              <path
                fill-rule="evenodd"
                d="M3.707 2.293a1 1 0 00-1.414 1.414l14 14a1 1 0 001.414-1.414l-1.473-1.473A10.014 10.014 0 0019.542 10C18.268 5.943 14.478 3 10 3a9.958 9.958 0 00-4.512 1.074l-1.78-1.781zm4.261 4.26l1.514 1.515a2.003 2.003 0 012.45 2.45l1.514 1.514a4 4 0 00-5.478-5.478z"
                clip-rule="evenodd"
              />
              <path
                d="M12.454 16.697L9.75 13.992a4 4 0 01-3.742-3.741L2.335 6.578A9.98 9.98 0 00.458 10c1.274 4.057 5.064 7 9.542 7 .847 0 1.669-.105 2.454-.303z"
              />
            </svg>
          </button>
        </div>
      </div>

      <div class="field">
        <label class="label">IP地址</label>
        <input
          v-model="config.ip"
          type="text"
          class="input"
          placeholder="自动获取或手动输入"
          :disabled="readonly || config.dhcp"
        />
      </div>

      <div class="field">
        <label class="label">网关</label>
        <input
          v-model="config.gateway"
          type="text"
          class="input"
          placeholder="网关地址"
          :disabled="readonly || config.dhcp"
        />
      </div>

      <div class="field">
        <label class="label">子网掩码</label>
        <input
          v-model="config.subnet"
          type="text"
          class="input"
          placeholder="子网掩码"
          :disabled="readonly || config.dhcp"
        />
      </div>

      <div class="field">
        <label class="label">DNS服务器</label>
        <input
          v-model="config.dns"
          type="text"
          class="input"
          placeholder="DNS服务器地址"
          :disabled="readonly || config.dhcp"
        />
      </div>

      <div class="field">
        <label class="checkbox">
          <input v-model="config.dhcp" type="checkbox" :disabled="readonly" />
          <span>自动获取IP地址 (DHCP)</span>
        </label>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';
import type { WiFiConfig } from '@/types';

interface Props {
  modelValue: WiFiConfig;
  readonly?: boolean;
}

interface Emits {
  'update:modelValue': [config: WiFiConfig];
}

const props = withDefaults(defineProps<Props>(), {
  readonly: false,
});

const emit = defineEmits<Emits>();

const showPassword = ref(false);

const config = computed({
  get: () => props.modelValue,
  set: (newConfig) => {
    emit('update:modelValue', { ...newConfig });
  },
});
</script>

<style scoped>
.container {
  display: flex;
  flex-direction: column;
  gap: 1rem;
}

.form {
  display: flex;
  flex-direction: column;
  gap: 0.75rem;
}

.field {
  display: flex;
  flex-direction: column;
  gap: 0.25rem;
}

.label {
  display: block;
  font-size: 0.875rem;
  font-weight: 500;
  color: #374151;
}

.input {
  width: 100%;
  padding: 0.5rem 0.75rem;
  border: 1px solid #d1d5db;
  border-radius: 0.5rem;
  background-color: white;
  color: #111827;
  outline: none;
  transition: border-color 0.2s;
}

.input:focus {
  box-shadow: 0 0 0 2px rgba(59, 130, 246, 0.5);
}

.input:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.password {
  position: relative;
}

.toggle {
  position: absolute;
  right: 0.5rem;
  top: 50%;
  transform: translateY(-50%);
  padding: 0.25rem;
  color: #6b7280;
  transition: color 0.2s;
}

.toggle:hover {
  color: #374151;
}

.toggle:disabled {
  opacity: 0.5;
  cursor: not-allowed;
}

.checkbox {
  display: flex;
  align-items: center;
  gap: 0.5rem;
  cursor: pointer;
}

.checkbox input {
  width: 1rem;
  height: 1rem;
  border-radius: 0.25rem;
  border-color: #d1d5db;
  color: #2563eb;
}

.checkbox input:focus {
  box-shadow: 0 0 0 2px rgba(59, 130, 246, 0.5);
}

.checkbox span {
  font-size: 0.875rem;
  color: #374151;
}
</style>
