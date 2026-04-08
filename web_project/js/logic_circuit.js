// 全局变量
let selectedGateType = null;
let inputGroups = [];
let logicGates = [];
let outputGroups = [];
let gateCounter = 1;
let groupModules = {};  // 存储所有组的模块类型

// 页面加载完成后初始化逻辑电路功能
document.addEventListener('DOMContentLoaded', function() {
    initializeLogicCircuit();
    
    // 不再需要输入类型选择，移除相关事件监听器
});

// 测试连接功能
function testConnection() {
    var ipElement = document.getElementById('esp32-ip-logic');
    if (!ipElement) {
        alert('未找到IP地址输入框');
        return;
    }
    
    var ip = ipElement.value;
    if (!ip) {
        alert('请先输入ESP32的IP地址');
        return;
    }

    // 保存IP地址到localStorage
    localStorage.setItem('esp32_ip', ip);

    var xhr = new XMLHttpRequest();
    var url = "http://" + ip + "/gpio?test=connection";

    xhr.open("GET", url, true);
    xhr.timeout = 5000; // 5秒超时

    var statusElement = document.getElementById("connection-status");
    statusElement.innerHTML = "正在测试连接...";
    statusElement.className = "status info";

    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
            if (xhr.status === 200) {
                statusElement.innerHTML = "Status: Connected successfully to ESP32 at " + ip;
                statusElement.className = "status success";
            } else if (xhr.status === 0) {
                // 网络错误，可能设备未连接或离线环境
                statusElement.innerHTML = "Status: 无法连接到ESP32。请检查设备是否开启并连接到同一网络。";
                statusElement.className = "status error";
            } else {
                statusElement.innerHTML = "Status: Error - HTTP " + xhr.status;
                statusElement.className = "status error";
            }
        }
    };

    xhr.onerror = function() {
        var statusElement = document.getElementById("connection-status");
        statusElement.innerHTML = "Status: 网络错误。请检查设备连接或确认是否在离线环境中。";
        statusElement.className = "status error";
    };

    xhr.ontimeout = function() {
        var statusElement = document.getElementById("connection-status");
        statusElement.innerHTML = "Status: 连接超时。设备可能未响应或网络连接异常。";
        statusElement.className = "status error";
    };

    try {
        xhr.send();
    } catch (e) {
        var statusElement = document.getElementById("connection-status");
        statusElement.innerHTML = "Status: Error - " + e.message;
        statusElement.className = "status error";
    }
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
    var moduleType = document.getElementById('input-module-type').value;
    var table = document.getElementById('input-groups-table').getElementsByTagName('tbody')[0];
    
    // 检查是否已添加该组
    for (var i = 0; i < inputGroups.length; i++) {
        if (inputGroups[i].id == groupId) {
            showErrorMessage('该GPIO组已添加');
            return;
        }
    }
    
    // 添加到数组（不包含moduleType和type）
    var group = {
        id: parseInt(groupId),
        state: '' // 默认不关联状态值
    };
    inputGroups.push(group);
    
    // 将模块类型存储到groupModules对象中
    groupModules[groupId] = moduleType;
    
    // 添加到表格
    var row = table.insertRow();
    row.id = 'input-row-gpio-' + groupId;
    // 定义所有12个IO组的映射
    const pinMap = {
        0: [13,12], 
        1: [15,2], 
        2: [26,25], 
        3: [27,14],
        4: ['E1'], 
        5: ['E2'], 
        6: ['E3'], 
        7: ['E4'],
        8: ['E5'], 
        9: ['E6'], 
        10: ['E7'], 
        11: ['E8']
    };
    
    // 确定组类型和显示名称
    let groupType = "GPIO";
    let groupDisplayName = "GPIO 组 " + groupId;
    if (groupId >= 4) {
        groupType = "IO";
        groupDisplayName = "E" + (groupId - 3) + " (AW9523)";
    }
    
    // 模块类型显示名称
    var moduleTypeText = moduleType === 'led_button' ? '带灯按键' : '普通按键';
    
    row.innerHTML = `
        <td>${groupType}</td>
        <td>${groupDisplayName}</td>
        <td>
            <select onchange="updateInputGroupModuleType(${groupId}, this.value)">
                <option value="button" ${moduleType === 'button' ? 'selected' : ''}>普通按键</option>
                <option value="led_button" ${moduleType === 'led_button' ? 'selected' : ''}>带灯按键</option>
            </select>
        </td>
        <td>
            <select onchange="updateInputGroupState(${groupId}, this.value)">
                ${generateStateOptions('')}
            </select>
        </td>
        <td>
            <button class="btn-danger" onclick="removeInputGroup(${groupId})">删除</button>
        </td>
    `;
    
    // 更新同步历史
    updateSyncHistory();
}

// 更新输入组状态值
function updateInputGroupState(groupId, stateValue) {
    for (var i = 0; i < inputGroups.length; i++) {
        if (inputGroups[i].id == groupId) {
            inputGroups[i].state = stateValue;
            break;
        }
    }
    updateSyncHistory();
}

// 更新输入组模块类型
function updateInputGroupModuleType(groupId, moduleType) {
    // 更新groupModules对象
    groupModules[groupId] = moduleType;
    updateSyncHistory();
}

