# Yonovate V2 - ESP32 IoT Control System

基于 ESP32 的物联网控制系统，支持 GPIO 控制、电机控制、状态监控等功能。

## 项目概述

本项目是一个完整的 ESP32 IoT 解决方案，采用模块化设计，包含多个功能组件：

- **硬件控制**: GPIO 管理、电机控制、I2C 设备管理
- **网络连接**: WiFi 管理、Web 服务器
- **用户界面**: OLED 显示、前后端分离的 Web 界面
- **系统管理**: 状态机、逻辑电路、系统状态监控

## 目录结构

```
yonovate-v2/
├── main/                  # 主程序入口
│   ├── main.cpp          # 主程序代码
│   └── CMakeLists.txt    # 主程序构建配置
├── components/           # 功能组件
│   ├── wifi_manager/     # WiFi 连接管理
│   ├── web_server/       # HTTP Web 服务器
│   ├── gpio_manager/     # GPIO 输出控制
│   ├── gpio_monitor/     # GPIO 输入监控
│   ├── gpio_control/     # GPIO 综合控制
│   ├── i2c_manager/      # I2C 总线管理
│   ├── motor_control/    # 电机控制
│   ├── ssd1306/          # OLED 显示驱动
│   ├── aw9523/           # AW9523 GPIO 扩展芯片驱动
│   ├── state_machine/    # 状态机实现
│   ├── logic_circuit/    # 逻辑电路功能
│   ├── system_status/    # 系统状态管理
│   └── web_common/       # Web 公共模块
├── web_project/          # 前端 Web 项目（前后端分离）
│   ├── index.html        # 主页
│   ├── css/              # 样式文件
│   ├── js/               # JavaScript 文件
│   └── pages/            # 功能页面
├── CMakeLists.txt        # 项目构建配置
└── README.md             # 项目说明文档
```

## 功能特性

### GPIO 控制
- 支持 12 个 GPIO 组（4 个原生 GPIO + 8 个 AW9523 扩展）
- 每组包含 2 个 GPIO 引脚
- 可配置为输入或输出模式
- 输出模式下可控制电平高低

### 电机控制
- 支持 2 个直流电机
- **M1**: DIR=GPIO18, PWM=GPIO19
- **M2**: DIR=GPIO23, PWM=GPIO16
- 控制模式：正转、反转、停止

### 网络功能
- WiFi 客户端模式
- 内置 Web 服务器（端口 80）
- RESTful API 接口
- CORS 跨域支持

### 显示功能
- SSD1306 OLED 显示屏支持
- 实时显示系统状态、IP 地址等信息

## 快速开始

### 环境要求

- ESP-IDF v5.0 或更高版本
- CMake 3.5+
- ESP32 开发板
- Python 3.x

### 编译和烧录

```bash
# 设置 ESP-IDF 环境
. $IDF_PATH/export.sh

# 配置项目（可选）
idf.py menuconfig

# 编译项目
idf.py build

# 烧录到 ESP32（替换 /dev/ttyUSB0 为你的串口）
idf.py -p /dev/ttyUSB0 flash

# 查看串口输出
idf.py -p /dev/ttyUSB0 monitor
```

### 配置 WiFi

在 `main/main.cpp` 中修改 WiFi 配置：

```cpp
static const char* WIFI_SSID = "你的WiFi名称";
static const char* WIFI_PASS = "你的WiFi密码";
```

## Web 前端使用

项目采用前后端分离架构，前端可以独立运行。

### 步骤

1. **启动 ESP32** 并获取 IP 地址（通过串口监视器）

2. **配置前端 IP** - 两种方法：
   - 编辑 `web_project/js/config.js` 设置 IP 地址
   - 或在网页中输入框直接输入

3. **打开前端页面**:
   ```
   file:///path/to/web_project/pages/gpio_control.html
   ```

4. **测试连接** - 点击页面上的 "Test Connection" 按钮

详细说明请参考 [web_project/README.md](web_project/README.md)

## API 接口

### GPIO 控制

| 方法 | 端点 | 描述 |
|------|------|------|
| POST | `/gpio` | 同步 GPIO 配置 |
| GET | `/gpio/state` | 获取 GPIO 状态 |

### 电机控制

| 方法 | 端点 | 描述 |
|------|------|------|
| POST | `/motor` | 控制电机 |

请求示例：
```json
{
  "motor_id": 0,
  "direction": 1
}
```
- `motor_id`: 0 或 1
- `direction`: 0=停止，1=正转，2=反转

### 系统状态

| 方法 | 端点 | 描述 |
|------|------|------|
| GET | `/status` | 获取系统状态 |

## 组件说明

| 组件 | 功能描述 |
|------|----------|
| `wifi_manager` | WiFi 连接管理，支持自动重连 |
| `web_server` | HTTP 服务器，提供 REST API |
| `gpio_manager` | GPIO 输出控制管理 |
| `gpio_monitor` | GPIO 输入状态监控 |
| `i2c_manager` | I2C 总线主机管理 |
| `ssd1306` | SSD1306 OLED 显示驱动 |
| `aw9523` | AW9523 I2C GPIO 扩展芯片驱动 |
| `motor_control` | 直流电机控制 |
| `state_machine` | 系统状态机实现 |
| `logic_circuit` | 逻辑电路功能模拟 |
| `system_status` | 系统状态信息收集和管理 |

## 故障排查

### 常见问题

1. **连接失败**
   - 检查 ESP32 是否已连接到 WiFi
   - 确认 IP 地址是否正确
   - 确保电脑和 ESP32 在同一网络

2. **编译错误**
   - 确认 ESP-IDF 环境变量已正确设置
   - 检查 CMake 版本是否满足要求

3. **405 Method Not Allowed**
   - 检查 API 请求方法和路径是否正确

4. **CORS 错误**
   - 确认后端已设置正确的 CORS 头
   - 重新编译并烧录固件

## 开发指南

### 添加新组件

1. 在 `components/` 目录下创建新组件文件夹
2. 创建组件的 `Kconfig` 和 `CMakeLists.txt`
3. 在根目录 `CMakeLists.txt` 中添加组件路径

### 修改前端代码

1. 编辑 `web_project/` 目录下的文件
2. 直接在浏览器中刷新页面查看效果
3. 无需重新编译 ESP32 固件

## 注意事项

1. 确保电脑和 ESP32 在同一局域网
2. 部分浏览器可能会限制 `file://` 协议的某些功能
3. 建议使用 Chrome 或 Firefox 浏览器
4. 首次使用需要配置正确的 WiFi 凭据

## 许可证

本项目采用开源许可证，具体请参考 LICENSE 文件（如有）。

## 贡献

欢迎提交 Issue 和 Pull Request！

## 联系方式

如有问题或建议，请通过 Issue 系统联系。
