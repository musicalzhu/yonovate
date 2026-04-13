---
description: ESP32烧录测试日志监控规范
tags: [esp32, monitoring, logging, test]
---

# ESP32烧录测试日志监控规范

## 烧录测试中的日志监控（AI必须执行）

### 1. 烧录命令（用户确认后执行）

```bash
idf.py -p [PORT] flash monitor
```

**注意**：`monitor`参数会自动在烧录后启动串口监控，这是必需的。

### 2. 必须监控的日志序列

#### WiFi智能双模式启动检查点

| 时间顺序 | 必须看到的日志 | 含义 | 如果没出现 |
|---------|---------------|------|-----------|
| 1 | `========== WiFi Smart Dual Mode Initializing ==========` | Smart dual mode started | ⚠️ Firmware not flashed or boot failed |
| 2 | `NVS init done` | Config storage OK | ⚠️ NVS corrupted, need erase-flash |
| 3 | `WiFi config params` (shows SSID) | Config loaded | ⚠️ Config may be lost |
| 4 | `APSTA dual mode started` | AP started | ⚠️ WiFi init failed |
| 5 | `Waiting for STA connection result...` | Trying to connect | Waiting... |

#### 连接结果分支检查

**Success Path**:
```
[OK] STA connected successfully! IP: xxx.xxx.xxx.xxx
[OK] Current mode: STA only mode (connected to router)
```

**Failure Path**:
```
[FAIL] STA connection timeout (30000 ms)
[WARN] Current mode: AP provisioning mode (waiting for config)
Please connect WiFi: Yonovate-Setup
Visit http://192.168.4.1 for configuration
```

#### Web服务器检查点

| 日志 | 含义 | 如果没出现 |
|------|------|-----------|
| `HTTP Server started on port: 80` | Web server started | ⚠️ Server start failed |
| `HTTP server started (AP provisioning mode)` | Config page available | ⚠️ Cannot access 192.168.4.1 |

#### API请求检查点

When saving config, must see:
```
Received WiFi config POST request, content_len=XX
Saving WiFi config to NVS: SSID=XXXX
WiFi credentials updated, connecting with new config...
```

### 3. 错误日志立即报告

**如果出现以下日志，立即停止并报告**：

| 错误日志 | 问题 | 建议操作 |
|---------|------|---------|
| `esp_err_wifi_mode` | WiFi模式切换失败 | 检查是否在AP模式下配置STA |
| `DHCP failed` | DHCP服务器启动失败 | 检查WiFi初始化顺序 |
| `HTTP server start failed` | Web服务器失败 | 检查端口占用（80） |
| `NVS init failed` | Storage corrupted | Run `idf.py erase-flash` |
| `out of memory` | Out of memory | Reduce task stack size |

### 4. 日志监控流程

```
User: Confirm flash
    ↓
AI: Execute flash + monitor command
    ↓
Wait for device reboot
    ↓
Check log sequence [1] → [2] → [3] → [4] → [5]
    ↓
Branch:
  ├─ All checkpoints passed → Report WiFi mode (STA or AP)
  └─ Any checkpoint failed → Report error and logs immediately
```

### 5. 正确的日志报告格式

```
[OK] Flash successful!
📊 Log Monitor Report:

Startup Check:
  ✓ [1/5] Smart dual mode started
  ✓ [2/5] NVS init done
  ✓ [3/5] WiFi config params (SSID: XINGHUI)
  ✓ [4/5] APSTA dual mode started
  ✓ [5/5] Waiting for STA connection...

Connection Result:
  ✗ STA connection timeout (30s)
  → Enter AP provisioning mode

Provisioning Info:
  SSID: Yonovate-Setup
  Password: 12345678
  Admin Page: http://192.168.4.1
```

### 6. 常见启动日志问题诊断

**Issue 1: No WiFi related logs**
```
Cause: WiFi init code in main.cpp not executed
Check: Is wifi_manager_start_smart_dual_mode() called?
```

**Issue 2: AP started but no IP shown**
```
Cause: AP config incomplete
Check: esp_wifi_set_config() and esp_wifi_start() order
```

**Issue 3: STA keeps connecting**
```
Cause: Wrong password or weak signal
Advice: Enter AP mode to reconfig, or check router signal
```

**Issue 4: HTTP server start failed**
```
Cause: Port occupied or out of memory
Check: Is port 80 used by other tasks?
Solution: Reduce httpd_config_t.max_uri_handlers
```

### 7. Manual Verification Steps

If AI monitoring finds issues, user can verify manually:

```bash
# 1. Enter monitor mode
idf.py -p [PORT] monitor

# 2. Press Ctrl+] to exit, then check specific logs
grep -E "(WiFi|AP|STA|HTTP|ERROR)" build/log/idf_py_stdout_output_*

# 3. If needed, erase flash and re-flash
idf.py -p [PORT] erase-flash flash
```
