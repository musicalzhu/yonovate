import { describe, it, expect } from 'vitest';
import { useCounter } from '../useCounter';

describe('useCounter', () => {
  it('应该初始化为默认值 0', () => {
    const { count } = useCounter();
    expect(count.value).toBe(0);
  });

  it('应该支持自定义初始值', () => {
    const { count } = useCounter({ initialValue: 10 });
    expect(count.value).toBe(10);
  });

  it('应该增加计数', () => {
    const { count, increment } = useCounter();
    increment();
    expect(count.value).toBe(1);
    increment(5);
    expect(count.value).toBe(6);
  });

  it('应该减少计数', () => {
    const { count, decrement } = useCounter({ initialValue: 10 });
    decrement();
    expect(count.value).toBe(9);
    decrement(3);
    expect(count.value).toBe(6);
  });

  it('应该遵守最小值限制', () => {
    const { count, decrement, isMin } = useCounter({ initialValue: 5, min: 0 });
    decrement(10);
    expect(count.value).toBe(0);
    expect(isMin.value).toBe(true);
  });

  it('应该遵守最大值限制', () => {
    const { count, increment, isMax } = useCounter({ initialValue: 5, max: 10 });
    increment(10);
    expect(count.value).toBe(10);
    expect(isMax.value).toBe(true);
  });

  it('应该重置到初始值', () => {
    const { count, increment, reset } = useCounter({ initialValue: 5 });
    increment(10);
    expect(count.value).toBe(15);
    reset();
    expect(count.value).toBe(5);
  });

  it('应该设置值', () => {
    const { count, setValue } = useCounter();
    setValue(20);
    expect(count.value).toBe(20);
  });

  it('设置值时应该遵守边界', () => {
    const { count, setValue } = useCounter({ min: 0, max: 10 });
    setValue(-5);
    expect(count.value).toBe(0);
    setValue(15);
    expect(count.value).toBe(10);
  });

  it('应该支持自定义步长', () => {
    const { count, increment, decrement } = useCounter({ step: 5 });
    increment();
    expect(count.value).toBe(5);
    decrement();
    expect(count.value).toBe(0);
  });

  it('应该支持临时覆盖步长', () => {
    const { count, increment } = useCounter({ step: 1 });
    increment(10);
    expect(count.value).toBe(10);
  });
});