// 删除输入组
function removeInputGroup(groupId) {
    // 从数组中移除
    inputGroups = inputGroups.filter(function(group) {
        return group.id != groupId;
    });
    
    // 从groupModules中删除
    delete groupModules[groupId];
    
    // 从表格中移除行
    var row = document.getElementById('input-row-gpio-' + groupId);
    if (row) {
        row.remove();
    }
    
    // 更新同步历史
    updateSyncHistory();
}



// 更新输入组状态值
function updateInputGroupState(groupId, stateValue) {
    for (var i = 0; i < inputGroups.length; i++) {
        if (inputGroups[i].id == groupId) {
            inputGroups[i].state = stateValue;
            break;
        }
    }
    updateSyncHistory();
}

// 删除输入组
function removeInputGroup(groupId) {
    // 从数组中移除
    inputGroups = inputGroups.filter(function(group) {
        return group.id != groupId;
    });
    
    // 从表格中移除行
    var row = document.getElementById('input-row-gpio-' + groupId);
    if (row) {
        row.remove();
    }
    
    // 更新同步历史
    updateSyncHistory();
}

// 添加逻辑门
function addLogicGate() {
    var gateType = document.getElementById('gate-type-select').value;
    var gateId = 'gate' + gateCounter++;
    
    // 检查是否已添加该逻辑门
    for (var i = 0; i < logicGates.length; i++) {
        if (logicGates[i].id == gateId) {
            showErrorMessage('逻辑门ID冲突');
            return;
        }
    }
    
    // 添加到数组
    var gate = {
        id: gateId,
        type: gateType,
        inputs: ['', ''], // 默认不关联状态值
        output: '' // 默认不关联状态值
    };
    logicGates.push(gate);
    
    // 添加到表格
    var table = document.getElementById('logic-gates-table').getElementsByTagName('tbody')[0];
    var row = table.insertRow();
    row.id = 'gate-row-' + gateId;
    
    if (gateType === 'NOT') {
        row.innerHTML = `
            <td>${gateType}门</td>
            <td>
                <select onchange="updateLogicGateInput1('${gateId}', this.value)">
                    ${generateStateOptions('')}
                </select>
            </td>
            <td>
                <select onchange="updateLogicGateOutput('${gateId}', this.value)">
                    ${generateStateOptions('')}
                </select>
            </td>
            <td>
                <button class="btn-danger" onclick="removeLogicGate('${gateId}')">删除</button>
            </td>
        `;
    } else {
        row.innerHTML = `
            <td>${gateType}门</td>
            <td>
                <select onchange="updateLogicGateInput1('${gateId}', this.value)">
                    ${generateStateOptions('')}
                </select>
            </td>
            <td>
                <select onchange="updateLogicGateInput2('${gateId}', this.value)">
                    ${generateStateOptions('')}
                </select>
            </td>
            <td>
                <select onchange="updateLogicGateOutput('${gateId}', this.value)">
                    ${generateStateOptions('')}
                </select>
            </td>
            <td>
                <button class="btn-danger" onclick="removeLogicGate('${gateId}')">删除</button>
            </td>
        `;
    }
    
    // 更新同步历史
    updateSyncHistory();
}

// 更新逻辑门输入1
function updateLogicGateInput1(gateId, inputValue) {
    for (var i = 0; i < logicGates.length; i++) {
        if (logicGates[i].id == gateId) {
            logicGates[i].inputs[0] = inputValue;
            break;
        }
    }
    updateSyncHistory();
}

// 更新逻辑门输入2
function updateLogicGateInput2(gateId, inputValue) {
    for (var i = 0; i < logicGates.length; i++) {
        if (logicGates[i].id == gateId) {
            logicGates[i].inputs[1] = inputValue;
            break;
        }
    }
    updateSyncHistory();
}

// 更新逻辑门输出
function updateLogicGateOutput(gateId, outputValue) {
    for (var i = 0; i < logicGates.length; i++) {
        if (logicGates[i].id == gateId) {
            logicGates[i].output = outputValue;
            break;
        }
    }
    updateSyncHistory();
}

// 删除逻辑门
function removeLogicGate(gateId) {
    // 从数组中移除
    logicGates = logicGates.filter(function(gate) {
        return gate.id != gateId;
    });
    
    // 从表格中移除行
    var row = document.getElementById('gate-row-' + gateId);
    if (row) {
        row.remove();
    }
    
    // 更新同步历史
    updateSyncHistory();
}

