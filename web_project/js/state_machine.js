// 全局变量存储状态机配置
let stateMachineConfig = {
    stateOutputs: [],    // 状态关联输出配置（空数组，由用户添加）
    initialState: "",    // 初始状态
    currentState: "",    // 当前状态
    isRunning: false     // 状态机运行状态
};

// 存储模拟GPIO状态的全局变量
let simulatedGPIOStates = {
    0: false,
    1: false,
    2: false,
    3: false,
    4: false,
    5: false,
    6: false,
    7: false,
    8: false,
    9: false,
    10: false,
    11: false
};

// IO组配置
let ioGroups = [
    { id: 0, name: "GPIO组0" },
    { id: 1, name: "GPIO组1" },
    { id: 2, name: "GPIO组2" },
    { id: 3, name: "GPIO组3" },
    { id: 4, name: "E1 (AW9523)" },
    { id: 5, name: "E2 (AW9523)" },
    { id: 6, name: "E3 (AW9523)" },
    { id: 7, name: "E4 (AW9523)" },
    { id: 8, name: "E5 (AW9523)" },
    { id: 9, name: "E6 (AW9523)" },
    { id: 10, name: "E7 (AW9523)" },
    { id: 11, name: "E8 (AW9523)" }
];

// 输入组配置
let inputGroups = [];
let timerCounter = 1;

// 初始化状态机控制功能
function initializeStateMachine() {
    // 恢复保存的IP地址
    var savedIP = localStorage.getItem('esp32-ip');
    if (savedIP) {
        document.getElementById('esp32-ip').value = savedIP;
    }
    
    // 为初始状态下拉框生成选项
    generateInitialStateOptions();
    
    // 从本地存储恢复配置
    loadStateMachineConfig();
    
    // 初始化IO组配置（默认4组IO）
    loadIOGroupsConfig();
    
    // 从本地存储恢复输入组配置
    loadInputGroupsConfig();
    
    // 只有在没有恢复到输入组配置时，才添加默认的输入组配置
    if (inputGroups.length === 0) {
        // 不添加任何默认配置，让用户自己添加
        // 用户可以通过界面手动添加需要的GPIO或计时器输入
    }
    
    // 重新渲染输入组表格
    var table = document.getElementById('input-groups-table').getElementsByTagName('tbody')[0];
    table.innerHTML = '';
    
    inputGroups.forEach(function(group) {
        var row = table.insertRow();
        if (group.type === 'gpio') {
            // 使用uniqueId作为行ID（如果存在），否则使用旧格式兼容
            var rowId = group.uniqueId ? 'input-row-' + group.uniqueId : 'input-row-gpio-' + group.id;
            var updateId = group.uniqueId ? group.uniqueId : 'gpio-' + group.id;
            
            // 根据组ID确定显示名称
            var groupDisplayName;
            if (group.id >= 4) {
                groupDisplayName = "E" + (group.id - 3) + " (AW9523)";
            } else {
                groupDisplayName = "GPIO 组 " + group.id;
            }
            
            // 模块类型显示名称
            var moduleTypeName = (group.moduleType === 'led_button') ? '带灯按键' : '普通按键';
            
            row.id = rowId;
            row.innerHTML = `
                <td>GPIO</td>
                <td>${groupDisplayName}</td>
                <td>${moduleTypeName}</td>
                <td>
                    <select onchange="updateInputGroupCurrentState('${updateId}', this.value)">
                        ${generateStateOptions(group.currentState)}
                    </select>
                </td>
                <td>
                    <select onchange="updateInputGroupNextState('${updateId}', this.value)">
                        ${generateStateOptions(group.nextState)}
                    </select>
                </td>
                <td>
                    <button class="delete-btn" onclick="removeInputGroup('${updateId}')">删除</button>
                </td>
            `;
        } else if (group.type === 'timer') {
            row.id = 'input-row-' + group.id;
            row.innerHTML = `
                <td>计时器</td>
                <td>${group.name}</td>
                <td>
                    <select onchange="updateInputGroupCurrentState('${group.id}', this.value)">
                        ${generateStateOptions(group.currentState)}
                    </select>
                </td>
                <td>
                    <select onchange="updateInputGroupNextState('${group.id}', this.value)">
                        ${generateStateOptions(group.nextState)}
                    </select>
                </td>
                <td>
                    <button class="delete-btn" onclick="removeInputGroup('${group.id}')">删除</button>
                </td>
            `;
        }
    });
    
    // 添加输入类型选择的事件监听器
    const inputTypeSelect = document.getElementById('input-type-select');
    if (inputTypeSelect) {
        inputTypeSelect.addEventListener('change', function() {
            const selectedType = this.value;
            const gpioSection = document.getElementById('gpio-input-section');
            const timerSection = document.getElementById('timer-input-section');
            
            if (gpioSection && timerSection) {
                if (selectedType === 'gpio') {
                    gpioSection.style.display = 'block';
                    timerSection.style.display = 'none';
                } else if (selectedType === 'timer') {
                    gpioSection.style.display = 'none';
                    timerSection.style.display = 'block';
                }
            }
        });
    }
    
    // 添加GPIO输出状态显示区域
    addGPIOOutputDisplay();
    
    console.log("状态机控制初始化完成");
}

// 添加GPIO输出状态显示区域
function addGPIOOutputDisplay() {
    var stateMachineControl = document.querySelector('.state-machine-control');
    if (!stateMachineControl) return;
    
    // 检查是否已添加该区域
    if (document.getElementById('gpio-output-display')) return;
    
    var displayContainer = document.createElement('div');
    displayContainer.id = 'gpio-output-display';
    displayContainer.className = 'gpio-output-display';
    
    var title = document.createElement('h3');
    title.innerHTML = 'GPIO输出状态';
    displayContainer.appendChild(title);
    
    var outputGrid = document.createElement('div');
    outputGrid.className = 'gpio-output-grid';
    
    // 添加4个GPIO组的输出状态显示
    for (var i = 0; i < 4; i++) {
        var outputItem = document.createElement('div');
        outputItem.className = 'gpio-output-item';
        outputItem.innerHTML = `
            <label>GPIO组 ${i}: </label>
            <span id="gpio-output-${i}" class="gpio-output">未设置</span>
        `;
        outputGrid.appendChild(outputItem);
    }
    
    displayContainer.appendChild(outputGrid);
    
    // 在"当前状态"显示和控制按钮之间插入
    var controlButtons = stateMachineControl.querySelector('button.start-btn').parentNode;
    var currentStateDisplay = document.querySelector('.control-group:last-child');
    
    if (currentStateDisplay && controlButtons) {
        stateMachineControl.insertBefore(displayContainer, controlButtons);
    }
}

// 生成初始状态下拉选项
function generateInitialStateOptions() {
    const initialStateSelect = document.getElementById('initial-state');
    if (!initialStateSelect) return;
    
    // 清空现有选项（保留第一个"请选择"选项）
    while (initialStateSelect.options.length > 1) {
        initialStateSelect.remove(1);
    }
    
    // 使用现有的generateStateOptions函数生成选项
    const optionsHTML = generateStateOptions();
    
    // 创建一个临时div来解析HTML字符串
    const tempDiv = document.createElement('div');
    tempDiv.innerHTML = optionsHTML;
    
    // 将生成的选项添加到下拉框
    const options = tempDiv.querySelectorAll('option');
    options.forEach(option => {
        initialStateSelect.appendChild(option.cloneNode(true));
    })
}

// 生成状态值选项
function generateStateOptions(selectedValue) {
    var options = '<option value="">未连接</option>';
    
    // 生成s1到s128选项
    for (var i = 1; i <= 128; i++) {
        var state = 's' + i;
        var selected = (state === selectedValue) ? 'selected' : '';
        options += `<option value="${state}" ${selected}>${state}</option>`;
    }
    
    return options;
}

