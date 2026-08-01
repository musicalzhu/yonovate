# Yonovate V2 - ESP32 IoT Control System

基于 ESP32 的STEM教学系统，支持 GPIO 控制、电机控制、状态监控等功能。

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
│   ├── test_app_main.cpp # 测试应用
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
│   ├── state_machine/    # V2状态机实现（兼容性）
│   ├── state_machine_v3/ # V3状态机实现（推荐）
│   ├── nvs_manager/      # NVS存储管理
│   ├── logic_circuit/    # 逻辑电路功能
│   ├── system_status/    # 系统状态管理
│   └── wifi_config_store/ # WiFi配置存储
├── web_project/          # 前端 Web 项目（前后端分离）
│   ├── index.html        # 主页
│   ├── css/              # 样式文件
│   ├── js/               # JavaScript 文件
│   └── pages/            # 功能页面
├── openspec/             # 项目规范文档
│   ├── specs/            # 功能规范
│   └── changes/          # 变更记录
├── docs/                 # 项目文档
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
- **Smart Dual Mode WiFi**: 同时启动AP配网和STA连接，30秒超时自动切换
- **AP配网模式**: SSID: `Yonovate-Setup`，密码: `12345678`，IP: `192.168.4.1`
- **WiFi客户端模式**: 自动连接保存的WiFi网络
- **内置Web服务器**: 端口80，支持配置页面和RESTful API
- **NVS配置存储**: 非易失性存储WiFi配置，重启后自动恢复
- **RESTful API接口**: 完整的GPIO、电机、状态机、WiFi配置API
- **CORS跨域支持**: 支持前端页面直接访问API

### 显示功能
- SSD1306 OLED 显示屏支持
- 实时显示系统状态、IP 地址等信息

### 状态机系统

#### V3状态机 (推荐)

**核心特性**：
- **多触发器组合**: 支持最多3个触发器的AND/OR逻辑
- **状态绑定效果**: 进入状态时自动执行绑定效果
- **完整JSON配置**: 表驱动设计，无需修改代码
- **前端可视化**: 支持Web界面配置和实时预览

**触发器类型**：
- **GPIO边沿触发** (`SM_TRIGGER_EDGE`): 检测电平变化，支持上升沿/下降沿/任意边沿
- **GPIO电平触发** (`SM_TRIGGER_LEVEL`): 基于电平值比较，支持6种运算符(==, <, >, <=, >=, !=)
- **定时器触发** (`SM_TRIGGER_TIMER`): 延时到期触发，支持毫秒级精度
- **手动触发** (`SM_TRIGGER_MANUAL`): 通过API或HTTP接口手动触发

**效果类型**：
- **GPIO输出控制**: 设置指定GPIO引脚的高低电平
- **电机控制**: 控制电机正转、反转、停止
- **定时器启动**: 启动延时定时器，支持"进入状态后延时转换"场景

**配置格式**：
```json
{
  "transitions": [
    {
      "current_state": "IDLE",
      "next_state": "ACTIVE",
      "triggers": [
        {"type": 1, "source": "P1", "op": 0, "value": 1}
      ],
      "trigger_count": 1,
      "trigger_logic": 0,
      "stateEffects": [
        {"type": 0, "target": "P2", "value": 1}
      ]
    }
  ]
}
```

#### V2状态机 (兼容性)

**核心特性**：
- **简单GPIO触发**: 基础的GPIO输入检测
- **代码配置**: 通过代码定义状态转换
- **基础状态转换**: 适合简单的控制逻辑

**适用场景**：
- 学习状态机基本概念
- 简单的自动化控制
- 向后兼容性保证

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

### 配置WiFi

1. **首次启动或恢复出厂设置**
   - ESP32自动启动AP模式，SSID: `Yonovate-Setup`，密码: `12345678`
   - 连接到该WiFi网络

2. **打开配置页面**
   - 浏览器访问: `http://192.168.4.1/`
   - 输入目标WiFi的SSID和密码
   - 点击"保存配置"

3. **配置保存流程**
   - 配置保存到NVS（非易失性存储）
   - **需重启设备**才能应用新的WiFi设置
   - 重启后自动尝试连接指定WiFi
   - 连接成功后自动关闭AP模式

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