// 添加输出组
function addOutputGroup() {
    var groupId = document.getElementById('output-group-select').value;
    var table = document.getElementById('output-groups-table').getElementsByTagName('tbody')[0];
    
    // 检查是否已添加该组
    for (var i = 0; i < outputGroups.length; i++) {
        if (outputGroups[i].id == groupId) {
            showErrorMessage('该GPIO组已添加');
            return;
        }
    }
    
    // 添加到数组（不包含moduleType）
    var group = {
        id: parseInt(groupId),
        state: '' // 默认不关联状态值
    };
    outputGroups.push(group);
    
    // 添加到表格
    var row = table.insertRow();
    row.id = 'output-row-' + groupId;
    // 定义所有12个IO组和2个电机的映射
    const pinMap = {
        0: [13,12], 
        1: [15,2], 
        2: [26,25], 
        3: [27,14],
        4: ['E1'], 
        5: ['E2'], 
        6: ['E3'], 
        7: ['E4'],
        8: ['E5'], 
        9: ['E6'], 
        10: ['E7'], 
        11: ['E8'],
        12: ['M1'], 
        13: ['M2']
    };
    
    // 确定组类型和显示名称
    let groupDisplayName;
    let moduleTypeControl = '';
    if (groupId >= 12) {
        // 电机输出
        groupDisplayName = pinMap[groupId][0] + " 电机 (状态: 0=停止, 1=正转)";
        moduleTypeControl = '-'; // 电机不需要模块类型
    } else if (groupId >= 4) {
        // AW9523扩展IO
        groupDisplayName = "E" + (groupId - 3) + " (AW9523)";
        // 添加模块类型选择（仅输出类型）
        moduleTypeControl = `
            <select onchange="updateOutputGroupModuleType(${groupId}, this.value)">
                <option value="">请选择</option>
                <option value="led">LED灯</option>
                <option value="traffic_light">交通灯</option>
                <option value="fan">风扇</option>
            </select>
        `;
    } else {
        // ESP32原生GPIO
        groupDisplayName = "GPIO 组 " + groupId + " (使用第一个引脚: GPIO " + pinMap[groupId][0] + ")";
        // 添加模块类型选择（仅输出类型）
        moduleTypeControl = `
            <select onchange="updateOutputGroupModuleType(${groupId}, this.value)">
                <option value="">请选择</option>
                <option value="led">LED灯</option>
                <option value="traffic_light">交通灯</option>
                <option value="fan">风扇</option>
            </select>
        `;
    }
    
    row.innerHTML = `
        <td>${groupDisplayName}</td>
        <td>${moduleTypeControl}</td>
        <td>
            <select onchange="updateOutputGroupState(${groupId}, this.value)">
                ${generateStateOptions('')}
            </select>
        </td>
        <td>
            <button class="btn-danger" onclick="removeOutputGroup(${groupId})"></button>
        </td>
    `;
    
    // 更新同步历史
    updateSyncHistory();
}

// 更新输出组状态值
function updateOutputGroupState(groupId, stateValue) {
    for (var i = 0; i < outputGroups.length; i++) {
        if (outputGroups[i].id == groupId) {
            outputGroups[i].state = stateValue;
            break;
        }
    }
    updateSyncHistory();
}

// 更新输出组模块类型
function updateOutputGroupModuleType(groupId, moduleType) {
    // 更新groupModules对象
    groupModules[groupId] = moduleType;
    updateSyncHistory();
}

// 删除输出组
function removeOutputGroup(groupId) {
    // 从数组中移除
    outputGroups = outputGroups.filter(function(group) {
        return group.id != groupId;
    });
    
    // 从groupModules中删除
    delete groupModules[groupId];
    
    // 从表格中移除行
    var row = document.getElementById('output-row-' + groupId);
    if (row) {
        row.remove();
    }
    
    // 更新同步历史
    updateSyncHistory();
}