// 添加GPIO输入组
function addInputGroup() {
    var groupId = document.getElementById('input-group-select').value;
    var moduleType = document.getElementById('input-module-type').value; // 直接获取 "button" 或 "led_button"
    var table = document.getElementById('input-groups-table').getElementsByTagName('tbody')[0];
    
    // 生成唯一ID，用于区分相同GPIO组的不同规则
    var uniqueId = 'gpio-' + groupId + '-' + Date.now();
    
    // 添加到数组
    var group = {
        id: parseInt(groupId),
        uniqueId: uniqueId, // 添加唯一标识符
        type: 'gpio',
        moduleType: moduleType, // "button"=普通按键, "led_button"=带灯按键
        currentState: '', // 当前状态
        nextState: '', // 下一状态
        mode: 1 // 1表示输入模式
    };
    inputGroups.push(group);
    
    console.log('添加新的GPIO输入组:', groupId, '模块类型:', moduleType, '唯一ID:', uniqueId, '当前输入组配置:', JSON.stringify(inputGroups));
    
    // 根据组ID确定显示名称
    var groupDisplayName;
    if (parseInt(groupId) >= 4) {
        groupDisplayName = "E" + (parseInt(groupId) - 3) + " (AW9523)";
    } else {
        groupDisplayName = "GPIO 组 " + groupId;
    }
    
    // 模块类型显示名称
    var moduleTypeName = (moduleType === 'led_button') ? '带灯按键' : '普通按键';
    
    // 添加到表格
    var row = table.insertRow();
    row.id = 'input-row-' + uniqueId;
    row.innerHTML = `
        <td>GPIO</td>
        <td>${groupDisplayName}</td>
        <td>${moduleTypeName}</td>
        <td>
            <select onchange="updateInputGroupCurrentState('${uniqueId}', this.value)">
                ${generateStateOptions('')}
            </select>
        </td>
        <td>
            <select onchange="updateInputGroupNextState('${uniqueId}', this.value)">
                ${generateStateOptions('')}
            </select>
        </td>
        <td>
            <button class="delete-btn" onclick="removeInputGroup('${uniqueId}')">删除</button>
        </td>
    `;
    
    // 为双向状态转移提供指导提示
    console.log('提示：要创建双向状态转移规则，请确保为每个方向分别添加规则，例如：');
    console.log('1. 先添加从s1到s2的规则（当前状态=s1，下一状态=s2）');
    console.log('2. 再添加从s2到s1的规则（当前状态=s2，下一状态=s1）');
    console.log('这样可以在状态s1激活GPIO时转到s2，在状态s2激活GPIO时转回s1');
    
    // 保存配置
    saveInputGroupsConfig();
}

// 添加计时器输入
function addTimerInput() {
    var timerName = document.getElementById('timer-name').value;
    var table = document.getElementById('input-groups-table').getElementsByTagName('tbody')[0];
    
    // 验证输入
    if (!timerName) {
        alert('请选择计时器名称');
        return;
    }
    
    // 生成简单的数字ID，从1开始计数（与后端保持一致）
    var timerId = timerCounter;
    timerCounter++;
    
    // 获取显示名称
    var displayName = document.getElementById('timer-name').options[document.getElementById('timer-name').selectedIndex].text;
    
    // 不再限制重复添加计时器，允许创建重复的配置
    
    // 添加到数组
    var timer = {
        id: timerId, // 使用纯数字ID
        type: 'timer',
        name: displayName,
        duration: 1, // 默认1秒，将在表2中配置实际时间
        currentState: '', // 当前状态
        nextState: '' // 下一状态
    };
    inputGroups.push(timer);
    
    // 添加到表格
    var row = table.insertRow();
    row.id = 'input-row-' + timerId;
    row.innerHTML = `
        <td>计时器</td>
        <td>${displayName}</td>
        <td>
            <select onchange="updateInputGroupCurrentState('${timerId}', this.value)">
                ${generateStateOptions('')}
            </select>
        </td>
        <td>
            <select onchange="updateInputGroupNextState('${timerId}', this.value)">
                ${generateStateOptions('')}
            </select>
        </td>
        <td>
            <button class="delete-btn" onclick="removeInputGroup('${timerId}')">删除</button>
        </td>
    `;
    
    // 保存配置
    saveInputGroupsConfig();
}

// 更新输入组当前状态
function updateInputGroupCurrentState(uniqueId, stateValue) {
    var updated = false;
    for (var i = 0; i < inputGroups.length; i++) {
        // 使用唯一ID来识别特定的输入组规则
        // 修复：确保ID比较时类型一致
        let idMatch = false;
        if (inputGroups[i].uniqueId === uniqueId) {
            idMatch = true;
        } else if (inputGroups[i].id !== undefined) {
            // 兼容数字ID和字符串ID的比较
            idMatch = (inputGroups[i].id === uniqueId || inputGroups[i].id == uniqueId);
        }
        
        if (idMatch) {
            inputGroups[i].currentState = stateValue;
            updated = true;
            break; // 只更新找到的第一个匹配项
        }
    }
    if (updated) {
        saveInputGroupsConfig();
        console.log('更新输入组当前状态:', uniqueId, '->', stateValue);
    }
}

// 更新输入组下一状态
function updateInputGroupNextState(uniqueId, stateValue) {
    var updated = false;
    for (var i = 0; i < inputGroups.length; i++) {
        // 使用唯一ID来识别特定的输入组规则
        // 修复：确保ID比较时类型一致
        let idMatch = false;
        if (inputGroups[i].uniqueId === uniqueId) {
            idMatch = true;
        } else if (inputGroups[i].id !== undefined) {
            // 兼容数字ID和字符串ID的比较
            idMatch = (inputGroups[i].id === uniqueId || inputGroups[i].id == uniqueId);
        }
        
        if (idMatch) {
            inputGroups[i].nextState = stateValue;
            updated = true;
            break; // 只更新找到的第一个匹配项
        }
    }
    if (updated) {
        saveInputGroupsConfig();
        console.log('更新输入组下一状态:', uniqueId, '->', stateValue);
    }
}

// 删除输入组
function removeInputGroup(uniqueId) {
    // 从数组中移除
    inputGroups = inputGroups.filter(function(group) {
        // 使用唯一ID来识别特定的输入组规则
        if (group.uniqueId === uniqueId) {
            console.log('删除输入组规则:', uniqueId);
            return false; // 移除这个组
        } else if (group.type === 'gpio' && typeof group.id === 'number' && uniqueId.startsWith('gpio-')) {
            // 兼容旧的删除方式
            return group.id != parseInt(uniqueId.split('-')[1]);
        }
        // 兼容旧的非GPIO类型规则
        return group.id != uniqueId;
    });
    
    // 从表格中移除行
    var rowId = 'input-row-' + uniqueId;
    var row = document.getElementById(rowId);
    if (row) {
        row.remove();
    } else if (uniqueId.startsWith('gpio-')) {
        // 兼容旧的行ID格式
        rowId = 'input-row-gpio-' + uniqueId.split('-')[1];
        row = document.getElementById(rowId);
        if (row) {
            row.remove();
        }
    }
    
    // 保存配置
    saveInputGroupsConfig();
}

// 保存输入组配置到本地存储
function saveInputGroupsConfig() {
    try {
        localStorage.setItem('stateMachineInputGroups', JSON.stringify(inputGroups));
        console.log('输入组配置已保存:', JSON.stringify(inputGroups));
    } catch (e) {
        console.error('保存输入组配置失败:', e);
    }
}

// 从本地存储加载输入组配置
function loadInputGroupsConfig() {
    try {
        var savedConfig = localStorage.getItem('stateMachineInputGroups');
        if (savedConfig) {
            inputGroups = JSON.parse(savedConfig);
            
            console.log('从本地存储加载的输入组配置:', JSON.stringify(inputGroups));
            
            // 重新渲染表格
            var table = document.getElementById('input-groups-table').getElementsByTagName('tbody')[0];
            table.innerHTML = '';
            
            inputGroups.forEach(function(group) {
                var row = table.insertRow();
                
                if (group.type === 'gpio') {
                    row.id = 'input-row-gpio-' + group.id;
                    row.innerHTML = `
                        <td>GPIO</td>
                        <td>GPIO 组 ${group.id}</td>
                        <td>
                            <select onchange="updateInputGroupCurrentState('gpio-${group.id}', this.value)">
                                ${generateStateOptions(group.currentState)}
                            </select>
                        </td>
                        <td>
                            <select onchange="updateInputGroupNextState('gpio-${group.id}', this.value)">
                                ${generateStateOptions(group.nextState)}
                            </select>
                        </td>
                        <td>
                            <button class="delete-btn" onclick="removeInputGroup('gpio-${group.id}')">删除</button>
                        </td>
                    `;
                } else if (group.type === 'timer') {
                    row.id = 'input-row-' + group.id;
                    row.innerHTML = `
                        <td>计时器</td>
                        <td>${group.name}</td>
                        <td>
                            <select onchange="updateInputGroupCurrentState('${group.id}', this.value)">
                                ${generateStateOptions(group.currentState)}
                            </select>
                        </td>
                        <td>
                            <select onchange="updateInputGroupNextState('${group.id}', this.value)">
                                ${generateStateOptions(group.nextState)}
                            </select>
                        </td>
                        <td>
                            <button class="delete-btn" onclick="removeInputGroup('${group.id}')">删除</button>
                        </td>
                    `;
                }
            });
            
            console.log('输入组配置已加载并渲染到表格');
        }
    } catch (e) {
        console.error('加载输入组配置失败:', e);
        inputGroups = [];
    }
}

