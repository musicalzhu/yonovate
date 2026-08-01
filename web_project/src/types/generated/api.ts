/**
 * Vue 3.5+ API 类型定义
 * 定义 API 请求、响应、WebSocket 和相关配置的类型
 */

// ============ 基础 API 响应接口 ============
/**
 * 基础 API 响应接口
 * 定义所有 API 响应的通用结构
 */
export interface BaseApiResponse<T = any> {
  /** 响应数据 */
  data: T;
  /** 请求是否成功 */
  success: boolean;
  /** 响应消息（可选） */
  message?: string;
  /** 响应代码（可选） */
  code?: string;
  /** 响应时间戳（可选） */
  timestamp?: number;
}

/**
 * API 响应类型
 * BaseApiResponse 的别名
 */
export type ApiResponse<T = any> = BaseApiResponse<T>;

// ============ 分页相关接口 ============
/**
 * 分页参数接口
 * 定义分页请求的参数
 */
export interface PaginationParams {
  /** 当前页码 */
  page: number;
  /** 每页数量 */
  limit: number;
  /** 排序字段（可选） */
  sort?: string;
  /** 排序方向（可选） */
  order?: 'asc' | 'desc';
}

/**
 * 分页响应接口
 * 定义包含分页信息的响应
 */
export interface PaginatedResponse<T> extends BaseApiResponse<T[]> {
  /** 分页信息 */
  pagination: {
    /** 当前页码 */
    page: number;
    /** 每页数量 */
    limit: number;
    /** 总记录数 */
    total: number;
    /** 总页数 */
    totalPages: number;
  };
}

// ============ 错误响应接口 ============
/**
 * 错误响应接口
 * 定义 API 错误响应的结构
 */
export interface ErrorResponse extends BaseApiResponse {
  /** 成功标志固定为 false */
  success: false;
  /** 错误详情 */
  error: {
    /** 错误代码 */
    code: string;
    /** 错误消息 */
    message: string;
    /** 错误详细信息（可选） */
    details?: any;
  };
}

// ============ HTTP 方法类型 ============
/**
 * HTTP 方法类型
 * 定义支持的 HTTP 方法
 */
export type HttpMethod = 'GET' | 'POST' | 'PUT' | 'DELETE' | 'PATCH';

// ============ API 端点配置接口 ============
/**
 * API 端点配置接口
 * 定义单个 API 端点的配置
 */
export interface ApiEndpoint {
  /** 请求 URL */
  url: string;
  /** HTTP 方法 */
  method: HttpMethod;
  /** 请求头（可选） */
  headers?: Record<string, string>;
  /** 查询参数（可选） */
  params?: Record<string, any>;
  /** 请求体数据（可选） */
  data?: any;
}

// ============ WebSocket 相关接口 ============
/**
 * WebSocket 消息接口
 * 定义 WebSocket 消息的结构
 */
export interface WebSocketMessage {
  /** 消息类型 */
  type: string;
  /** 消息负载 */
  payload: any;
  /** 消息时间戳 */
  timestamp: number;
  /** 消息 ID（可选） */
  id?: string;
}

/**
 * 实时事件接口
 * 定义实时推送的事件
 */
export interface RealtimeEvent {
  /** 事件类型 */
  type: 'state_change' | 'gpio_update' | 'device_status' | 'error';
  /** 设备 ID */
  deviceId: string;
  /** 事件数据 */
  data: any;
  /** 事件时间戳 */
  timestamp: number;
}

// ============ API 请求配置接口 ============
/**
 * API 请求配置接口
 * 定义 API 请求的配置选项
 */
export interface ApiRequestConfig {
  /** 请求超时时间（毫秒，可选） */
  timeout?: number;
  /** 重试次数（可选） */
  retries?: number;
  /** 重试延迟（毫秒，可选） */
  retryDelay?: number;
  /** 请求头（可选） */
  headers?: Record<string, string>;
  /** 查询参数（可选） */
  params?: Record<string, any>;
}

// ============ 缓存配置接口 ============
/**
 * 响应缓存配置接口
 * 定义 API 响应缓存的配置
 */
export interface CacheConfig {
  /** 是否启用缓存 */
  enabled: boolean;
  /** 生存时间（毫秒） */
  ttl: number;
  /** 最大缓存数量 */
  maxSize: number;
}

// ============ 拦截器接口 ============
/**
 * 请求拦截器接口
 * 定义请求拦截器的结构
 */
export interface RequestInterceptor {
  /** 请求拦截器 */
  onRequest?: (config: ApiEndpoint) => Promise<ApiEndpoint>;
  /** 请求错误拦截器 */
  onRequestError?: (error: any) => Promise<any>;
}

/**
 * 响应拦截器接口
 * 定义响应拦截器的结构
 */
export interface ResponseInterceptor {
  /** 响应拦截器 */
  onResponse?: (response: BaseApiResponse) => Promise<BaseApiResponse>;
  /** 响应错误拦截器 */
  onResponseError?: (error: any) => Promise<any>;
}

// ============ API 服务接口 ============
/**
 * API 服务接口
 * 定义 API 服务的方法
 */
export interface ApiService {
  /** 通用请求方法 */
  request<T>(endpoint: ApiEndpoint, config?: ApiRequestConfig): Promise<BaseApiResponse<T>>;
  /** GET 请求 */
  get<T>(url: string, params?: Record<string, any>): Promise<BaseApiResponse<T>>;
  /** POST 请求 */
  post<T>(url: string, data?: any): Promise<BaseApiResponse<T>>;
  /** PUT 请求 */
  put<T>(url: string, data?: any): Promise<BaseApiResponse<T>>;
  /** DELETE 请求 */
  delete<T>(url: string): Promise<BaseApiResponse<T>>;
  /** PATCH 请求 */
  patch<T>(url: string, data?: any): Promise<BaseApiResponse<T>>;
}

// ============ 文件上传接口 ============
/**
 * 文件上传接口
 * 定义文件上传的信息
 */
export interface FileUpload {
  /** 文件对象 */
  file: File;
  /** 文件名称 */
  name: string;
  /** 文件大小 */
  size: number;
  /** 文件类型 */
  type: string;
  /** 上传进度（可选） */
  progress?: number;
}

// ============ 批量操作接口 ============
/**
 * 批量操作接口
 * 定义批量操作的配置
 */
export interface BatchOperation<T> {
  /** 操作项列表 */
  items: T[];
  /** 操作类型 */
  operation: 'create' | 'update' | 'delete';
  /** 操作选项（可选） */
  options?: {
    /** 出错时是否继续 */
    continueOnError?: boolean;
    /** 批次大小 */
    batchSize?: number;
  };
}

// ============ API 版本信息接口 ============
/**
 * API 版本信息接口
 * 定义 API 的版本信息
 */
export interface ApiVersion {
  /** 版本号 */
  version: string;
  /** 构建号 */
  build: string;
  /** 构建时间戳 */
  timestamp: string;
  /** 支持的功能列表 */
  features: string[];
}