// 生成逻辑图
function generateLogicDiagram() {
    var diagramContainer = document.getElementById('logic-diagram');
    diagramContainer.innerHTML = '';
    
    // 创建SVG元素
    var svgNS = "http://www.w3.org/2000/svg";
    var svg = document.createElementNS(svgNS, "svg");
    svg.setAttribute("width", "100%");
    svg.setAttribute("height", "600");
    svg.setAttribute("style", "border: 1px solid #ccc; background-color: #f9f9f9;");
    diagramContainer.appendChild(svg);
    
    // 添加标题
    var title = document.createElementNS(svgNS, "text");
    title.setAttribute("x", "50%");
    title.setAttribute("y", "30");
    title.setAttribute("text-anchor", "middle");
    title.textContent = "逻辑电路连接图";
    title.setAttribute("font-family", "Arial");
    title.setAttribute("font-size", "20");
    title.setAttribute("font-weight", "bold");
    svg.appendChild(title);
    
    if (inputGroups.length === 0 && logicGates.length === 0 && outputGroups.length === 0) {
        var noDataText = document.createElementNS(svgNS, "text");
        noDataText.setAttribute("x", "50%");
        noDataText.setAttribute("y", "250");
        noDataText.setAttribute("text-anchor", "middle");
        noDataText.textContent = "暂无数据，请添加输入组、逻辑门或输出组";
        noDataText.setAttribute("font-family", "Arial");
        noDataText.setAttribute("font-size", "16");
        noDataText.setAttribute("fill", "#666");
        svg.appendChild(noDataText);
        return;
    }
    
    // 计算模块位置
    var startY = 80;
    var moduleWidth = 120;
    var moduleHeight = 60;
    var horizontalSpacing = 200;
    var verticalSpacing = 80;
    
    // 存储模块位置信息
    var modulePositions = {
        inputs: {},
        gates: {},
        outputs: {}
    };
    
    // 绘制输入组模块
    var inputY = startY;
    inputGroups.forEach(function(group, index) {
        var x = 50;
        var y = inputY + index * verticalSpacing;
        
        // 存储位置信息
        modulePositions.inputs[group.id] = { x: x, y: y, width: moduleWidth, height: moduleHeight };
        
        // 绘制模块矩形
        var rect = document.createElementNS(svgNS, "rect");
        rect.setAttribute("x", x);
        rect.setAttribute("y", y);
        rect.setAttribute("width", moduleWidth);
        rect.setAttribute("height", moduleHeight);
        
        // 使用GPIO类型的颜色
        rect.setAttribute("fill", "#bbdefb");
        rect.setAttribute("stroke", "#1565c0");
        
        rect.setAttribute("stroke-width", "2");
        rect.setAttribute("rx", "5");
        svg.appendChild(rect);
        
        // 绘制模块文本
        var text = document.createElementNS(svgNS, "text");
        text.setAttribute("x", x + moduleWidth/2);
        text.setAttribute("y", y + moduleHeight/2 - 10);
        text.setAttribute("text-anchor", "middle");
        
        // 显示GPIO组文本
        var moduleText = "GPIO组 " + group.id;
        
        text.textContent = moduleText;
        text.setAttribute("font-family", "Arial");
        text.setAttribute("font-size", "12");
        text.setAttribute("font-weight", "bold");
        svg.appendChild(text);
        
        // 绘制状态值文本
        if (group.state) {
            var stateText = document.createElementNS(svgNS, "text");
            stateText.setAttribute("x", x + moduleWidth/2);
            stateText.setAttribute("y", y + moduleHeight/2 + 10);
            stateText.setAttribute("text-anchor", "middle");
            stateText.textContent = "状态: " + group.state;
            stateText.setAttribute("font-family", "Arial");
            stateText.setAttribute("font-size", "10");
            stateText.setAttribute("fill", "#666");
            svg.appendChild(stateText);
        }
    });
    
    // 绘制逻辑门模块 - 多列布局
    // 根据逻辑门数量和连接关系计算布局
    var gateColumns = {}; // 按列存储逻辑门
    var gateLevels = {};  // 存储每个逻辑门的层级
    
    // 首先确定每个逻辑门的层级（基于输入依赖关系）
    logicGates.forEach(function(gate) {
        gateLevels[gate.id] = 0; // 默认层级为0
    });
    
    // 多次遍历，确定依赖关系层级
    for (var iter = 0; iter < logicGates.length + 1; iter++) {
        logicGates.forEach(function(gate) {
            // 检查输入是否来自其他逻辑门的输出
            gate.inputs.forEach(function(input) {
                if (input) {
                    logicGates.forEach(function(otherGate) {
                        if (otherGate.output === input) {
                            // 如果当前逻辑门的输入来自其他逻辑门的输出，则层级应该更高
                            gateLevels[gate.id] = Math.max(gateLevels[gate.id], gateLevels[otherGate.id] + 1);
                        }
                    });
                }
            });
        });
    }
    
    // 根据层级分组逻辑门到不同列
    logicGates.forEach(function(gate) {
        var level = gateLevels[gate.id];
        if (!gateColumns[level]) {
            gateColumns[level] = [];
        }
        gateColumns[level].push(gate);
    });
    
    // 绘制逻辑门模块
    var columnCount = Object.keys(gateColumns).length;
    for (var col = 0; col < columnCount; col++) {
        if (!gateColumns[col]) continue;
        
        var gatesInColumn = gateColumns[col];
        var gatesCount = gatesInColumn.length;
        
        // 计算该列的起始Y位置，使逻辑门在垂直方向居中
        var columnHeight = gatesCount * verticalSpacing;
        var columnStartY = startY + Math.max(0, (600 - startY - columnHeight) / 2);
        
        gatesInColumn.forEach(function(gate, index) {
            var x = 50 + (col + 1) * horizontalSpacing;
            var y = columnStartY + index * verticalSpacing;
            
            // 存储位置信息
            modulePositions.gates[gate.id] = { x: x, y: y, width: moduleWidth, height: moduleHeight };
            
            // 根据逻辑门类型设置颜色
            var fillColor, strokeColor;
            switch(gate.type) {
                case 'AND':
                    fillColor = "#c8e6c9";
                    strokeColor = "#388e3c";
                    break;
                case 'OR':
                    fillColor = "#ffecb3";
                    strokeColor = "#ffa000";
                    break;
                case 'NOT':
                    fillColor = "#ffcdd2";
                    strokeColor = "#d32f2f";
                    break;
                default:
                    fillColor = "#f5f5f5";
                    strokeColor = "#666";
            }
            
            // 绘制模块矩形
            var rect = document.createElementNS(svgNS, "rect");
            rect.setAttribute("x", x);
            rect.setAttribute("y", y);
            rect.setAttribute("width", moduleWidth);
            rect.setAttribute("height", moduleHeight);
            rect.setAttribute("fill", fillColor);
            rect.setAttribute("stroke", strokeColor);
            rect.setAttribute("stroke-width", "2");
            rect.setAttribute("rx", "5");
            svg.appendChild(rect);
            
            // 绘制模块文本
            var text = document.createElementNS(svgNS, "text");
            text.setAttribute("x", x + moduleWidth/2);
            text.setAttribute("y", y + moduleHeight/2 - 10);
            text.setAttribute("text-anchor", "middle");
            text.textContent = gate.type + " (" + gate.id + ")";
            text.setAttribute("font-family", "Arial");
            text.setAttribute("font-size", "12");
            text.setAttribute("font-weight", "bold");
            svg.appendChild(text);
            
            // 绘制输出文本
            if (gate.output) {
                var outputText = document.createElementNS(svgNS, "text");
                outputText.setAttribute("x", x + moduleWidth/2);
                outputText.setAttribute("y", y + moduleHeight/2 + 10);
                outputText.setAttribute("text-anchor", "middle");
                outputText.textContent = "输出: " + gate.output;
                outputText.setAttribute("font-family", "Arial");
                outputText.setAttribute("font-size", "10");
                outputText.setAttribute("fill", "#666");
                svg.appendChild(outputText);
            }
        });
    }
    
    // 绘制输出组模块
    var outputY = startY;
    outputGroups.forEach(function(group, index) {
        var x = 50 + (columnCount + 1) * horizontalSpacing;
        var y = outputY + index * verticalSpacing;
        
        // 存储位置信息
        modulePositions.outputs[group.id] = { x: x, y: y, width: moduleWidth, height: moduleHeight };
        
        // 绘制模块矩形
        var rect = document.createElementNS(svgNS, "rect");
        rect.setAttribute("x", x);
        rect.setAttribute("y", y);
        rect.setAttribute("width", moduleWidth);
        rect.setAttribute("height", moduleHeight);
        rect.setAttribute("fill", "#e1bee7");
        rect.setAttribute("stroke", "#7b1fa2");
        rect.setAttribute("stroke-width", "2");
        rect.setAttribute("rx", "5");
        svg.appendChild(rect);
        
        // 绘制模块文本
        var text = document.createElementNS(svgNS, "text");
        text.setAttribute("x", x + moduleWidth/2);
        text.setAttribute("y", y + moduleHeight/2 - 10);
        text.setAttribute("text-anchor", "middle");
        
        // 根据ID确定输出类型
        var outputText = "输出组 " + group.id;
        if (group.id === 12) {
            outputText = "M1 电机";
        } else if (group.id === 13) {
            outputText = "M2 电机";
        } else if (group.id >= 4 && group.id <= 11) {
            outputText = "E" + (group.id - 3);
        }
        
        text.textContent = outputText;
        text.setAttribute("font-family", "Arial");
        text.setAttribute("font-size", "12");
        text.setAttribute("font-weight", "bold");
        svg.appendChild(text);
        
        // 绘制状态值文本
        if (group.state) {
            var stateText = document.createElementNS(svgNS, "text");
            stateText.setAttribute("x", x + moduleWidth/2);
            stateText.setAttribute("y", y + moduleHeight/2 + 10);
            stateText.setAttribute("text-anchor", "middle");
            stateText.textContent = "状态: " + group.state;
            stateText.setAttribute("font-family", "Arial");
            stateText.setAttribute("font-size", "10");
            stateText.setAttribute("fill", "#666");
            svg.appendChild(stateText);
        }
    });
    
    // 绘制连接线
    var connections = [];
    
    // 输入组到逻辑门的连接
    inputGroups.forEach(function(inputGroup) {
        if (inputGroup.state) {
            logicGates.forEach(function(gate) {
                gate.inputs.forEach(function(input, index) {
                    if (input === inputGroup.state) {
                        connections.push({
                            from: "input_" + inputGroup.id,
                            to: "gate_" + gate.id + "_input_" + index,
                            fromModule: "inputs",
                            fromId: inputGroup.id,
                            toModule: "gates",
                            toId: gate.id,
                            text: inputGroup.state
                        });
                    }
                });
            });
        }
    });
    
    // 逻辑门到逻辑门的连接
    logicGates.forEach(function(gate) {
        if (gate.output) {
            logicGates.forEach(function(otherGate) {
                if (otherGate.id !== gate.id) {
                    otherGate.inputs.forEach(function(input, index) {
                        if (input === gate.output) {
                            connections.push({
                                from: "gate_" + gate.id,
                                to: "gate_" + otherGate.id + "_input_" + index,
                                fromModule: "gates",
                                fromId: gate.id,
                                toModule: "gates",
                                toId: otherGate.id,
                                text: gate.output
                            });
                        }
                    });
                }
            });
        }
    });
    
    // 逻辑门到输出组的连接
    logicGates.forEach(function(gate) {
        if (gate.output) {
            outputGroups.forEach(function(outputGroup) {
                if (outputGroup.state === gate.output) {
                    connections.push({
                        from: "gate_" + gate.id,
                        to: "output_" + outputGroup.id,
                        fromModule: "gates",
                        fromId: gate.id,
                        toModule: "outputs",
                        toId: outputGroup.id,
                        text: gate.output
                    });
                }
            });
        }
    });
    
    // 绘制连接线
    connections.forEach(function(conn, index) {
        // 获取起始和结束模块的位置
        var fromPos = modulePositions[conn.fromModule][conn.fromId];
        var toPos = modulePositions[conn.toModule][conn.toId];
        
        if (fromPos && toPos) {
            // 计算起始点和结束点（模块的中心右侧到中心左侧）
            var startX = fromPos.x + fromPos.width;
            var startY = fromPos.y + fromPos.height / 2;
            var endX = toPos.x;
            var endY = toPos.y + toPos.height / 2;
            
            // 绘制连接线
            var line = document.createElementNS(svgNS, "line");
            line.setAttribute("x1", startX);
            line.setAttribute("y1", startY);
            line.setAttribute("x2", endX);
            line.setAttribute("y2", endY);
            line.setAttribute("stroke", "#666");
            line.setAttribute("stroke-width", "2");
            line.setAttribute("marker-end", "url(#arrowhead)");
            svg.appendChild(line);
            
            // 在连接线中间添加标签
            var midX = (startX + endX) / 2;
            var midY = (startY + endY) / 2;
            
            var labelBg = document.createElementNS(svgNS, "rect");
            labelBg.setAttribute("x", midX - 20);
            labelBg.setAttribute("y", midY - 10);
            labelBg.setAttribute("width", 40);
            labelBg.setAttribute("height", 20);
            labelBg.setAttribute("fill", "white");
            labelBg.setAttribute("stroke", "none");
            svg.appendChild(labelBg);
            
            var labelText = document.createElementNS(svgNS, "text");
            labelText.setAttribute("x", midX);
            labelText.setAttribute("y", midY + 5);
            labelText.setAttribute("text-anchor", "middle");
            labelText.textContent = conn.text;
            labelText.setAttribute("font-family", "Arial");
            labelText.setAttribute("font-size", "10");
            labelText.setAttribute("fill", "#333");
            svg.appendChild(labelText);
        }
    });
    
    // 添加箭头定义
    var defs = document.createElementNS(svgNS, "defs");
    var marker = document.createElementNS(svgNS, "marker");
    marker.setAttribute("id", "arrowhead");
    marker.setAttribute("markerWidth", "10");
    marker.setAttribute("markerHeight", "7");
    marker.setAttribute("refX", "10");
    marker.setAttribute("refY", "3.5");
    marker.setAttribute("orient", "auto");
    var polygon = document.createElementNS(svgNS, "polygon");
    polygon.setAttribute("points", "0 0, 10 3.5, 0 7");
    polygon.setAttribute("fill", "#666");
    marker.appendChild(polygon);
    defs.appendChild(marker);
    svg.appendChild(defs);
    
    // 如果没有连接，显示提示信息
    if (connections.length === 0 && (inputGroups.length > 0 || logicGates.length > 0 || outputGroups.length > 0)) {
        var noConnText = document.createElementNS(svgNS, "text");
        noConnText.setAttribute("x", "50%");
        noConnText.setAttribute("y", "500");
        noConnText.setAttribute("text-anchor", "middle");
        noConnText.textContent = "暂无连接";
        noConnText.setAttribute("font-family", "Arial");
        noConnText.setAttribute("font-size", "14");
        noConnText.setAttribute("fill", "#666");
        svg.appendChild(noConnText);
    }
}

