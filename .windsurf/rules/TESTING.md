# ESP-IDF 单元测试与CI/CD最佳实践

> **适用范围**: ESP32/ESP-IDF项目测试策略、CI/CD集成
> **性质**: 建议性规范，用于建立测试流程

---

## 🧪 单元测试策略

### 1. 测试框架选择

ESP-IDF官方推荐：Unity测试框架（已集成）

```cmake
# CMakeLists.txt中添加测试
idf_component_register(
    SRCS "my_component.c"
    INCLUDE_DIRS "include"
    REQUIRES unity  # 添加测试依赖
)
```

### 2. 组件级单元测试结构

```
components/
└── my_component/
    ├── include/
    ├── my_component.c
    ├── my_component.h
    └── test/                      # 测试目录
        ├── CMakeLists.txt
        └── test_my_component.c    # 测试文件
```

### 3. 单元测试编写规范

```cpp
// test/test_wifi_manager.c
#include "unity.h"
#include "wifi_manager.h"

// 测试前置准备
TEST_CASE("wifi_manager_init_should_succeed", "[wifi_manager]")
{
    // Arrange: 准备测试环境
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_erase());
        TEST_ASSERT_EQUAL(ESP_OK, nvs_flash_init());
    }
    
    // Act: 执行被测函数
    err = wifi_manager_init();
    
    // Assert: 验证结果
    TEST_ASSERT_EQUAL(ESP_OK, err);
}

TEST_CASE("wifi_validate_ssid_should_reject_empty", "[wifi_manager]")
{
    // Arrange
    const char* empty_ssid = "";
    
    // Act
    bool result = wifi_validate_ssid(empty_ssid);
    
    // Assert
    TEST_ASSERT_FALSE(result);
}

TEST_CASE("wifi_validate_ssid_should_accept_valid", "[wifi_manager]")
{
    // Arrange
    const char* valid_ssid = "MyNetwork";
    
    // Act
    bool result = wifi_validate_ssid(valid_ssid);
    
    // Assert
    TEST_ASSERT_TRUE(result);
}
```

### 4. 测试运行方法

```bash
# 方式1：在目标设备上运行测试
idf.py -p COM5 flash monitor  # 固件自动运行测试

# 方式2：在Linux主机上运行（模拟器）
idf.py test

# 方式3：仅构建测试
idf.py build
```

---

## 🔄 CI/CD集成

### 1. GitHub Actions工作流示例

```yaml
# .github/workflows/esp32-ci.yml
name: ESP32 CI

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Setup ESP-IDF
        uses: espressif/esp-idf-ci-action@v1
        with:
          esp_idf_version: v5.1.2
          target: esp32
          
      - name: Build project
        run: |
          . $IDF_PATH/export.sh
          idf.py build
          
      - name: Static analysis
        run: |
          . $IDF_PATH/export.sh
          idf.py cppcheck
          
      - name: Check code formatting
        run: |
          find components -name "*.c" -o -name "*.cpp" -o -name "*.h" | \
            xargs clang-format --dry-run --Werror

  test:
    runs-on: ubuntu-latest
    needs: build
    steps:
      - uses: actions/checkout@v4
      
      - name: Run unit tests on simulator
        uses: espressif/esp-idf-ci-action@v1
        with:
          esp_idf_version: v5.1.2
          target: esp32
          command: idf.py test
```

### 2. 预提交钩子

```bash
# .pre-commit-config.yaml
repos:
  - repo: local
    hooks:
      - id: esp32-clang-format
        name: ESP32 Code Format
        entry: clang-format
        language: system
        files: '\.(c|cpp|h)$'
        args: ['-i']
        
      - id: esp32-cppcheck
        name: ESP32 Static Analysis
        entry: cppcheck
        language: system
        files: '\.(c|cpp)$'
        args: ['--error-exitcode=1', '--enable=all']
```

---

## 📊 测试覆盖要求

### 最低覆盖率标准

| 组件类型 | 覆盖率要求 | 关键路径要求 |
|---------|-----------|-------------|
| 核心组件 | ≥80% | 100% |
| 业务逻辑 | ≥70% | ≥90% |
| 工具函数 | ≥60% | ≥80% |
| UI/Web | ≥50% | ≥70% |

### 覆盖率检查

```bash
# 使用gcov生成覆盖率报告
# 需要开启编译选项：-fprofile-arcs -ftest-coverage

# 1. 开启覆盖率编译
idf.py build --coverage

# 2. 运行测试
idf.py -p COM5 flash monitor

# 3. 生成报告
# 使用gcovr或lcov工具
```

---

## 🎯 测试分类

### 1. 单元测试（Unit Test）
- 测试单个函数或模块
- 不依赖硬件
- 快速执行
- 可模拟依赖

### 2. 集成测试（Integration Test）
- 测试模块间交互
- 需要部分硬件
- 测试API契约
- 数据流验证

### 3. 系统测试（System Test）
- 完整固件测试
- 需要实际硬件
- 端到端场景
- 性能测试

---

## ✅ AI代码修改测试清单

AI在修改代码后应执行：

```markdown
- [ ] **影响分析**: 修改影响哪些模块？
- [ ] **单元测试**: 相关单元测试是否通过？
- [ ] **回归测试**: 未修改功能是否正常？
- [ ] **边界测试**: 边界条件是否处理正确？
- [ ] **错误测试**: 错误路径是否测试？
- [ ] **内存测试**: Valgrind/asan无内存错误？
- [ ] **构建测试**: 能否成功编译？
```

---

## 🔧 测试辅助工具

### ESP-IDF监视器测试模式

```bash
# 自动运行测试并捕获结果
idf.py -p COM5 flash monitor | tee test_output.log

# 检查结果
grep -E "(PASS|FAIL|Tests)" test_output.log
```

### 断言使用指南

```cpp
// ✅ 正确：有意义的断言消息
TEST_ASSERT_NOT_NULL(ptr, "Memory allocation failed for buffer");
TEST_ASSERT_EQUAL(ESP_OK, err, "WiFi init failed: %s", esp_err_to_name(err));

// ❌ 错误：无意义的断言
TEST_ASSERT_TRUE(true);  // 这没意义
```

---

*版本：1.0 | 创建日期：2026-04-14 | 适用：ESP32/ESP-IDF项目测试*
