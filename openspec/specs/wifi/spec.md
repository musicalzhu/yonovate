# WiFi 组件规范

## 概述

ESP32 WiFi 管理组件，提供统一的 WiFi 连接管理和配置存储功能。

**设计目标**：简化 WiFi 管理复杂度，提供清晰的两阶段初始化模式（init → start），统一 NVS 配置管理。

**核心特性**：
- 两阶段初始化：`wifi_manager_init()` 硬件初始化 + `wifi_manager_start()` 启动连接
- Smart Dual Mode：同时启动 AP 配网和 STA 连接，30秒超时自动切换
- 统一 NVS 配置管理：`wifi_config_store` 组件封装所有配置存储逻辑
- 事件驱动状态管理：STA 和 AP 状态通过 ESP-IDF 事件机制同步
- 状态回调系统：多订阅者 WiFi 状态变化通知机制

---

## 架构决策

| 决策 | 说明 | 理由 |
|------|------|------|
| 分离 init() 与 start() | `wifi_manager_init()` 仅初始化硬件，`wifi_manager_start()` 加载配置并启动 | 遵循组件初始化模式，支持测试 mock |
| 统一 NVS 封装 | 创建 `wifi_config_store` 组件封装所有 WiFi 配置存储 | 消除重复代码，确保键名一致性 |
| Smart Dual Mode | 同时启动 AP + STA，30秒超时后决定是否关闭 AP | 配网入口立即可用，用户体验最佳 |

---

## 核心组件

### WiFi Manager

#### 初始化流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        WiFi Manager 初始化流程                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   main.cpp                                                                  │
│       │                                                                     │
│       ├──► nvs_mgr_init()                                                   │
│       │                                                                     │
│       └──► net_init()                                                       │
│               │                                                             │
│               ├──► wifi_manager_init()      // 硬件初始化                  │
│               │       ├── esp_netif_init()                                │
│               │       ├── esp_wifi_init()                                 │
│               │       └── 注册事件处理器                                  │
│               │                                                             │
│               └──► wifi_manager_start()     // 启动连接流程              │
│                       │                                                     │
│                       ├──► wifi_config_store_load()    // 从NVS读配置     │
│                       ├──► wifi_manager_update_credentials()            │
│                       ├──► wifi_manager_start_ap_provisioning()          │
│                       └──► 等待STA连接或30秒超时                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### 状态机

```
                    ┌──────────────┐
                    │  DISCONNECTED │
                    └──────┬───────┘
                           │
              esp_wifi_connect()
                           ▼
                    ┌──────────────┐
                    │  CONNECTING   │
                    └──────┬───────┘
                           │
           ┌───────────────┼───────────────┐
           │               │               │
           ▼               ▼               ▼
    ┌──────────┐   ┌──────────┐   ┌──────────┐
    │ CONNECTED│   │AP_MODE   │   │DISCONNECTED
    │ (STA OK) │   │ (配网)   │   │(连接失败) │
    └──────────┘   └──────────┘   └──────────┘
```

#### 状态枚举

```c
typedef enum {
    WIFI_STATUS_IDLE,               /*!< 空闲状态 */
    WIFI_STATUS_DISCONNECTED,       /*!< 已断开连接 */
    WIFI_STATUS_CONNECTING,         /*!< 正在连接 */
    WIFI_STATUS_CONNECTED,          /*!< STA 已连接（有 IP） */
    WIFI_STATUS_AP_MODE             /*!< AP 模式启动 */
} wifi_status_t;
```

---

### WiFi Config Store

#### API 规范

| 函数 | 说明 | 参数 |
|------|------|------|
| `wifi_config_store_load()` | 从 NVS 加载 WiFi 配置 | `ssid, ssid_len, pass, pass_len` |
| `wifi_config_store_save()` | 保存 WiFi 配置到 NVS | `ssid, password` |
| `wifi_config_store_erase()` | 擦除 NVS 中的 WiFi 配置 | - |

#### NVS 键名

| 键 | 命名空间 | 说明 |
|----|----------|------|
| `ssid` | `wifi_cfg` | WiFi SSID |
| `pass` | `wifi_cfg` | WiFi 密码（脱敏日志） |

---

## 数据流

### WiFi 配置保存流程

```
前端配网页面
    │ POST /api/wifi/config {"ssid":"xxx","pass":"yyy"}
    ▼
web_server.cpp: wifi_config_handler()
    │ wifi_config_store_save(ssid, pass)
    ▼
wifi_config_store.cpp
    │ nvs_mgr_set_string("wifi_cfg", "ssid", ssid)
    │ nvs_mgr_set_string("wifi_cfg", "pass", pass)
    ▼
nvs_manager.c
    │ NVS 写入 Flash
    ▼
NVS 分区
```

### WiFi 配置加载流程（重启后）

```
main.cpp: app_main()
    │ nvs_mgr_init()
    ▼
net_init()
    │ wifi_manager_init()
    │ wifi_manager_start()
    ▼
wifi_manager.cpp: wifi_manager_start()
    │ wifi_config_store_load(ssid, pass)
    ▼
wifi_config_store.cpp
    │ nvs_mgr_get_string("wifi_cfg", "ssid", ...)
    │ nvs_mgr_get_string("wifi_cfg", "pass", ...)
    ▼
返回配置给 wifi_manager_start()
    │ wifi_manager_update_credentials(ssid, pass)
    ▼
STA 使用配置连接 WiFi
```

---

## API 规范

### WiFi Manager API

```c
// 初始化与启动
esp_err_t wifi_manager_init(void);
void wifi_manager_start(void);

// 状态查询与回调
wifi_status_t wifi_manager_get_status(void);
int wifi_manager_register_status_callback(wifi_status_cb_t cb, void *user_arg);

// AP 配网控制
void wifi_manager_start_ap_provisioning(void);
void wifi_manager_stop_ap_provisioning(void);

// 配置更新
esp_err_t wifi_manager_update_credentials(const char *ssid, const char *password);
```

### WiFi Config Store API

```c
esp_err_t wifi_config_store_load(char *ssid, size_t ssid_len, 
                                  char *pass, size_t pass_len);
esp_err_t wifi_config_store_save(const char *ssid, const char *password);
esp_err_t wifi_config_store_erase(void);
```

---

## 关键设计决策详情

### Decision 1: Smart Dual Mode 启动策略

**Context**: 两种启动策略可选：(A) 同时启动 AP+STA (B) 先STA 30秒，失败再AP

**Decision**: 采用方案 A：同时启动 AP 配网模式和 STA 连接尝试。

**Trade-offs**:
- ✅ 用户体验最佳
- ✅ 失败恢复无需等待
- ⚠️ 启动初期功耗略高（双模式）

---

## 测试验证

### 关键测试场景

1. **首次启动（无NVS配置）**
   - AP 热点立即可见
   - 可连接 AP 访问配网页面

2. **配置保存后重启**
   - AP 仍然可见（Smart Dual Mode）
   - STA 尝试连接保存的配置
   - 30秒内连接成功 → AP 自动关闭

3. **配置错误重启**
   - AP 始终可见
   - STA 连接失败后保持 AP 模式
   - 用户可重新配网无需等待

---

