# ESP32 前后端分离使用说明

## 概述
本项目采用前后端分离架构：
- **前端**: 本地 HTML/CSS/JavaScript 文件（可离线打开）
- **后端**: ESP32 设备上运行的 Web 服务器

## 使用步骤

### 1. 启动 ESP32 设备
确保 ESP32 已烧录固件并连接到 WiFi 网络。

### 2. 获取 ESP32 IP 地址
通过串口监视器查看 ESP32 的 IP 地址：
```bash
cd [项目根目录]
idf.py -p [串口设备] monitor
# Windows示例: idf.py -p COM5 monitor
# Linux示例: idf.py -p /dev/ttyUSB0 monitor
```

在日志中找到类似这样的信息：
```
I (4177) wifi_manager: WiFi connection successful, IP: [IP地址]
# 示例: 192.168.1.100
```

### 3. 配置前端 IP 地址

#### 方法 1: 修改配置文件（推荐）
编辑 `web_project/js/config.js`：
```javascript
const ESP32_CONFIG = {
    IP: '[IP地址]',  // 修改为ESP32实际IP地址
    PORT: 80,
};
```

#### 方法 2: 在网页中输入
1. 在浏览器中打开 `web_project/pages/gpio_control.html`
2. 在页面顶部的 "ESP32 IP Address" 输入框中输入 IP 地址
3. 点击 "Test Connection" 测试连接
4. IP 地址会自动保存到浏览器的 localStorage

### 4. 打开前端页面
直接在浏览器中打开本地文件：
```
file:///[项目路径]/web_project/pages/gpio_control.html
# Windows示例: file:///C:/Projects/yonovate/web_project/pages/gpio_control.html
```

或者在文件管理器中双击打开 `gpio_control.html`

### 5. 测试连接
点击页面上的 "Test Connection" 按钮，确认能够连接到 ESP32。

## 功能说明

### GPIO 控制
- 支持 12 个 GPIO 组（4 个原生 GPIO 组 + 8 个 AW9523 扩展组）
- 每组有 2 个 GPIO 引脚
- 可设置为输入或输出模式
- 输出模式下可控制电平高低

### 电机控制
- 支持 2 个电机（M1 和 M2）
- M1: DIR=GPIO18, PWM=GPIO19
- M2: DIR=GPIO23, PWM=GPIO16
- 控制选项：正转、反转、停止

## 跨域访问 (CORS)
ESP32 后端已配置允许所有来源的跨域访问：
```
Access-Control-Allow-Origin: *
```

这使得本地 HTML 文件可以直接访问 ESP32 的 API。

## 故障排查

### 问题 1: 连接失败
**原因**: ESP32 IP 地址不正确或设备未启动
**解决**: 
1. 检查 ESP32 是否已连接到 WiFi
2. 确认 IP 地址是否正确
3. 确保电脑和 ESP32 在同一网络

### 问题 2: 405 Method Not Allowed
**原因**: API 端点路径错误
**解决**: 检查前端 JavaScript 中的 URL 是否正确

### 问题 3: CORS 错误
**原因**: 浏览器阻止跨域请求
**解决**: 
1. 确认后端已设置正确的 CORS 头
2. 重新编译并烧录固件

## API 端点

### GPIO 控制
- **POST** `/gpio` - 同步 GPIO 配置
- **GET** `/gpio/state` - 获取 GPIO 状态

### 电机控制
- **POST** `/motor` - 控制电机
  ```json
  {
    "motor_id": 0,
    "direction": 1
  }
  ```
  - motor_id: 0 或 1
  - direction: 0=停止, 1=正转, 2=反转

### 系统状态
- **GET** `/status` - 获取系统状态

## 开发说明
如需修改前端代码：
1. 编辑 `web_project/` 目录下的文件
2. 直接在浏览器中刷新页面即可看到效果
3. 无需重新编译或烧录 ESP32

## 注意事项
1. 确保电脑和 ESP32 在同一局域网
2. 部分浏览器可能会阻止 `file://` 协议的某些功能
3. 建议使用 Chrome 或 Firefox 浏览器
4. IP 地址会保存在浏览器 localStorage 中，下次自动填充
