/**
 * Vue 3.5+ 计数器组合式函数
 * 提供基础的计数功能，支持增减、重置和设置值
 *
 * @example
 * ```typescript
 * const { count, increment, decrement, reset, setValue } = useCounter(0)
 * ```
 */
import { ref, computed, type Ref, type ComputedRef } from 'vue';

/**
 * 计数器选项
 */
export interface UseCounterOptions {
  /** 初始值（默认 0） */
  initialValue?: number;
  /** 最小值（可选） */
  min?: number;
  /** 最大值（可选） */
  max?: number;
  /** 步长（默认 1） */
  step?: number;
}

/**
 * 计数器返回值
 */
export interface UseCounterReturn {
  /** 当前计数值 */
  count: Ref<number>;
  /** 增加计数 */
  increment: (step?: number) => void;
  /** 减少计数 */
  decrement: (step?: number) => void;
  /** 重置计数 */
  reset: () => void;
  /** 设置值 */
  setValue: (value: number) => void;
  /** 是否达到最大值 */
  isMax: ComputedRef<boolean>;
  /** 是否达到最小值 */
  isMin: ComputedRef<boolean>;
}

/**
 * 计数器 Composable
 * @param options - 配置选项
 * @returns 计数器状态和控制方法
 */
export function useCounter(options: UseCounterOptions = {}): UseCounterReturn {
  const { initialValue = 0, min, max, step: defaultStep = 1 } = options;

  const count = ref<number>(initialValue);

  // 计算是否达到边界
  const isMax = computed(() => max !== undefined && count.value >= max);
  const isMin = computed(() => min !== undefined && count.value <= min);

  /**
   * 增加计数
   * @param step - 步长（可选，默认使用配置的步长）
   */
  const increment = (step?: number) => {
    const actualStep = step ?? defaultStep;
    if (max !== undefined) {
      count.value = Math.min(count.value + actualStep, max);
    } else {
      count.value += actualStep;
    }
  };

  /**
   * 减少计数
   * @param step - 步长（可选，默认使用配置的步长）
   */
  const decrement = (step?: number) => {
    const actualStep = step ?? defaultStep;
    if (min !== undefined) {
      count.value = Math.max(count.value - actualStep, min);
    } else {
      count.value -= actualStep;
    }
  };

  /**
   * 重置计数到初始值
   */
  const reset = () => {
    count.value = initialValue;
  };

  /**
   * 设置计数值
   * @param value - 新的计数值
   */
  const setValue = (value: number) => {
    if (min !== undefined && value < min) {
      count.value = min;
    } else if (max !== undefined && value > max) {
      count.value = max;
    } else {
      count.value = value;
    }
  };

  return {
    count,
    increment,
    decrement,
    reset,
    setValue,
    isMax,
    isMin,
  };
}
