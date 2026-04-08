// ESP32 设备配置文件
// 使用说明：
// 1. 打开 ESP32 串口监视器，查看设备的 IP 地址
// 2. 将下面的 ESP32_IP 修改为您的 ESP32 实际 IP 地址
// 3. 保存文件后，刷新网页即可

const ESP32_CONFIG = {
    // ESP32 设备的 IP 地址（请根据实际情况修改）
    IP: '192.168.3.55',
    
    // API 端口（默认为 80）
    PORT: 80,
    
    // 获取完整 URL
    getBaseURL: function() {
        return this.PORT === 80 
            ? `http://${this.IP}` 
            : `http://${this.IP}:${this.PORT}`;
    }
};

// 导出配置（用于其他 JS 文件）
window.ESP32_CONFIG = ESP32_CONFIG;