// 测试逻辑电路
function testLogicCircuit() {
    var statusElement = document.getElementById("sync-status");
    if (statusElement) {
        updateStatus(statusElement, "正在测试逻辑电路配置...", "info");
    }

    // 检查是否存在配置
    if (inputGroups.length === 0 && logicGates.length === 0 && outputGroups.length === 0) {
        if (statusElement) {
            updateStatus(statusElement, "测试失败: 没有配置任何逻辑电路组件", "error");
        }
        return;
    }

    // 验证输入组配置
    var inputErrors = [];
    for (var i = 0; i < inputGroups.length; i++) {
        var group = inputGroups[i];
        if (!group.state || group.state.trim() === "") {
            var groupName = "GPIO组 " + group.id;
            inputErrors.push(groupName + " 未关联状态值");
        }
    }

    // 验证逻辑门配置
    var gateErrors = [];
    for (var i = 0; i < logicGates.length; i++) {
        var gate = logicGates[i];
        
        // 检查逻辑门类型
        if (!gate.type || gate.type.trim() === "") {
            gateErrors.push("逻辑门 " + gate.id + " 未选择类型");
            continue;
        }
        
        // 非门只需要一个输入
        if (gate.type === "NOT") {
            if (!gate.inputs[0] || gate.inputs[0].trim() === "") {
                gateErrors.push("非门 " + gate.id + " 未连接输入1");
            }
            if (gate.inputs[1] && gate.inputs[1].trim() !== "") {
                gateErrors.push("非门 " + gate.id + " 不应连接输入2");
            }
        } else {
            // 与门和或门需要两个输入
            if (!gate.inputs[0] || gate.inputs[0].trim() === "") {
                gateErrors.push(gate.type + "门 " + gate.id + " 未连接输入1");
            }
            if (!gate.inputs[1] || gate.inputs[1].trim() === "") {
                gateErrors.push(gate.type + "门 " + gate.id + " 未连接输入2");
            }
        }
        
        // 检查输出
        if (!gate.output || gate.output.trim() === "") {
            gateErrors.push(gate.type + "门 " + gate.id + " 未连接输出");
        }
    }

    // 验证输出组配置
    var outputErrors = [];
    for (var i = 0; i < outputGroups.length; i++) {
        if (!outputGroups[i].state || outputGroups[i].state.trim() === "") {
            outputErrors.push("输出组 " + outputGroups[i].id + " 未关联状态值");
        }
    }

    // 检查状态值重复使用情况（仅作为警告）
    var stateUsage = {};
    var warnings = [];
    
    // 收集所有状态值的使用情况
    inputGroups.forEach(function(group) {
        if (group.state && group.state.trim() !== "") {
            if (!stateUsage[group.state]) {
                stateUsage[group.state] = [];
            }
            var groupName = "GPIO组 " + group.id;
            stateUsage[group.state].push(groupName);
        }
    });
    
    logicGates.forEach(function(gate) {
        // 检查输入
        gate.inputs.forEach(function(input, index) {
            if (input && input.trim() !== "") {
                if (!stateUsage[input]) {
                    stateUsage[input] = [];
                }
                stateUsage[input].push("逻辑门 " + gate.id + " 输入" + (index + 1));
            }
        });
        
        // 检查输出
        if (gate.output && gate.output.trim() !== "") {
            if (!stateUsage[gate.output]) {
                stateUsage[gate.output] = [];
            }
            stateUsage[gate.output].push("逻辑门 " + gate.id + " 输出");
        }
    });
    
    outputGroups.forEach(function(group) {
        if (group.state && group.state.trim() !== "") {
            if (!stateUsage[group.state]) {
                stateUsage[group.state] = [];
            }
            stateUsage[group.state].push("输出组 " + group.id);
        }
    });
    
    // 检查重复使用的情况
    for (var state in stateUsage) {
        if (stateUsage[state].length > 1) {
            warnings.push("状态值 " + state + " 被多次使用: " + stateUsage[state].join(", "));
        }
    }

    // 汇总所有错误和警告
    var allErrors = inputErrors.concat(gateErrors, outputErrors);
    
    if (allErrors.length > 0) {
        var errorMsg = "配置错误:<br>" + allErrors.join("<br>");
        if (warnings.length > 0) {
            errorMsg += "<br><br>警告:<br>" + warnings.join("<br>");
        }
        if (statusElement) {
            updateStatus(statusElement, errorMsg, "error");
        }
    } else if (warnings.length > 0) {
        var warningMsg = "逻辑电路配置基本正确<br><br>警告:<br>" + warnings.join("<br>");
        if (statusElement) {
            updateStatus(statusElement, warningMsg, "warning");
        }
    } else {
        if (statusElement) {
            updateStatus(statusElement, "逻辑电路配置测试通过，没有发现错误", "success");
        }
    }
}

