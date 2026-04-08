// 简化版GPIO控制脚本

// 动态获取 ESP32 URL
function getESP32URL() {
    // 1. 优先使用输入框中的 IP
    const ipInput = document.getElementById('esp32-ip');
    if (ipInput && ipInput.value) {
        const ip = ipInput.value.trim();
        if (ip) {
            return `http://${ip}`;
        }
    }
    
    // 2. 使用 config.js 中的配置
    if (window.ESP32_CONFIG) {
        return window.ESP32_CONFIG.getBaseURL();
    }
    
    // 3. 使用默认值
    return 'http://192.168.124.6';
}

let gpioConfig = {
    groupModes: {},
    outputStates: {}
};

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', function() {
    // 从 localStorage 加载保存的 IP 地址
    const savedIP = localStorage.getItem('esp32_ip');
    const ipInput = document.getElementById('esp32-ip');
    if (savedIP && ipInput) {
        ipInput.value = savedIP;
    } else if (window.ESP32_CONFIG && ipInput) {
        ipInput.value = window.ESP32_CONFIG.IP;
    }
    
    // 监听 IP 输入框变化
    if (ipInput) {
        ipInput.addEventListener('change', function() {
            localStorage.setItem('esp32_ip', this.value);
        });
    }
    
    initializeUI();
    
    // 尝试加载配置
    fetch(`${getESP32URL()}/api/gpio_config`)
        .then(response => response.json())
        .then(data => {
            console.log("Loaded config:", data);
            gpioConfig = data;
            updateUIFromConfig();
        })
        .catch(error => console.error('Error fetching GPIO config:', error));
});

// 测试连接功能
function testConnection() {
    const statusEl = document.getElementById('connection-status');
    statusEl.textContent = '正在测试连接...';
    statusEl.className = 'status';
    
    fetch(`${getESP32URL()}/status`)
        .then(response => response.json())
        .then(data => {
            statusEl.textContent = `连接成功！设备: ${data.device || 'ESP32'}, 状态: ${data.status || 'running'}`;
            statusEl.className = 'status success';
        })
        .catch(error => {
            statusEl.textContent = `连接失败: ${error.message}`;
            statusEl.className = 'status error';
        });
}

function initializeUI() {
    for (let i = 0; i < 12; i++) {
        const outputBtn = document.getElementById(`btn_mode_${i}_0`);
        const inputBtn = document.getElementById(`btn_mode_${i}_1`);
        if (outputBtn && inputBtn) {
            outputBtn.addEventListener('click', () => setGroupMode(i, 0));
            inputBtn.addEventListener('click', () => setGroupMode(i, 1));
        }
    }
}

// 设置组模式
function setGroupMode(group, mode) {
    if (gpioConfig.groupModes[group] === mode) return;
    
    gpioConfig.groupModes[group] = mode;
    console.log(`Group ${group} mode set to ${mode}`);
    
    updateModeUI(group);
    syncConfigToESP();
}

// 设置GPIO状态
function setGPIOState(group, pin, state) {
    if (!gpioConfig.outputStates[group]) {
        gpioConfig.outputStates[group] = {};
    }
    gpioConfig.outputStates[group][pin] = state;
    console.log(`Set GPIO state for group ${group}, pin ${pin} to ${state}`);
    
    updatePinUI(group, pin);
    syncConfigToESP();
}

// 控制电机状态
function setMotorState(motorId, direction) {
    console.log(`Setting motor ${motorId} to direction ${direction}`);
    
    fetch(`${getESP32URL()}/motor`, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify({ motor_id: motorId, direction: direction }),
    })
    .then(response => response.json())
    .then(data => {
        console.log('Motor control response:', data);
        if (data.status === 'success') {
            document.getElementById(`status_motor_${motorId}`).textContent = 'OK';
        } else {
            document.getElementById(`status_motor_${motorId}`).textContent = 'Error';
        }
        setTimeout(() => {
            document.getElementById(`status_motor_${motorId}`).textContent = '';
        }, 2000);
    })
    .catch((error) => {
        console.error('Error:', error);
        document.getElementById(`status_motor_${motorId}`).textContent = 'Failed';
        setTimeout(() => {
            document.getElementById(`status_motor_${motorId}`).textContent = '';
        }, 2000);
    });
}

// 同步配置到ESP32设备
function syncConfigToESP() {
    console.log("Syncing to ESP:", gpioConfig);
    fetch(`${getESP32URL()}/gpio`, {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        },
        body: JSON.stringify(gpioConfig),
    })
    .then(response => response.json())
    .then(data => {
        console.log('Sync response:', data);
        if (data.status === 'success') {
            console.log('Configuration updated successfully');
        } else {
            console.error('Failed to update configuration');
        }
        // Update status indicators based on response
        if (data.pin_states) {
            for (const group in data.pin_states) {
                for (const pin in data.pin_states[group]) {
                    const statusEl = document.getElementById(`status_group_${group}_${pin}`);
                    if (statusEl) {
                        statusEl.textContent = data.pin_states[group][pin].status;
                        setTimeout(() => statusEl.textContent = '', 2000);
                    }
                }
            }
        }
    })
    .catch((error) => {
        console.error('Error syncing config:', error);
    });
}

// 根据配置更新UI
function updateUIFromConfig() {
    for (let group = 0; group < 12; group++) {
        updateModeUI(group);
        if (gpioConfig.outputStates && gpioConfig.outputStates[group]) {
            for (let pin = 0; pin < 2; pin++) {
                updatePinUI(group, pin);
            }
        }
    }
}

// 更新模式指示器和按钮状态
function updateModeUI(group) {
    const mode = gpioConfig.groupModes[group];
    const modeIndicator = document.getElementById(`mode_group_${group}`);
    const outputBtn = document.getElementById(`btn_mode_${group}_0`);
    const inputBtn = document.getElementById(`btn_mode_${group}_1`);
    const switchContainer = document.getElementById(`switch_group_${group}_0`)?.closest('.switch-container');

    if (!modeIndicator || !outputBtn || !inputBtn) return;

    if (mode === 0) { // 输出模式
        modeIndicator.textContent = '输出模式';
        modeIndicator.classList.remove('input-mode');
        modeIndicator.classList.add('output-mode');
        outputBtn.classList.add('active');
        inputBtn.classList.remove('active');
        if (switchContainer) switchContainer.style.display = 'block';
    } else { // 输入模式
        modeIndicator.textContent = '输入模式';
        modeIndicator.classList.remove('output-mode');
        modeIndicator.classList.add('input-mode');
        outputBtn.classList.remove('active');
        inputBtn.classList.add('active');
        if (switchContainer) switchContainer.style.display = 'none';
    }
}

// 更新单个引脚的UI
function updatePinUI(group, pin) {
    const state = gpioConfig.outputStates?.[group]?.[pin];
    const stateDisplay = document.getElementById(`state_group_${group}_${pin}`);
    const switchEl = document.getElementById(`switch_group_${group}_${pin}`);

    if (stateDisplay) {
        stateDisplay.textContent = state !== undefined ? `电平: ${state}` : '未设置';
    }
    if (switchEl) {
        switchEl.checked = (state === 1);
    }
}