// 页面加载完成后初始化
window.addEventListener('DOMContentLoaded', function() {
    initializeStateMachine();
});

// 测试与ESP32的连接
function testConnection() {
    var ip = document.getElementById('esp32-ip').value;
    if (!ip) {
        alert('请先输入ESP32的IP地址');
        return;
    }
    
    // 保存IP地址到localStorage
    localStorage.setItem('esp32-ip', ip);
    
    var statusElement = document.getElementById("connection-status");
    statusElement.innerHTML = "正在测试连接...";
    statusElement.className = "status info";
    
    var xhr = new XMLHttpRequest();
    var url = "http://" + ip + "/gpio?test=connection";
    
    xhr.open("GET", url, true);
    xhr.timeout = 5000; // 5秒超时
    
    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
            if (xhr.status === 200) {
                try {
                    var response = JSON.parse(xhr.responseText);
                    // 修改：匹配后端返回的"ok"状态值
                    if (response.status === "success" || response.status === "ok") {
                        statusElement.innerHTML = "状态: 已连接";
                        statusElement.className = "status success";
                    }
                } catch (e) {
                    statusElement.innerHTML = "状态: 连接失败 - 解析响应出错";
                    statusElement.className = "status error";
                }
            } else {
                statusElement.innerHTML = "状态: 连接失败 - HTTP " + xhr.status;
                statusElement.className = "status error";
            }
        }
    };
    
    xhr.onerror = function() {
        statusElement.innerHTML = "状态: 网络错误 - 无法连接到ESP32";
        statusElement.className = "status error";
    };
    
    xhr.ontimeout = function() {
        statusElement.innerHTML = "状态: 连接超时 - 请检查IP地址是否正确";
        statusElement.className = "status error";
    };
    
    try {
        xhr.send();
    } catch (e) {
        statusElement.innerHTML = "状态: 错误 - " + e.message;
        statusElement.className = "status error";
    }
}

// 添加状态关联输出
function addStateOutput() {
    var outputId = stateMachineConfig.stateOutputs.length + 1;
    
    var output = {
        id: outputId,
        stateName: "",
        outputType: "",
        target: "",
        outputValue: ""
    };
    
    stateMachineConfig.stateOutputs.push(output);
    renderStateOutputTable();
    saveStateMachineConfig();
}

// 生成GPIO输出值的下拉选项
function generateGPIOOutputOptions(moduleType, selectedValue) {
    var options = '';
    
    if (moduleType === 'traffic_light') {
        // 交通灯: 0=灭, 1=红, 2=黄, 3=绿
        var trafficOptions = [
            {value: '0', text: '灭'},
            {value: '1', text: '红'},
            {value: '2', text: '黄'},
            {value: '3', text: '绿'}
        ];
        trafficOptions.forEach(function(option) {
            var selected = (selectedValue === option.value) ? 'selected' : '';
            options += `<option value="${option.value}" ${selected}>${option.text}</option>`;
        });
    } else {
        // LED灯和风扇: 0=关, 1=开
        var simpleOptions = [
            {value: '0', text: '关'},
            {value: '1', text: '开'}
        ];
        simpleOptions.forEach(function(option) {
            var selected = (selectedValue === option.value) ? 'selected' : '';
            options += `<option value="${option.value}" ${selected}>${option.text}</option>`;
        });
    }
    
    return options;
}

// 更新GPIO组的模块类型
function updateGroupModuleType(groupId, moduleType) {
    // 查找或创建inputGroups中的记录
    var existingGroup = inputGroups.find(g => g.id === groupId);
    if (existingGroup) {
        existingGroup.moduleType = moduleType;
    } else {
        // 创建新的组记录（仅用于存储模块类型）
        inputGroups.push({
            id: groupId,
            type: 'gpio',
            moduleType: moduleType,
            mode: 0  // 输出模式
        });
    }
    saveStateMachineConfig();
}

// 生成计时器输出值的下拉选项
function generateTimerOutputOptions(selectedValue) {
    var options = '';
    for (var i = 1; i <= 10; i++) {
        var selected = (selectedValue === i.toString()) ? 'selected' : '';
        options += `<option value="${i}" ${selected}>${i}秒</option>`;
    }
    return options;
}

// 渲染状态关联输出表格
function renderStateOutputTable() {
    var tableBody = document.querySelector("#state-output-table tbody");
    tableBody.innerHTML = "";
    
    stateMachineConfig.stateOutputs.forEach(function(output) {
        var row = document.createElement("tr");
        
        // 从groupModules获取模块类型
        var groupModuleType = '';
        if (output.outputType === 'gpio' && output.target !== '') {
            // 查找该GPIO组的模块类型
            var existingGroup = inputGroups.find(g => g.id === parseInt(output.target));
            if (existingGroup && existingGroup.moduleType) {
                groupModuleType = existingGroup.moduleType;
            }
        }
        
        // 模块类型选择（仅GPIO类型显示，仅输出模块类型）
        var moduleTypeControl = '';
        if (output.outputType === 'gpio' && output.target !== '') {
            var targetId = parseInt(output.target);
            moduleTypeControl = `
                <select onchange="updateGroupModuleType(${targetId}, this.value); renderStateOutputTable();">
                    <option value="">请选择</option>
                    <option value="led" ${groupModuleType === 'led' ? 'selected' : ''}>LED灯</option>
                    <option value="traffic_light" ${groupModuleType === 'traffic_light' ? 'selected' : ''}>交通灯</option>
                    <option value="fan" ${groupModuleType === 'fan' ? 'selected' : ''}>风扇</option>
                </select>
            `;
        } else if (output.outputType === 'gpio') {
            moduleTypeControl = '<span style="color: #999;">请先选择目标</span>';
        } else {
            moduleTypeControl = '-';
        }
        
        // 确定输出值控件类型
        var outputValueControl = '';
        if (output.outputType === 'gpio') {
            // GPIO组使用下拉菜单，根据模块类型生成不同选项
            outputValueControl = `
                <select onchange="updateStateOutput(${output.id}, 'outputValue', this.value)" value="${output.outputValue || ''}">
                    <option value="">请选择</option>
                    ${generateGPIOOutputOptions(groupModuleType, output.outputValue)}
                </select>
            `;
        } else if (output.outputType === 'timer') {
            // 计时器类型使用下拉菜单选择1-10秒
            outputValueControl = `
                <select onchange="updateStateOutput(${output.id}, 'outputValue', this.value)" value="${output.outputValue || ''}">
                    <option value="">请选择</option>
                    ${generateTimerOutputOptions(output.outputValue)}
                </select>
            `;
        } else if (output.outputType === 'motor') {
            // 电机类型使用下拉菜单选择转速和方向
            // 正值1-9: 正转10%-90%, 负值-9到-1: 反转10%-90%, 0: 停止
            var motorOptions = '<option value="">请选择</option>';
            motorOptions += '<option value="0"' + (output.outputValue === '0' ? ' selected' : '') + '>停止</option>';
            
            // 正转选项 (1-9)
            for (var i = 1; i <= 9; i++) {
                var speed = i * 10;
                motorOptions += '<option value="' + i + '"' + (output.outputValue === String(i) ? ' selected' : '') + '>正转 ' + speed + '%</option>';
            }
            
            // 反转选项 (-1到-9)
            for (var i = 1; i <= 9; i++) {
                var speed = i * 10;
                var value = -i;
                motorOptions += '<option value="' + value + '"' + (output.outputValue === String(value) ? ' selected' : '') + '>反转 ' + speed + '%</option>';
            }
            
            outputValueControl = `
                <select onchange="updateStateOutput(${output.id}, 'outputValue', this.value)" value="${output.outputValue || ''}">
                    ${motorOptions}
                </select>
            `;
        } else {
            // 其他类型继续使用文本输入框
            outputValueControl = `<input type="text" value="${output.outputValue || ''}" onchange="updateStateOutput(${output.id}, 'outputValue', this.value)">
            `;
        }
        
        row.innerHTML = `
            <td>${output.id}</td>
            <td>
                <select onchange="updateStateOutput(${output.id}, 'stateName', this.value)" value="${output.stateName || ''}">
                    <option value="">请选择</option>
                    ${generateStateOptions(output.stateName)}
                </select>
            </td>
            <td>
                <select onchange="updateStateOutput(${output.id}, 'outputType', this.value)" value="${output.outputType || ''}">
                    <option value="">请选择</option>
                    <option value="gpio">GPIO组</option>
                    <option value="timer">计时器</option>
                    <option value="motor">电机</option>
                </select>
            </td>
            <td>${moduleTypeControl}</td>
            <td>
                <select id="target-select-${output.id}" onchange="updateStateOutput(${output.id}, 'target', this.value)" value="${output.target || ''}">
                    <option value="">请选择</option>
                    ${generateTargetOptions(output.outputType, output.target)}
                </select>
            </td>
            <td>${outputValueControl}</td>
            <td><button class="delete-btn" onclick="deleteStateOutput(${output.id})"></button></td>
        `;
        
        // 设置select的选中值
        var stateNameSelect = row.querySelector('select[onchange*="stateName"]');
        if (output.stateName) {
            stateNameSelect.value = output.stateName;
        }
        
        var outputTypeSelect = row.querySelector('select[onchange*="outputType"]');
        if (output.outputType) {
            outputTypeSelect.value = output.outputType;
        }
        
        var outputValueSelect = row.querySelector('td:nth-child(5) select');
        if (outputValueSelect && output.outputValue) {
            outputValueSelect.value = output.outputValue;
        }
        
        // 添加输出类型变化事件监听
        outputTypeSelect.addEventListener('change', function() {
            var outputId = this.onchange.toString().match(/\d+/)[0];
            updateTargetOptions(outputId, this.value);
            // 重新渲染表格以更新输出值控件类型
            renderStateOutputTable();
        });
        
        tableBody.appendChild(row);
    });
}

