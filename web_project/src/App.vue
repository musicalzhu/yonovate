<!--
   Vue 3.5+ 应用根组件

   【功能说明】
   应用的入口组件，是 Vue 应用的根组件。
   负责渲染路由视图（router-view），显示当前路由对应的页面。

   【核心概念】
   1. Router View: Vue Router 提供的路由视图组件，根据 URL 渲染对应页面
   2. 全局样式: 通过 #app 容器设置全局字体平滑
   3. 组件挂载: onMounted 生命周期钩子用于初始化操作

   【应用架构】
   App.vue (根组件)
   └── router-view (路由视图)
       ├── Home.vue (首页)
       ├── DeviceControlPage.vue (设备控制)
       ├── GpioControl.vue (GPIO控制)
       ├── WifiConfig.vue (WiFi配置)
       ├── LogicCircuit.vue (逻辑电路)
       └── StateMachineV3Page.vue (状态机)

   【适用场景】
   - 应用入口点
   - 全局布局容器
   - 路由视图渲染
 -->
<template>
  <div id="app">
    <router-view />
  </div>
</template>

<script setup lang="ts"></script>

<style scoped>
/**
 * 应用根容器样式
 * 启用字体平滑渲染，提升显示效果
 */
#app {
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
}
</style>