// 同步配置到ESP32设备
function syncConfigToESP() {
    var ip = document.getElementById('esp32-ip-logic').value;
    if (!ip) {
        alert('请先输入ESP32的IP地址');
        return;
    }
    
    // 保存IP地址到localStorage
    localStorage.setItem('esp32-ip', ip);
    
    var statusElement = document.getElementById("sync-status");
    if (statusElement) {
        updateStatus(statusElement, "正在同步配置到ESP设备...", "warning");
    }
    
    // 检查网络连接状态
    if (!navigator.onLine) {
        if (statusElement) {
            showNetworkError(statusElement);
        }
        return;
    }
    
    // 构建要发送到ESP32的数据
    var configData = {
        inputGroups: inputGroups,
        logicGates: logicGates,
        outputGroups: outputGroups,
        // 始终添加groupModes字段，确保前后端JSON格式一致
        groupModes: {}
    };
    
    // 从输入组和输出组中提取模式信息，与状态机保持一致
    const groupModes = {};
    
    // 添加输入组模式（默认模式为1，输入）
    if (inputGroups && Array.isArray(inputGroups)) {
        inputGroups.forEach(group => {
            if (group && group.id !== undefined) {
                groupModes[group.id] = group.mode !== undefined ? group.mode : 1;
            }
        });
    }
    
    // 添加输出组模式（默认模式为0，输出）
    if (outputGroups && Array.isArray(outputGroups)) {
        outputGroups.forEach(group => {
            if (group && group.id !== undefined) {
                groupModes[group.id] = group.mode !== undefined ? group.mode : 0;
            }
        });
    }
    
    // 如果存在模式信息，则使用它
    if (Object.keys(groupModes).length > 0) {
        configData.groupModes = groupModes;
    }
    
    // 如果存在模块类型信息，则添加到配置中（直接使用全局groupModules对象）
    if (Object.keys(groupModules).length > 0) {
        configData.groupModules = groupModules;
    }
    
    // 发送配置到ESP32
    var xhr = new XMLHttpRequest();
    // 优先使用新的端点，同时保持向后兼容性
    var url = "http://" + ip + "/logic";
    
    xhr.open("POST", url, true);
    xhr.setRequestHeader("Content-Type", "application/json");
    xhr.timeout = 10000; // 10秒超时
    
    xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
            if (xhr.status === 200) {
                try {
                    var response = JSON.parse(xhr.responseText);
                    if (response.status === "success") {
                        if (statusElement) {
                            updateStatus(statusElement, "配置已成功同步到ESP设备!", "success");
                        }
                        
                        // 存储最后同步时间
                        localStorage.setItem('last_sync_time', new Date().toISOString());
                        
                        // 更新同步历史记录
                        updateSyncHistory(ip);
                    } else {
                        if (statusElement) {
                            updateStatus(statusElement, "同步失败: " + (response.message || "未知错误"), "error");
                        }
                    }
                } catch (e) {
                    if (statusElement) {
                        updateStatus(statusElement, "解析响应失败: " + e.message, "error");
                    }
                }
            } else if (xhr.status === 0) {
                // 网络错误
                if (statusElement) {
                    showNetworkError(statusElement);
                }
            } else {
                if (statusElement) {
                    showServerError(statusElement);
                }
            }
        }
    };
    
    xhr.onerror = function() {
        var statusElement = document.getElementById("sync-status");
        if (statusElement) {
            showNetworkError(statusElement);
        }
    };
    
    xhr.ontimeout = function() {
        var statusElement = document.getElementById("sync-status");
        if (statusElement) {
            updateStatus(statusElement, "连接超时，请重试", "error");
        }
    };
    
    try {
        xhr.send(JSON.stringify(configData));
    } catch (e) {
        var statusElement = document.getElementById("sync-status");
        if (statusElement) {
            updateStatus(statusElement, "发送数据时出错: " + e.message, "error");
        }
    }
}