// 生成目标选项
function generateTargetOptions(outputType, selectedValue) {
    var options = '';
    
    if (outputType === 'gpio') {
        // GPIO组选项 (0-11共12个组)
        for (var i = 0; i < 12; i++) {
            var selected = (selectedValue === i.toString()) ? 'selected' : '';
            // 根据组ID决定显示名称
            var groupName;
            if (i < 4) {
                groupName = `GPIO 组 ${i}`;
            } else {
                groupName = `E${i-3} (AW9523)`;
            }
            options += `<option value="${i}" ${selected}>${groupName}</option>`;
        }
    } else if (outputType === 'timer') {
        // 计时器选项 - 使用纯数字ID
        for (var i = 1; i <= 5; i++) {
            var selected = (selectedValue === i.toString()) ? 'selected' : '';
            options += `<option value="${i}" ${selected}>计时器${i}</option>`;
        }
    } else if (outputType === 'motor') {
        // 电机选项 - M1和M2
        var motors = [
            { id: '0', name: 'M1 电机' },
            { id: '1', name: 'M2 电机' }
        ];
        motors.forEach(function(motor) {
            var selected = (selectedValue === motor.id) ? 'selected' : '';
            options += `<option value="${motor.id}" ${selected}>${motor.name}</option>`;
        });
    }
    
    return options;
}

// 更新目标选项
function updateTargetOptions(outputId, outputType) {
    var selectElement = document.getElementById(`target-select-${outputId}`);
    if (selectElement) {
        selectElement.innerHTML = `<option value="">请选择</option>${generateTargetOptions(outputType, '')}`;
    }
}

// 更新状态关联输出
function updateStateOutput(id, field, value) {
    var outputIndex = stateMachineConfig.stateOutputs.findIndex(output => output.id === id);
    if (outputIndex !== -1) {
        // 对计时器值进行特殊验证
        if (field === 'outputValue' && stateMachineConfig.stateOutputs[outputIndex].outputType === 'timer') {
            // 去除空格
            value = value.trim();
            // 如果值为空或不是有效数字，设置为默认值1
            if (!value || isNaN(parseInt(value, 10)) || parseInt(value, 10) < 1) {
                value = '1';
                console.log('警告：无效的计时器值，已设置为默认值1秒');
            }
        }
        
        // 添加日志记录target字段的修改
        if (field === 'target') {
            console.log(`更新输出ID ${id}的target: 从'${stateMachineConfig.stateOutputs[outputIndex].target}'到'${value}'`);
        }
        
        stateMachineConfig.stateOutputs[outputIndex][field] = value;
        saveStateMachineConfig();
    }
}

// 删除状态关联输出
function deleteStateOutput(id) {
    stateMachineConfig.stateOutputs = stateMachineConfig.stateOutputs.filter(output => output.id !== id);
    // 重新编号
    stateMachineConfig.stateOutputs.forEach((output, index) => {
        output.id = index + 1;
    });
    renderStateOutputTable();
    saveStateMachineConfig();
}

// 启动状态机
function startStateMachine() {
    try {
        var initialState = document.getElementById('initial-state').value;
        if (!initialState) {
            alert('请先选择初始状态');
            return;
        }
        
        console.log('--- 启动状态机 ---');
        console.log('初始状态:', initialState);
        console.log('当前输入组配置:', JSON.stringify(inputGroups));
        
        stateMachineConfig.initialState = initialState;
        stateMachineConfig.currentState = initialState;
        stateMachineConfig.isRunning = true;
        
        // 更新UI
        var currentStateDisplay = document.getElementById('current-state-display');
        if (currentStateDisplay) {
            currentStateDisplay.innerHTML = initialState;
            currentStateDisplay.className = "state-display running";
        } else {
            console.error('未找到current-state-display元素');
        }
        
        // 应用初始状态的输出
        applyStateOutputs(initialState);
        
        // 保存配置
        saveStateMachineConfig();
        
        // 启动监控
        startMonitoring();
        
        console.log('状态机启动成功');
    } catch (error) {
        console.error('启动状态机时出错:', error);
        alert('启动状态机时出错: ' + error.message);
    }
}

// 停止状态机
function stopStateMachine() {
    stateMachineConfig.isRunning = false;
    
    // 清除所有活跃的计时器
    for (var timerId in activeTimers) {
        if (activeTimers.hasOwnProperty(timerId)) {
            clearTimeout(activeTimers[timerId]);
        }
    }
    activeTimers = {};
    
    // 更新UI
    document.getElementById('current-state-display').innerHTML = "已停止";
    document.getElementById('current-state-display').className = "state-display stopped";
    
    // 保存配置
    saveStateMachineConfig();
    
    // 停止监控
    stopMonitoring();
}

// 重置状态机
function resetStateMachine() {
    stopStateMachine();
    
    // 重置当前状态
    stateMachineConfig.currentState = "";
    
    // 更新UI
    document.getElementById('current-state-display').innerHTML = "未启动";
    document.getElementById('current-state-display').className = "state-display";
    
    // 保存配置
    saveStateMachineConfig();
}

// 清空所有配置
function clearAllConfig() {
    if (!confirm('确定要清空所有配置吗？此操作不可撤销！')) {
        return;
    }
    
    // 停止状态机
    stopStateMachine();
    
    // 清空所有配置数据
    stateMachineConfig = {
        stateOutputs: [],
        initialState: "",
        currentState: "",
        isRunning: false
    };
    
    inputGroups = [];
    
    // 清空localStorage
    localStorage.removeItem('stateMachineConfig');
    localStorage.removeItem('stateMachineInputGroups');
    localStorage.removeItem('stateMachineIOGroups');
    
    // 清空UI表格
    var stateOutputTable = document.getElementById('state-output-table').getElementsByTagName('tbody')[0];
    stateOutputTable.innerHTML = '';
    
    var inputGroupsTable = document.getElementById('input-groups-table').getElementsByTagName('tbody')[0];
    inputGroupsTable.innerHTML = '';
    
    // 重置初始状态选择
    document.getElementById('initial-state').value = '';
    
    // 重置当前状态显示
    document.getElementById('current-state-display').innerHTML = "未启动";
    document.getElementById('current-state-display').className = "state-display";
    
    // 显示成功消息
    var syncStatus = document.getElementById('sync-status');
    syncStatus.textContent = '所有配置已清空！';
    syncStatus.className = 'status success';
    
    setTimeout(function() {
        syncStatus.textContent = '';
        syncStatus.className = 'status';
    }, 3000);
}

// 应用状态输出
function applyStateOutputs(stateName) {
    // 查找该状态的所有输出
    var outputs = stateMachineConfig.stateOutputs.filter(output => output.stateName === stateName);
    
    console.log(`应用状态'${stateName}'的输出，输出数量: ${outputs.length}`);
    outputs.forEach(function(output) {
        // 添加日志追踪target值
        console.log(`处理输出 - ID: ${output.id}, 类型: ${output.outputType}, target: ${output.target}, 值: ${output.outputValue}`);
        
        // 根据输出类型执行不同的操作
        if (output.outputType === "gpio") {
            // 处理GPIO输出
            handleGPIOOutput(output.target, output.outputValue);
        } else if (output.outputType === "timer") {
            // 处理计时器输出 - 添加额外的防护措施
            let timerValue = output.outputValue;
            
            // 验证计时器值
            if (timerValue === null || timerValue === undefined || timerValue === '') {
                console.log("警告：在应用计时器输出时检测到无效值，使用默认值1秒");
                timerValue = "1";
            } else if (typeof timerValue === 'string') {
                // 去除可能的空格
                timerValue = timerValue.trim();
                // 如果是空字符串，设置默认值
                if (!timerValue) {
                    console.log("警告：在应用计时器输出时检测到空字符串，使用默认值1秒");
                    timerValue = "1";
                } else {
                    // 尝试解析为整数，明确指定基数10
                    const parsedValue = parseInt(timerValue, 10);
                    
                    // 检查解析结果
                    if (isNaN(parsedValue) || parsedValue < 1) {
                        console.log("警告：在应用计时器输出时检测到无效值'" + timerValue + "'，使用默认值1秒");
                        timerValue = "1";
                    }
                }
            }
            
            // 调用handleTimerOutput函数
            handleTimerOutput(output.target, timerValue);
        }
    });
}

