import axios from 'axios';
import type { AxiosRequestConfig, AxiosResponse } from 'axios';
import { handleError } from './notification/errorHandler';

/**
 * 读取当前 API 基础地址，优先使用本地缓存中的配置。
 */
function getBaseURL(): string {
  if (import.meta.env.DEV) {
    return '/';
  }
  return localStorage.getItem('esp32-ip') || 'http://192.168.3.55';
}

const axiosInstance = axios.create({
  baseURL: getBaseURL(),
  timeout: 10000,
  headers: {
    'Content-Type': 'application/json',
  },
});

axiosInstance.interceptors.request.use(
  (config) => {
    console.debug(`[API] 请求: ${config.method?.toUpperCase()} ${config.url}`);
    config.baseURL = getBaseURL();
    return config;
  },
  (error) => {
    console.error(`[API] 请求错误: ${error.message}`);
    handleError(error, { showToast: true });
    return Promise.reject(error);
  }
);

axiosInstance.interceptors.response.use(
  (response) => {
    console.debug(`[API] 响应: ${response.status} ${response.config.url}`);
    return response;
  },
  (error) => {
    console.error(`[API] 响应错误: ${error.message}`);
    handleError(error, { showToast: true });
    return Promise.reject(error);
  }
);

/**
 * 统一封装对后端接口的请求入口
 * MSW 在网络层拦截请求，无需在此处处理 mock
 */
export const customInstance = <T>(config: AxiosRequestConfig): Promise<AxiosResponse<T>> => {
  return axiosInstance(config);
};

/**
 * 更新接口基础地址，并同步持久化到本地存储。
 */
export const setBaseURL = (baseURL: string) => {
  localStorage.setItem('esp32-ip', baseURL);
  axiosInstance.defaults.baseURL = baseURL;
};

/**
 * 获取当前正在使用的接口基础地址。
 */
export const getCurrentBaseURL = (): string => {
  return axiosInstance.defaults.baseURL || getBaseURL();
};

export default customInstance;
