/**
 * Vue 3.5+ 计数器 Store (Pinia Setup 语法)
 *
 * 简单的计数器状态管理，用于学习 Pinia 的基本用法
 */
import { defineStore } from 'pinia';
import { ref, computed } from 'vue';

/**
 * 计数器 Store
 */
export const useCounterStore = defineStore('counter', () => {
  // 状态
  const count = ref(0);
  const min = ref(0);
  const max = ref(100);
  const step = ref(1);

  // 计算属性
  const isMax = computed(() => count.value >= max.value);
  const isMin = computed(() => count.value <= min.value);
  const percentage = computed(() => {
    if (max.value === min.value) return 0;
    return ((count.value - min.value) / (max.value - min.value)) * 100;
  });

  // 方法
  /**
   * 增加计数
   * @param customStep - 自定义步长（可选）
   */
  /**
   * 按当前步长或指定值增加计数。
   */
  const increment = (customStep?: number) => {
    const actualStep = customStep ?? step.value;
    count.value = Math.min(count.value + actualStep, max.value);
  };

  /**
   * 减少计数
   * @param customStep - 自定义步长（可选）
   */
  /**
   * 按当前步长或指定值减少计数。
   */
  const decrement = (customStep?: number) => {
    const actualStep = customStep ?? step.value;
    count.value = Math.max(count.value - actualStep, min.value);
  };

  /**
   * 重置计数
   */
  /**
   * 将计数重置为最小值。
   */
  const reset = () => {
    count.value = min.value;
  };

  /**
   * 设置计数值
   * @param value - 新的计数值
   */
  /**
   * 将计数设置为指定范围内的合法值。
   */
  const setValue = (value: number) => {
    count.value = Math.max(min.value, Math.min(value, max.value));
  };

  /**
   * 设置边界
   * @param newMin - 新的最小值
   * @param newMax - 新的最大值
   */
  /**
   * 同时更新计数上下限，并保证当前值仍然落在合法范围内。
   */
  const setBounds = (newMin: number, newMax: number) => {
    min.value = newMin;
    max.value = newMax;
    // 确保当前值在新的边界内
    count.value = Math.max(newMin, Math.min(count.value, newMax));
  };

  /**
   * 设置步长
   * @param newStep - 新的步长
   */
  /**
   * 设置计数器的步长，最小值至少为 1。
   */
  const setStep = (newStep: number) => {
    step.value = Math.max(1, newStep);
  };

  // 返回 store 接口
  return {
    // 状态
    count,
    min,
    max,
    step,

    // 计算属性
    isMax,
    isMin,
    percentage,

    // 方法
    increment,
    decrement,
    reset,
    setValue,
    setBounds,
    setStep,
  };
});