// 处理GPIO输出
function handleGPIOOutput(target, value) {
    // 这里可以根据实际需求实现GPIO输出的处理逻辑
    console.log("应用GPIO输出: 组=", target, "值=", value);
    
    // 在网页上模拟显示GPIO输出状态变化
    var gpioOutputElement = document.getElementById('gpio-output-' + target);
    if (gpioOutputElement) {
        // 根据值设置显示文本和样式
        var displayText = "未知";
        var className = "gpio-output";
        
        switch(value) {
            case "10":
                displayText = "开/黄";
                className = "gpio-output active";
                break;
            case "00":
                displayText = "关";
                className = "gpio-output";
                break;
            case "01":
                displayText = "红";
                className = "gpio-output active red";
                break;
            case "11":
                displayText = "绿";
                className = "gpio-output active green";
                break;
            default:
                displayText = value === "1" ? "开启" : "关闭";
                className = value === "1" ? "gpio-output active" : "gpio-output";
                break;
        }
        
        gpioOutputElement.innerHTML = displayText;
        gpioOutputElement.className = className;
    }
    
    // 如果已连接到ESP32，可以发送命令到设备
    var ip = document.getElementById('esp32-ip').value;
    if (ip) {
        // 这里可以根据GPIO组和值构建发送到ESP32的命令
        // 注意：10值同时对应"开"和"黄"，需要根据实际需求确定具体行为
    }
}

// 存储活跃的计时器
let activeTimers = {};

// 处理计时器输出
function handleTimerOutput(target, value) {
    // 这里可以根据实际需求实现计时器输出的处理逻辑
    console.log("应用计时器输出: 计时器=", target, "值=", value, "(类型:", typeof value, ")");
    
    // 解析计时器值（单位：秒）
    // 改进：增强健壮性，确保正确处理各种边缘情况
    var duration;
    
    // 首先确保value是字符串或数字
    if (value === null || value === undefined) {
        duration = 1;
        console.log("警告：计时器值为null或undefined，使用默认值1秒");
    } else {
        // 尝试将值转换为数字
        if (typeof value === 'string') {
            // 去除可能的空格
            value = value.trim();
            // 如果是空字符串，设置默认值
            if (!value) {
                duration = 1;
                console.log("警告：计时器值为空字符串，使用默认值1秒");
            } else {
                // 尝试解析为整数，明确指定基数10
                duration = parseInt(value, 10);
                
                // 检查解析结果
                if (isNaN(duration) || duration < 1) {
                    duration = 1;
                    console.log("警告：计时器值'" + value + "'解析失败或无效(<1)，使用默认值1秒");
                }
            }
        } else {
            duration = parseInt(value, 10);
            
            // 检查解析结果
            if (isNaN(duration) || duration < 1) {
                duration = 1;
                console.log("警告：计时器值'" + value + "'解析失败或无效(<1)，使用默认值1秒");
            }
        }
    }
    var milliseconds = duration * 1000;
    
    console.log("启动计时器: ", target, "持续时间=", duration, "秒 (" + milliseconds + "毫秒) - 计划触发时间: ", new Date(Date.now() + milliseconds).toISOString());
    
    // 清除之前的同名计时器（如果存在）
    if (window.activeTimers && window.activeTimers[target]) {
        console.log("清除window.activeTimers中的计时器: ", target);
        clearTimeout(window.activeTimers[target]);
        delete window.activeTimers[target];
    }
    if (activeTimers[target]) {
        console.log("清除activeTimers中的计时器: ", target);
        clearTimeout(activeTimers[target]);
    }
    
    // 统一使用activeTimers变量
    activeTimers[target] = setTimeout(function() {
        // 计时器到期后执行的操作
        console.log("计时器到期: ", target, " - 实际触发时间: ", new Date().toISOString());
        
        // 检查是否有与该计时器关联的状态转移
        let transitionTriggered = false;
        
        for (var i = 0; i < inputGroups.length; i++) {
            var inputGroup = inputGroups[i];
            // 改进ID匹配逻辑，确保能正确匹配各种格式的计时器ID
            let idMatch = false;
            if (inputGroup.type === 'timer') {
                if (typeof inputGroup.id === 'string' && inputGroup.id === target) {
                    idMatch = true;
                } else if (typeof inputGroup.id === 'number') {
                    // 支持数字ID加前缀匹配，忽略大小写
                    idMatch = ('timer' + inputGroup.id) === target.toLowerCase() || 
                             ('Timer' + inputGroup.id) === target;
                }
            }
            
            if (idMatch && inputGroup.currentState && inputGroup.nextState) {
                console.log("找到匹配的计时器输入组: ID=", inputGroup.id, ", 当前状态=", inputGroup.currentState, ", 下一状态=", inputGroup.nextState);
                console.log("检查状态匹配: 当前状态机状态=", stateMachineConfig.currentState, "输入组当前状态=", inputGroup.currentState);
                
                // 关键逻辑：只有当状态机当前状态与输入组中定义的当前状态完全匹配时，才触发状态转移
                if (stateMachineConfig.currentState === inputGroup.currentState) {
                    console.log("状态匹配，触发状态转移: 从" + inputGroup.currentState + "到" + inputGroup.nextState);
                    // 触发状态转移
                    triggerStateTransition(target, inputGroup.currentState, inputGroup.nextState);
                    transitionTriggered = true;
                    break;
                } else {
                    console.log("状态不匹配，不触发状态转移");
                }
            }
        }
        
        // 如果没有找到匹配的转移规则，尝试直接使用目标作为下一状态
        if (!transitionTriggered && stateMachineConfig.currentState && target) {
            console.log("未找到关联的状态转移规则，尝试直接使用目标作为下一状态");
            // 检查目标是否是有效的状态名（s1-s128格式）
            if (target.match(/^s\d+$/i)) {
                triggerStateTransition(target, stateMachineConfig.currentState, target);
                transitionTriggered = true;
            }
        }
        
        // 如果仍然没有触发状态转移，记录详细日志以便调试
        if (!transitionTriggered) {
            console.log("计时器到期但未触发状态转移的详细信息:");
            console.log("- 状态机当前状态: ", stateMachineConfig.currentState);
            console.log("- 计时器目标: ", target);
            console.log("- 所有计时器输入组配置: ");
            inputGroups.forEach(function(group) {
                if (group.type === 'timer') {
                    console.log("  - 计时器ID: ", group.id, "当前状态: ", group.currentState, "下一状态: ", group.nextState);
                }
            });
        }
        
        // 移除已完成的计时器
        delete activeTimers[target];
        // 确保window.activeTimers中也删除
        if (window.activeTimers) {
            delete window.activeTimers[target];
        }
        console.log("已移除完成的计时器: ", target);
    }, milliseconds);
}

// 启动监控
function startMonitoring() {
    // 现在使用触发式查询，不需要定时器
    console.log("状态机监控已启动（触发式查询模式）");
}

// 停止监控
function stopMonitoring() {
    // 触发式查询模式不需要清理定时器
    console.log("状态机监控已停止");
}

// 用户需求实现说明：
// 1. 输入或计时器结束触发状态转移：
//    - GPIO输入：通过checkInputStates函数定期检查GPIO输入状态，当状态匹配时触发状态转移
//    - 计时器结束：通过handleTimerOutput函数中的setTimeout在计时器结束时触发状态转移
// 2. 状态激活触发输出或计时器启动：
//    - 在startStateMachine和triggerStateTransition函数中通过applyStateOutputs应用状态输出
//    - applyStateOutputs会调用handleGPIOOutput和handleTimerOutput处理不同类型的输出

// 模拟GPIO输入变化
function simulateGPIOChange(gpioGroup, isActive) {
    // 更新模拟GPIO状态
    simulatedGPIOStates[gpioGroup] = isActive;
    
    // 更新UI状态显示
    var statusElement = document.getElementById('gpio-sim-' + gpioGroup + '-status');
    if (statusElement) {
        statusElement.innerHTML = isActive ? '已触发' : '未触发';
        statusElement.className = isActive ? 'gpio-status active' : 'gpio-status';
    }
    
    console.log('模拟GPIO输入变化: 组=', gpioGroup, '状态=', isActive);
    
    // 如果状态机正在运行，检查是否触发状态转移
    if (stateMachineConfig.isRunning) {
        checkSimulatedGPIOInputs();
    }
}