// 显示通用错误消息
function showErrorMessage(message) {
    var statusElement = document.getElementById("sync-status");
    updateStatus(statusElement, message, "error");
    
    // 在控制台记录错误
    console.error("同步错误: " + message);
    
    // 显示提示信息
    showUserPrompt("提示", message + "。请检查您的输入并重试。");
}

// 显示网络错误消息
function showNetworkError(element) {
    updateStatus(element, "网络连接不可用，请检查您的互联网连接", "error");
    showUserPrompt("网络错误", "您似乎处于离线状态。请检查您的网络连接并重试。");
    
    // 监听网络状态变化
    window.addEventListener('online', function() {
        showUserPrompt("网络恢复", "网络已恢复，您可以尝试重新同步配置。");
    });
}

// 显示服务器错误消息
function showServerError(element) {
    updateStatus(element, "无法连接到ESP设备，请重试", "error");
    
    // 提供重试选项
    var retryButton = document.createElement("button");
    retryButton.textContent = "重试";
    retryButton.className = "btn-retry";
    retryButton.onclick = function() {
        syncConfigToESP();
        retryButton.remove();
    };
    
    statusContainer.appendChild(retryButton);
}

// 更新状态显示
function updateStatus(element, message, type) {
    element.innerHTML = message;
    element.className = "status " + type;
    
    // 如果有状态变化监听器，触发事件
    if (typeof onStatusChange === "function") {
        onStatusChange(message, type);
    }
}

