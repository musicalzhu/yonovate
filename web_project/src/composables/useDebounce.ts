/**
 * Vue 3.5+ 防抖组合式函数 (VueUse 风格)
 * 提供通用的防抖功能，适用于函数调用和值更新
 *
 * @example
 * ```typescript
 * // 防抖函数调用
 * const { debouncedFn, cancel } = useDebounce(() => {
 *   console.log('执行')
 * }, 300)
 *
 * // 防抖值更新
 * const { value, debouncedValue } = useDebounceValue(input, 500)
 * ```
 */
import { ref, onUnmounted, type Ref } from 'vue';

/**
 * 防抖函数选项
 */
export interface UseDebounceOptions {
  /** 防抖延迟（毫秒） */
  delay?: number;
  /** 是否在延迟开始时立即执行 */
  leading?: boolean;
  /** 是否在延迟结束时执行 */
  trailing?: boolean;
}

/**
 * 防抖函数返回值
 */
export interface UseDebounceReturn<T extends (...args: any[]) => any> {
  /** 防抖后的函数 */
  debouncedFn: T;
  /** 取消防抖 */
  cancel: () => void;
  /** 是否正在等待 */
  isPending: Ref<boolean>;
  /** 立即执行 */
  flush: () => void;
}

/**
 * 防抖函数
 * @param fn - 要防抖的函数
 * @param options - 配置选项
 * @returns 防抖函数和控制方法
 */
export function useDebounce<T extends (...args: any[]) => any>(
  fn: T,
  options: UseDebounceOptions = {}
): UseDebounceReturn<T> {
  const { delay = 300, leading = false, trailing = true } = options;

  let timeout: NodeJS.Timeout | null = null;
  let lastArgs: Parameters<T> | null = null;
  const isPending = ref(false);

  const debouncedFn = ((...args: Parameters<T>) => {
    lastArgs = args;

    // 立即执行（leading edge）
    if (leading && !timeout) {
      fn(...args);
    }

    // 取消之前的定时器
    if (timeout) {
      clearTimeout(timeout);
    }

    isPending.value = true;

    // 设置新的定时器
    timeout = setTimeout(() => {
      if (trailing && lastArgs) {
        fn(...lastArgs);
      }
      timeout = null;
      isPending.value = false;
      lastArgs = null;
    }, delay);
  }) as T;

  const cancel = () => {
    if (timeout) {
      clearTimeout(timeout);
      timeout = null;
    }
    isPending.value = false;
    lastArgs = null;
  };

  const flush = () => {
    if (timeout && lastArgs) {
      clearTimeout(timeout);
      fn(...lastArgs);
      timeout = null;
      isPending.value = false;
      lastArgs = null;
    }
  };

  onUnmounted(() => {
    cancel();
  });

  return {
    debouncedFn,
    cancel,
    isPending,
    flush,
  };
}

/**
 * 防抖值选项
 */
export interface UseDebounceValueOptions {
  /** 防抖延迟（毫秒） */
  delay?: number;
}

/**
 * 防抖值返回值
 */
export interface UseDebounceValueReturn<T> {
  /** 原始值 */
  value: Ref<T>;
  /** 防抖后的值 */
  debouncedValue: Ref<T>;
  /** 取消防抖 */
  cancel: () => void;
  /** 立即更新 */
  flush: () => void;
}

/**
 * 防抖值更新
 * @param source - 源值
 * @returns 防抖后的值和控制方法
 */
export function useDebounceValue<T>(source: Ref<T>): UseDebounceValueReturn<T> {
  const debouncedValue = ref<T>(source.value);
  let timeout: NodeJS.Timeout | null = null;

  const cancel = () => {
    if (timeout) {
      clearTimeout(timeout);
      timeout = null;
    }
  };

  const flush = () => {
    cancel();
    debouncedValue.value = source.value;
  };

  onUnmounted(() => {
    cancel();
  });

  return {
    value: source,
    debouncedValue: debouncedValue as Ref<T>,
    cancel,
    flush,
  };
}