// 检查输入状态并触发状态转移
function checkInputStates() {
    try {
        // 如果状态机未运行，不需要检查输入
        if (!stateMachineConfig.isRunning) {
            return;
        }
        
        console.debug('--- 检查输入状态 ---');
        console.debug('当前状态:', stateMachineConfig.currentState, '运行状态:', stateMachineConfig.isRunning);
        console.debug('模拟GPIO状态:', JSON.stringify(simulatedGPIOStates));
        
        // 记录当前所有输入组配置，帮助理解当前的状态转移规则
        console.debug('当前输入组配置数量:', inputGroups.length);
        
        // 计算并显示每个状态的输入规则数量
        var stateRules = {};
        inputGroups.forEach(function(group) {
            if (group.currentState && group.nextState) {
                var key = group.currentState + '->' + group.nextState;
                stateRules[key] = (stateRules[key] || 0) + 1;
            }
        });
        console.debug('状态转移规则分布:', stateRules);
        
        // 如果状态机未运行，不需要检查输入
        if (!stateMachineConfig.isRunning) {
            console.log('状态机未运行，跳过输入检查');
            return;
        }
        
        // 存储需要重置的GPIO组
        var gpioGroupsToReset = [];
        
        // 记录是否有状态转移被触发
        var stateTransitionTriggered = false;
        
        // 遍历所有输入组，找到第一个匹配的规则就执行
        for (var i = 0; i < inputGroups.length; i++) {
            var inputGroup = inputGroups[i];
            
            // 确保inputGroup是有效的对象
            if (!inputGroup || typeof inputGroup !== 'object') {
                console.error('发现无效的输入组配置:', inputGroup);
                continue;
            }
            
            // 对于GPIO输入，当当前状态等于输入组的当前状态且模拟GPIO状态为激活时触发转移
            if (inputGroup.type === 'gpio') {
                // 检查uniqueId属性是否存在，如果不存在则添加
                if (!inputGroup.uniqueId) {
                    inputGroup.uniqueId = 'gpio-' + inputGroup.id;
                    console.debug('为GPIO输入组', inputGroup.id, '添加缺失的uniqueId:', inputGroup.uniqueId);
                }
                
                // 使用debug级别日志记录详细信息
                console.debug('检查GPIO输入组', inputGroup.id, '(唯一ID:', inputGroup.uniqueId, '):', '当前状态要求=', inputGroup.currentState, '下一状态=', inputGroup.nextState);
                
                // 打印详细的检查条件
                console.debug('  条件检查: currentState=' + stateMachineConfig.currentState + 
                          ', inputGroup.currentState=' + inputGroup.currentState + 
                          ', 匹配状态=' + (stateMachineConfig.currentState === inputGroup.currentState) +
                          ', simulatedGPIOStates[' + inputGroup.id + ']=' + simulatedGPIOStates[inputGroup.id]);
                
                // 条件不满足时不输出错误日志，直接继续检查下一个条件
                
                if (inputGroup.currentState && inputGroup.nextState && 
                    stateMachineConfig.currentState === inputGroup.currentState && 
                    simulatedGPIOStates[inputGroup.id]) {
                    
                    console.log('条件匹配! 触发状态转移');
                    // 触发状态转移
                    triggerStateTransition(inputGroup.id, inputGroup.currentState, inputGroup.nextState);
                    
                    // 记录需要重置的GPIO组
                    if (gpioGroupsToReset.indexOf(inputGroup.id) === -1) {
                        gpioGroupsToReset.push(inputGroup.id);
                    }
                    
                    // 标记有状态转移被触发
                    stateTransitionTriggered = true;
                    
                    // 根据用户要求，一次触发只要满足一个转移条件就好，找到第一个匹配的条件后跳出循环
                    break;
                } else {
                    // 使用debug级别日志，减少控制台噪音
                    console.debug('条件不匹配 - 原因:', 
                        !inputGroup.currentState ? '无当前状态' : 
                        !inputGroup.nextState ? '无下一状态' : 
                        stateMachineConfig.currentState !== inputGroup.currentState ? '状态不匹配(' + stateMachineConfig.currentState + '!=' + inputGroup.currentState + ')' : 
                        !simulatedGPIOStates[inputGroup.id] ? 'GPIO状态未激活' : '未知原因');
                }
            }
        }
        
        // 在处理完所有匹配规则后，统一重置模拟状态和更新UI
        for (var j = 0; j < gpioGroupsToReset.length; j++) {
            var groupId = gpioGroupsToReset[j];
            
            // 重置模拟状态
            simulatedGPIOStates[groupId] = false;
            
            // 更新UI
            var statusElement = document.getElementById('gpio-sim-' + groupId + '-status');
            if (statusElement) {
                statusElement.innerHTML = '已触发并重置';
                statusElement.className = 'gpio-status triggered';
            }
            
            // 重置开关状态
            var switchElement = document.getElementById('gpio-sim-' + groupId);
            if (switchElement) {
                switchElement.checked = false;
            }
        }
        
        // 如果有状态转移被触发，重新检查输入状态，以处理可能的连续状态转移
        if (stateTransitionTriggered) {
            console.debug('有状态转移被触发，重新检查输入状态');
            // 使用setTimeout延迟执行，避免调用栈过深
            setTimeout(function() {
                checkInputStates();
            }, 100);
        }
        
        // 注意：计时器输入的状态转移由handleTimerOutput函数在计时器结束时触发
    }
    catch (error) {
        console.error('检查输入状态时出错:', error);
        console.error('错误堆栈:', error.stack);
        // 不中断程序，继续执行下一次检查
    }
}



// 检查模拟GPIO输入并触发状态转移 - 现在直接调用checkInputStates，避免重复代码
function checkSimulatedGPIOInputs() {
    checkInputStates();
}

// 触发状态转移
function triggerStateTransition(triggerSource, fromState, toState) {
    // 检查当前状态是否匹配
    if (stateMachineConfig.currentState === fromState && stateMachineConfig.isRunning) {
        console.log("触发源: " + triggerSource + "，从状态: " + fromState + " 转移到状态: " + toState);
        
        // 更新状态机的当前状态
        stateMachineConfig.currentState = toState;
        
        // 更新UI
        document.getElementById('current-state-display').innerHTML = toState;
        
        // 应用新状态的输出
        applyStateOutputs(toState);
        
        // 保存配置
        saveStateMachineConfig();
    }
}

// 显示IO组管理模态框
function showIOGroupManager() {
    // 渲染IO组列表
    renderIOGroupsList();
    
    // 显示模态框
    document.getElementById('io-group-modal').style.display = 'block';
}

// 关闭IO组管理模态框
function closeIOGroupManager() {
    document.getElementById('io-group-modal').style.display = 'none';
}

// 渲染IO组列表
function renderIOGroupsList() {
    var ioGroupsList = document.getElementById('io-groups-list');
    ioGroupsList.innerHTML = '';
    
    if (ioGroups.length === 0) {
        ioGroupsList.innerHTML = '<p>暂无IO组，请点击"添加IO组"按钮添加</p>';
        return;
    }
    
    var table = document.createElement('table');
    table.className = 'io-groups-table';
    
    // 创建表头
    var thead = document.createElement('thead');
    thead.innerHTML = `
        <tr>
            <th>ID</th>
            <th>名称</th>
            <th>操作</th>
        </tr>
    `;
    table.appendChild(thead);
    
    // 创建表体
    var tbody = document.createElement('tbody');
    
    ioGroups.forEach(function(group) {
        var row = document.createElement('tr');
        row.innerHTML = `
            <td>${group.id}</td>
            <td><input type="text" value="${group.name}" onchange="updateIOGroupName(${group.id}, this.value)"></td>
            <td><button class="delete-btn" onclick="deleteIOGroup(${group.id})"></button></td>
        `;
        tbody.appendChild(row);
    });
    
    table.appendChild(tbody);
    ioGroupsList.appendChild(table);
}

// 添加新的IO组
function addNewIOGroup() {
    // 生成新的IO组ID
    var maxId = -1;
    ioGroups.forEach(function(group) {
        if (group.id > maxId) {
            maxId = group.id;
        }
    });
    
    var newId = maxId + 1;
    var newName = 'IO组' + (newId + 1);
    
    // 添加新的IO组
    addIOGroup(newId, newName);
    
    // 重新渲染IO组列表
    renderIOGroupsList();
    
    // 更新状态转移表格
    renderTransitionTable();
}

