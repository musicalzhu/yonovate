/**
 * Vue 3.5+ 导出导入工具函数
 * 统一的数据导出和导入实现
 */
import type {
  ExportableEntity,
  ExportMetadata,
  ExportOptions,
  ImportOptions,
  ImportResult,
} from '@/types/exportable';

/**
 * 默认导出版本号，所有导出实体都会携带该版本标识。
 */
const DEFAULT_EXPORT_VERSION = '1.0.0';

/**
 * 统一构建导出实体的公共元数据对象。
 */
function createMetadata(dataType: string, customMetadata?: Record<string, unknown>) {
  return {
    version: DEFAULT_EXPORT_VERSION,
    exportedAt: new Date().toISOString(),
    dataType,
    ...customMetadata,
  };
}

/**
 * 统一构建导出实体的元数据载荷，避免导出时重复拼接相同字段。
 */
function buildEntityMetadata(
  dataType: string,
  includeMetadata: boolean,
  customMetadata?: Record<string, unknown>
) {
  if (!includeMetadata) {
    return {
      version: DEFAULT_EXPORT_VERSION,
      exportedAt: new Date().toISOString(),
      dataType,
    };
  }

  return createMetadata(dataType, customMetadata);
}

/**
 * 导出数据为 JSON 字符串
 * @param data - 要导出的数据
 * @param dataType - 数据类型标识
 * @param options - 导出选项
 * @returns JSON 字符串
 */
export function exportToJson<T>(data: T, dataType: string, options: ExportOptions = {}): string {
  const { includeMetadata = true, format = true, customMetadata } = options;

  const metadata: ExportMetadata = buildEntityMetadata(
    dataType,
    includeMetadata,
    customMetadata
  ) as ExportMetadata;

  const entity: ExportableEntity<T> = {
    data,
    metadata,
  };

  return JSON.stringify(entity, null, format ? 2 : 0);
}

/**
 * 从 JSON 字符串导入数据
 * @param json - JSON 字符串
 * @param options - 导入选项
 * @returns 导入结果
 */
export function importFromJson<T>(json: string, options: ImportOptions<T> = {}): ImportResult<T> {
  const { validate = true, validator, migrate = false, migrator } = options;

  try {
    const parsed = JSON.parse(json);

    // 检查是否为导出格式
    if (parsed.data && parsed.metadata) {
      const entity = parsed as ExportableEntity<T>;
      const version = entity.metadata.version || '1.0.0';

      // 验证数据
      if (validate && validator) {
        if (!validator(entity.data)) {
          return {
            success: false,
            data: null,
            error: '数据验证失败',
          };
        }
      }

      // 迁移数据
      if (migrate && migrator) {
        try {
          const migratedData = migrator(entity.data, version);
          return {
            success: true,
            data: migratedData,
            version,
          };
        } catch (err) {
          return {
            success: false,
            data: null,
            error: '数据迁移失败',
          };
        }
      }

      return {
        success: true,
        data: entity.data,
        version,
      };
    }

    // 兼容旧格式（直接是数据）
    if (validate && validator) {
      if (!validator(parsed)) {
        return {
          success: false,
          data: null,
          error: '数据验证失败',
        };
      }
    }

    return {
      success: true,
      data: parsed,
      version: 'legacy',
    };
  } catch (err) {
    return {
      success: false,
      data: null,
      error: err instanceof Error ? err.message : '解析 JSON 失败',
    };
  }
}

/**
 * 导出数据为文件
 * @param data - 要导出的数据
 * @param dataType - 数据类型标识
 * @param filename - 文件名
 * @param options - 导出选项
 */
export function exportToFile<T>(
  data: T,
  dataType: string,
  filename: string,
  options: ExportOptions = {}
): void {
  const json = exportToJson(data, dataType, options);
  const blob = new Blob([json], { type: 'application/json' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  a.click();
  URL.revokeObjectURL(url);
}

/**
 * 从文件导入数据
 * @param file - 文件对象
 * @param options - 导入选项
 * @returns 导入结果 Promise
 */
export async function importFromFile<T>(
  file: File,
  options: ImportOptions<T> = {}
): Promise<ImportResult<T>> {
  try {
    const text = await file.text();
    return importFromJson<T>(text, options);
  } catch (err) {
    return {
      success: false,
      data: null,
      error: err instanceof Error ? err.message : '读取文件失败',
    };
  }
}

/**
 * 克隆数据（深拷贝）
 * @param data - 要克隆的数据
 * @returns 克隆后的数据
 */
export function cloneData<T>(data: T): T {
  return JSON.parse(JSON.stringify(data));
}
