// 标签页切换功能
function openTab(evt, tabName) {
    // 隐藏所有标签内容
    var tabContents = document.getElementsByClassName("tab-content");
    for (var i = 0; i < tabContents.length; i++) {
        tabContents[i].classList.remove("active");
    }

    // 取消选中所有标签按钮
    var tabButtons = document.getElementsByClassName("tab-button");
    for (var i = 0; i < tabButtons.length; i++) {
        tabButtons[i].classList.remove("active");
    }

    // 显示当前标签内容
    document.getElementById(tabName).classList.add("active");
    evt.currentTarget.classList.add("active");

    // 根据标签初始化相应功能
    if (tabName === 'gpio-control') {
        initializeGPIOControl();
    } else if (tabName === 'logic-circuit') {
        initializeLogicCircuit();
    }
}

// 初始化GPIO控制功能
function initializeGPIOControl() {
    restoreSavedIP();
}

// 初始化逻辑电路功能
function initializeLogicCircuit() {
    // 恢复保存的IP地址到逻辑电路页面
    var savedIP = localStorage.getItem('esp32-ip');
    if (savedIP) {
        document.getElementById('esp32-ip-logic').value = savedIP;
    }
}

// 恢复保存的IP地址
function restoreSavedIP() {
    var savedIP = localStorage.getItem('esp32-ip');
    if (savedIP) {
        document.getElementById('esp32-ip').value = savedIP;
    }
}

// 页面加载完成后默认显示初始化
document.addEventListener('DOMContentLoaded', function() {
    // 检查当前页面并执行相应的初始化
    if (document.getElementById('esp32-ip-logic')) {
        initializeLogicCircuit();
    } else if (document.getElementById('esp32-ip')) {
        restoreSavedIP();
    }
});