// 更新IO组名称
function updateIOGroupName(id, newName) {
    var groupIndex = ioGroups.findIndex(group => group.id === id);
    if (groupIndex !== -1) {
        ioGroups[groupIndex].name = newName;
        saveIOGroupsConfig();
    }
}

// 删除IO组
function deleteIOGroup(id) {
    if (ioGroups.length <= 1) {
        alert('至少需要保留一个IO组');
        return;
    }
    
    // 检查是否有状态转移规则正在使用该IO组
    var isUsed = stateMachineConfig.transitions.some(function(rule) {
        return rule.triggerSource === id.toString();
    });
    
    if (isUsed) {
        if (!confirm('该IO组正在被状态转移规则使用，确定要删除吗？')) {
            return;
        }
        
        // 删除使用该IO组的状态转移规则
        stateMachineConfig.transitions = stateMachineConfig.transitions.filter(function(rule) {
            return rule.triggerSource !== id.toString();
        });
        
        // 重新保存状态机配置
        saveStateMachineConfig();
        
        // 更新状态转移表格
        renderTransitionTable();
    }
    
    // 删除IO组
    ioGroups = ioGroups.filter(function(group) {
        return group.id !== id;
    });
    
    // 保存IO组配置
    saveIOGroupsConfig();
    
    // 重新渲染IO组列表
    renderIOGroupsList();
}

// 同步配置到ESP32
function syncConfigToESP() {
    var ip = document.getElementById('esp32-ip').value;
    if (!ip) {
        alert('请先输入ESP32的IP地址');
        return;
    }
    
    var statusElement = document.getElementById('sync-status');
    statusElement.innerHTML = '正在同步配置...';
    statusElement.className = 'status info';
    
    var initialState = document.getElementById('initial-state').value;
    if (!initialState) {
        alert('请先选择初始状态');
        statusElement.innerHTML = '配置同步失败: 未选择初始状态';
        statusElement.className = 'status error';
        return;
    }
    
    // 在同步前清理计时器target格式，确保都是纯数字
    sanitizeTimerTargets();
    
    // 准备要发送的配置，使用驼峰命名法
    var configToSync = {
        initialState: initialState,
        autoStart: true,
        stateEffects: [],
        transitions: []
    };
    
    // 添加GPIO组模式信息到配置中
    const groupModes = {};
    const groupModules = {}; // 新增：模块类型映射
    
    // 从stateOutputs中收集使用的GPIO组（作为输出）
    if (stateMachineConfig.stateOutputs && Array.isArray(stateMachineConfig.stateOutputs)) {
        stateMachineConfig.stateOutputs.forEach(output => {
            if (output.outputType === 'gpio' && output.target !== undefined && output.target !== '') {
                const groupId = parseInt(output.target);
                if (!isNaN(groupId) && groupId >= 0 && groupId < 12) {
                    // GPIO输出组，模式为0（输出）
                    groupModes[groupId] = 0;
                    // 收集输出GPIO组的模块类型
                    const groupInfo = inputGroups.find(g => g.id === groupId);
                    if (groupInfo && groupInfo.moduleType) {
                        groupModules[groupId] = groupInfo.moduleType;
                    }
                }
            }
        });
    }
    
    // 从inputGroups中收集使用的GPIO组（作为输入）
    if (inputGroups && Array.isArray(inputGroups)) {
        inputGroups.forEach(group => {
            if (group && group.id !== undefined && group.type === 'gpio') {
                // 检查是否已经在输出配置中使用（避免覆盖输出模式）
                if (groupModes[group.id] !== 0) {
                    // GPIO输入组，模式为1（输入）
                    groupModes[group.id] = 1;
                }
                // 添加模块类型信息（字符串："button" 或 "led_button" 用于输入，"led"/"traffic_light"/"fan" 用于输出）
                if (group.moduleType) {
                    groupModules[group.id] = group.moduleType;
                }
            }
        });
    }
    
    // 只有当有模式信息时才添加到配置中
    if (Object.keys(groupModes).length > 0) {
        configToSync.groupModes = groupModes;
    }
    
    // 只有当有模块类型信息时才添加到配置中
    if (Object.keys(groupModules).length > 0) {
        configToSync.groupModules = groupModules;
    }
    
    // 转换stateOutputs为stateEffects
    if (stateMachineConfig.stateOutputs && Array.isArray(stateMachineConfig.stateOutputs)) {
        stateMachineConfig.stateOutputs.forEach(function(output, index) {
            // 修复：将effectType从字符串转换为与后端枚举匹配的数字
            // 1=EFFECT_TYPE_OUTPUT(GPIO), 2=EFFECT_TYPE_TIMER, 3=EFFECT_TYPE_MOTOR
            var effectTypeNum = 1; // 默认为GPIO
            if (output.outputType === 'gpio') {
                effectTypeNum = 1;
            } else if (output.outputType === 'timer') {
                effectTypeNum = 2;
            } else if (output.outputType === 'motor') {
                effectTypeNum = 3;
            }
            
            // 对于定时器类型，确保target是纯数字（与transitions中的triggerSource保持一致）
            var target = output.target || '';
            // 已经全部使用纯数字格式，无需额外处理
            
            configToSync.stateEffects.push({
                id: index + 1,
                stateName: output.stateName || '',
                effectType: effectTypeNum, // 使用数字而不是字符串
                target: target,
                value: output.outputValue !== undefined && output.outputValue !== null ? String(output.outputValue) : ''
            });
        });
    }
    
    // 添加日志记录发送前的配置，特别是stateEffects中的target字段格式，以确定问题来源。
    console.log('发送到ESP32的配置:', JSON.stringify(configToSync));
    
    // 单独记录stateEffects中的计时器类型条目
    var timerEffects = configToSync.stateEffects.filter(effect => effect.effectType === 2);
    if (timerEffects.length > 0) {
        console.log('计时器效果条目:');
        timerEffects.forEach(effect => {
            console.log(`- ID: ${effect.id}, stateName: ${effect.stateName}, target类型: ${typeof effect.target}, target值: ${effect.target}, 值: ${effect.value}`);
        });
    }
    
    // 如果有输入组配置，转换为transitions
    if (inputGroups && Array.isArray(inputGroups)) {
        inputGroups.forEach(function(group, index) {
            if (group.currentState && group.nextState) {
                var triggerType = group.type === 'timer' ? 2 : 1; // 1=input, 2=timer
                
                // 对于GPIO类型的输入组，确保triggerSource是纯数字
                var triggerSource;
                if (group.type === 'gpio') {
                    // 从group.id中提取数字部分，确保是纯数字
                    if (typeof group.id === 'number') {
                        triggerSource = group.id.toString();
                    } else if (typeof group.id === 'string' && group.id.match(/^\d+$/)) {
                        triggerSource = group.id;
                    } else if (group.uniqueId && group.uniqueId.startsWith('gpio-')) {
                        // 从uniqueId中提取数字部分
                        var match = group.uniqueId.match(/^gpio-(\d+)/);
                        triggerSource = match ? match[1] : '0';
                    } else {
                        // 默认使用0
                        triggerSource = '0';
                    }
                } else if (group.type === 'timer') {
                    // 对于计时器类型，确保triggerSource是纯数字（与后端保持一致）
                    if (typeof group.id === 'number') {
                        triggerSource = group.id.toString();
                    } else if (typeof group.id === 'string' && group.id.match(/^\d+$/)) {
                        triggerSource = group.id;
                    } else {
                        // 默认使用0
                        triggerSource = '0';
                    }
                } else {
                    triggerSource = group.id.toString();
                }
                
                configToSync.transitions.push({
                    id: index + 1,
                    triggerSource: triggerSource,
                    triggerType: triggerType,
                    currentState: group.currentState,
                    nextState: group.nextState
                });
            }
        });
    }
    
    var xhr = new XMLHttpRequest();
    var url = 'http://' + ip + '/state_machine';
    
    xhr.open('POST', url, true);
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.timeout = 10000; // 10秒超时
    
    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
            if (xhr.status === 200) {
                try {
                    var response = JSON.parse(xhr.responseText);
                    if (response.status === 'success') {
                        statusElement.innerHTML = '配置同步成功，状态机已自动启动';
                        statusElement.className = 'status success';
                
                // 更新本地状态机配置
                stateMachineConfig.initialState = initialState;
                stateMachineConfig.currentState = initialState;
                stateMachineConfig.isRunning = true;
                
                // 更新UI显示
                var currentStateDisplay = document.getElementById('current-state-display');
                if (currentStateDisplay) {
                    currentStateDisplay.innerHTML = initialState;
                    currentStateDisplay.className = "state-display running";
                }
                
                // 保存配置
                saveStateMachineConfig();
                    } else {
                        statusElement.innerHTML = '配置同步失败: ' + (response.message || '未知错误');
                        statusElement.className = 'status error';
                    }
                } catch (e) {
                    statusElement.innerHTML = '配置同步失败: 解析响应出错';
                    statusElement.className = 'status error';
                }
            } else {
                statusElement.innerHTML = '配置同步失败: HTTP ' + xhr.status;
                statusElement.className = 'status error';
            }
        }
    };
    
    xhr.onerror = function() {
        statusElement.innerHTML = '配置同步失败: 网络错误';
        statusElement.className = 'status error';
    };
    
    xhr.ontimeout = function() {
        statusElement.innerHTML = '配置同步失败: 超时';
        statusElement.className = 'status error';
    };
    
    try {
        xhr.send(JSON.stringify(configToSync));
    } catch (e) {
        statusElement.innerHTML = '配置同步失败: ' + e.message;
        statusElement.className = 'status error';
    }
}