// 显示用户提示
function showUserPrompt(title, message) {
    // 如果浏览器支持桌面通知
    if (Notification.permission === "granted") {
        new Notification(title, { body: message });
    }
    
    // 在页面上显示提示
    var promptElement = document.createElement("div");
    promptElement.className = "user-prompt";
    promptElement.innerHTML = "<strong>" + title + ":</strong> " + message;
    
    var promptContainer = document.getElementById("prompt-container");
    if (!promptContainer) {
        promptContainer = document.createElement("div");
        promptContainer.id = "prompt-container";
        document.body.appendChild(promptContainer);
    }
    
    promptContainer.appendChild(promptElement);
    
    // 5秒后自动移除提示
    setTimeout(function() {
        promptElement.fadeOut(500, function() {
            promptElement.remove();
        });
    }, 5000);
}

// 更新同步历史记录
function updateSyncHistory(ip) {
    var history = JSON.parse(localStorage.getItem('sync_history') || '[]');
    
    var historyEntry = {
        timestamp: new Date().toISOString(),
        ip: ip,
        success: true
    };
    
    history.push(historyEntry);
    
    // 保留最近10条记录
    if (history.length > 10) {
        history = history.slice(-10);
    }
    
    localStorage.setItem('sync_history', JSON.stringify(history));
    
    // 如果有同步历史记录表格，更新它
    var historyTable = document.getElementById("sync-history-table");
    if (historyTable) {
        var tbody = historyTable.querySelector("tbody");
        if (!tbody) {
            tbody = document.createElement("tbody");
            historyTable.appendChild(tbody);
        }
        
        // 添加新记录
        var row = tbody.insertRow(0);
        var dateCell = row.insertCell(0);
        var ipCell = row.insertCell(1);
        
        dateCell.textContent = new Date().toLocaleString();
        ipCell.textContent = ip;
    }
}

// 重置配置
function resetConfiguration() {
    if (confirm("确定要重置所有配置吗？")) {
            // 重置所有数组
        inputGroups = [];
        logicGates = [];
        outputGroups = [];
        gateCounter = 1;
        selectedGateType = null;
        
        // 清空所有表格
        document.querySelector('#input-groups-table tbody').innerHTML = '';
        document.querySelector('#logic-gates-table tbody').innerHTML = '';
        document.querySelector('#output-groups-table tbody').innerHTML = '';
        
        // 重置逻辑门选择
        var gates = document.querySelectorAll('.logic-gate');
        gates.forEach(function(gate) {
            gate.classList.remove('selected');
        });
        
        // 清空逻辑图
        document.getElementById('logic-diagram').innerHTML = '';
        
        // 更新状态
        var statusElement = document.getElementById("sync-status");
        updateStatus(statusElement, "配置已重置", "warning");
        
        // 清除同步历史记录
        localStorage.removeItem('sync_history');
        
        // 如果有同步历史表格，清空它
        var historyTable = document.getElementById("sync-history-table");
        if (historyTable) {
            var tbody = historyTable.querySelector("tbody");
            if (tbody) {
                tbody.innerHTML = '';
            }
        }
    }
}

// 初始化逻辑电路功能
function initializeLogicCircuit() {
    // 恢复保存的IP地址
    var savedIP = localStorage.getItem('esp32-ip');
    if (savedIP) {
        document.getElementById('esp32-ip-logic').value = savedIP;
    }
}

// 页面加载完成后初始化逻辑电路功能
document.addEventListener('DOMContentLoaded', function() {
    initializeLogicCircuit();
});