/**
 * Vue 3.5+ 可导出实体类型定义
 * 统一的数据导出和导入接口
 */

/**
 * 导出元数据
 */
export interface ExportMetadata {
  /** 导出版本 */
  version: string;
  /** 导出时间 */
  exportedAt: string;
  /** 导出者 */
  exporter?: string;
  /** 数据类型标识 */
  dataType: string;
  /** 额外的元数据 */
  [key: string]: any;
}

/**
 * 可导出实体接口
 */
export interface ExportableEntity<T = any> {
  /** 实体数据 */
  data: T;
  /** 元数据 */
  metadata: ExportMetadata;
}

/**
 * 导出选项
 */
export interface ExportOptions {
  /** 是否包含元数据 */
  includeMetadata?: boolean;
  /** 是否格式化 JSON */
  format?: boolean;
  /** 自定义元数据 */
  customMetadata?: Partial<ExportMetadata>;
}

/**
 * 导入选项
 */
export interface ImportOptions<T = any> {
  /** 是否验证数据 */
  validate?: boolean;
  /** 验证函数 */
  validator?: (data: any) => data is T;
  /** 是否迁移旧版本 */
  migrate?: boolean;
  /** 迁移函数 */
  migrator?: (data: any, version: string) => T;
}

/**
 * 导入结果
 */
export interface ImportResult<T> {
  /** 是否成功 */
  success: boolean;
  /** 数据 */
  data: T | null;
  /** 错误信息 */
  error?: string;
  /** 版本信息 */
  version?: string;
}