// 保存状态机配置到本地存储
function saveStateMachineConfig() {
    localStorage.setItem('stateMachineConfig', JSON.stringify(stateMachineConfig));
}

// 从本地存储加载状态机配置
// 清理计时器target格式，确保都是纯数字
function sanitizeTimerTargets() {
    if (stateMachineConfig.stateOutputs && Array.isArray(stateMachineConfig.stateOutputs)) {
        let sanitizedCount = 0;
        
        stateMachineConfig.stateOutputs.forEach(function(output) {
            if (output.outputType === 'timer' && output.target) {
                // 尝试从target中提取纯数字部分
                let numericPart = output.target;
                
                // 如果target是字符串且包含'timer'前缀，提取后面的数字
                if (typeof numericPart === 'string' && numericPart.toLowerCase().includes('timer')) {
                    const match = numericPart.toLowerCase().match(/timer(\d+)/);
                    if (match && match[1]) {
                        console.log(`清理计时器target: 从'${output.target}'到'${match[1]}'`);
                        output.target = match[1];
                        sanitizedCount++;
                    }
                } else if (typeof numericPart === 'string' && !isNaN(parseInt(numericPart, 10))) {
                    // 确保是纯数字字符串（去掉可能的前导0）
                    const cleanedNumber = parseInt(numericPart, 10).toString();
                    if (cleanedNumber !== numericPart) {
                        console.log(`规范化计时器target: 从'${output.target}'到'${cleanedNumber}'`);
                        output.target = cleanedNumber;
                        sanitizedCount++;
                    }
                }
            }
        });
        
        if (sanitizedCount > 0) {
            console.log(`共清理了${sanitizedCount}个计时器target格式`);
            saveStateMachineConfig();
        }
    }
}

// 从本地存储加载状态机配置
function loadStateMachineConfig() {
    try {
        var savedConfig = localStorage.getItem('stateMachineConfig');
        if (savedConfig) {
            var parsedConfig = JSON.parse(savedConfig);
            
            // 初始化完整的stateMachineConfig对象
            stateMachineConfig = {
                stateOutputs: parsedConfig.stateOutputs || [],
                initialState: parsedConfig.initialState || "",
                currentState: parsedConfig.currentState || "",
                isRunning: parsedConfig.isRunning || false
            };
            
            // 验证并清理计时器输出值
            stateMachineConfig.stateOutputs.forEach(function(output) {
                if (output.outputType === 'timer') {
                    // 确保outputValue是有效的数字
                    let value = output.outputValue || '';
                    value = value.trim();
                    if (!value || isNaN(parseInt(value, 10)) || parseInt(value, 10) < 1) {
                        output.outputValue = '1';
                        console.log('警告：加载的计时器值无效，已设置为默认值1秒');
                    }
                }
            });
            
            // 清理计时器target格式，确保都是纯数字
            sanitizeTimerTargets();
            
            // 更新UI
            if (stateMachineConfig.initialState) {
                document.getElementById('initial-state').value = stateMachineConfig.initialState;
            }
            
            if (stateMachineConfig.currentState && stateMachineConfig.isRunning) {
                document.getElementById('current-state-display').innerHTML = stateMachineConfig.currentState;
                document.getElementById('current-state-display').className = "state-display running";
            } else if (stateMachineConfig.currentState && !stateMachineConfig.isRunning) {
                document.getElementById('current-state-display').innerHTML = "已停止";
                document.getElementById('current-state-display').className = "state-display stopped";
            }
            
            // 渲染表格
            renderStateOutputTable();
            
            console.log("状态机配置已从本地存储加载");
        } else {
            // 使用默认配置
            stateMachineConfig = {
                stateOutputs: [],
                initialState: "",
                currentState: "",
                isRunning: false
            };
        }
    } catch (e) {
        console.error("加载状态机配置失败: " + e.message);
        // 使用默认配置
        stateMachineConfig = {
            stateOutputs: [],
            initialState: "",
            currentState: "",
            isRunning: false
        };
    }
}

// 注意：triggerStateTransition函数的完整实现在文件上方

// 保存IO组配置到本地存储
function saveIOGroupsConfig() {
    localStorage.setItem('stateMachineIOGroups', JSON.stringify(ioGroups));
}

// 从本地存储加载IO组配置
function loadIOGroupsConfig() {
    var savedIOGroups = localStorage.getItem('stateMachineIOGroups');
    if (savedIOGroups) {
        try {
            ioGroups = JSON.parse(savedIOGroups);
        } catch (e) {
            console.log("解析IO组配置失败: " + e.message);
            // 不初始化默认组，让用户自己添加需要的组
            ioGroups = [];
        }
    } else {
        // 不初始化默认组，让用户自己添加需要的组
        ioGroups = [];
    }
}

// 初始化默认的IO组配置（已废弃，保留以兼容旧代码）
function initializeDefaultIOGroups() {
    // 不再自动添加所有IO组，用户需要通过输出配置来指定使用哪些组
    ioGroups = [];
    saveIOGroupsConfig();
}

// 添加IO组
function addIOGroup(id, name) {
    ioGroups.push({
        id: id,
        name: name
    });
    saveIOGroupsConfig();
}

// 渲染触发源选择控件
function renderTriggerSourceControl(rule) {
    // 根据triggerSource的值来决定渲染什么控件
    if (rule && rule.triggerSource && rule.triggerSource.includes('timer_')) {
        // 如果triggerSource包含'timer_'，显示计时器下拉选择框
        const select = document.createElement('select');
        select.className = 'timer-source-select';
        
        // 预设计时器选项
        const timerOptions = [
            {value: 'timer_1000', display: '计时器1 (1000ms)'},
            {value: 'timer_2000', display: '计时器2 (2000ms)'},
            {value: 'timer_3000', display: '计时器3 (3000ms)'},
            {value: 'timer_5000', display: '计时器4 (5000ms)'},
            {value: 'timer_10000', display: '计时器5 (10000ms)'}
        ];
        
        // 添加选项
        timerOptions.forEach(option => {
            const opt = document.createElement('option');
            opt.value = option.value;
            opt.textContent = option.display;
            if (option.value === rule.triggerSource) {
                opt.selected = true;
            }
            select.appendChild(opt);
        });
        
        select.onchange = function() {
            updateTransitionRule(rule.id, 'triggerSource', this.value);
        };
        
        return select;
    } else {
        // 默认或GPIO组，显示IO组选择框
        return renderIOGroupSelect(rule ? rule.triggerSource : null);
    }
}

// 渲染IO组选择下拉框
function renderIOGroupSelect(selectedGroup = null) {
    // 初始化默认IO组
    initializeDefaultIOGroups();
    
    // 创建select元素
    const select = document.createElement('select');
    select.className = 'trigger-source-select';
    
    // 添加默认选项
    const defaultOption = document.createElement('option');
    defaultOption.value = '';
    defaultOption.textContent = '选择GPIO组';
    defaultOption.selected = !selectedGroup;
    select.appendChild(defaultOption);
    
    // 添加所有IO组选项
    ioGroups.forEach(group => {
        const option = document.createElement('option');
        option.value = group.id;
        option.textContent = `GPIO组 ${group.id} (${group.name || '未命名'})`;
        option.selected = selectedGroup === group.id.toString();
        select.appendChild(option);
    });
    
    return select;
}

// 确保在DOM完全加载后初始化状态机
document.addEventListener('DOMContentLoaded', function() {
    try {
        // 初始化状态机控制功能
        initializeStateMachine();
        console.log('状态机页面DOM已完全加载并初始化');
    } catch (error) {
        console.error('状态机初始化失败:', error);
        console.error('错误堆栈:', error.stack);
        // 显示友好的错误提示给用户
        alert('状态机初始化失败: ' + error.message + '\n请刷新页面重试。');
    }
});