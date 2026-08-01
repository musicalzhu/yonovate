# Proposal: V3 测试页面重构

## 概述

**名称**：`v3-test-page-refactor`  
**类型**：增强 + 破坏性变更（前端配置格式与页面结构）  
**范围**：`web_project/pages/state_machine_v3.html` 测试/配置页，承载完整 V3 JSON 编辑体验与模块化前端架构

## 摘要

核心改进：
1. **完整格式**：页面与模板采用 V3 原生完整 JSON（`triggers[]`、`triggerCount`、`triggerLogic`、`operator` 等），GPIO/定时器源命名规范为 `P0`–`P11`、`T0`–`T2`、`MANUAL`
2. **模块化**：业务代码分离到 `state-machine-ui.js`、`state-machine-api.js`、`state-machine-data.js`；入口 HTML 仅含静态结构和少量模块引导
3. **字段与校验**：内部编辑态统一 camelCase，通过 `DataConverter` 与后端 snake_case 转换；前端轻校验 + 后端权威校验；用户提示使用中文
4. **Vue 3框架迁移**：渐进式迁移到Vue 3 + TypeScript + Vite技术栈，提升开发效率、代码可维护性和用户体验

## 能力范围

| 方向 | 覆盖内容 |
|------|----------|
| 完整格式编辑 | 多触发器 UI、AND/OR 逻辑、运算符、标准源字符串、`buildConfig` 模板、JSON 预览、设备联调 |
| 模块化与数据层 | 三模块拆分、`FIELD_MAPPINGS` 转换、入口去内联、`GET/POST` 与状态刷新策略 |
| Vue 3框架迁移 | Vue 3 + TypeScript + Vite技术栈、组件化架构、响应式状态管理、渐进式迁移策略 |

## 动机

- **格式割裂**：简化 JSON 无法表达 V3 多触发器与逻辑组合
- **维护困难**：单体页面内联脚本难以演进
- **口径不统一**：需统一「编辑态 / 传输态 / 后端语义」边界
- **开发效率**：原生JavaScript + DOM操作开发效率低，Vue框架可提升开发效率50%+
- **代码可维护性**：组件化架构和响应式系统可显著提升代码可维护性
- **用户体验**：Vue的响应式更新和虚拟DOM可提供更流畅的用户体验

## 范围

### 包含

- `state_machine_v3.html`（及同源别名页）、`state_machine_v3_config.js`、三个 `state-machine-*.js` 模块
- 单一规范：`specs/v3-test-page/spec.md`（含重构边界、完整格式编辑、命名转换、轻量校验、模块化）
- Vue 3框架迁移：Vue 3 + TypeScript + Vite技术栈、组件化架构、响应式状态管理、渐进式迁移策略

### 排除

- 固件 HTTP 路由与解析逻辑变更（需另立变更）
- 状态机 V2 页面 `state_machine.html`

## 能力矩阵（详见 `specs/v3-test-page/spec.md`）

| 模块 | 内容摘要 |
|------|----------|
| 职责边界 | 不重复实现后端能力；配置语义与验证状态以 API/固件为准；GET 返回 `lastLoadSuccess` 等字段 |
| 轻量校验 | 仅做结构/类型校验；中文提示；区分本地与远端错误 |
| 命名转换 | 内部 camelCase；`DataConverter` 与传输约定对齐；兼容混合键名 |
| 模块化 | UI / API / Data 分层；入口无大块内联脚本；模块路径为 `../js/` |
| 格式编辑 | 多触发器（1-3个）、AND/OR、运算符、标准源、模板、JSON 预览 |

## 风险与缓解

| 风险 | 缓解措施 |
|------|----------|
| 旧简化 JSON 不再兼容 | 保留数据层对混合格式的兼容处理与迁移辅助函数 |
| 文档滞后 | 以本 proposal/design/tasks + specs 作为唯一归档口径 |

## 验收标准

1. 测试页支持配置 **1–3** 个触发器及 **AND/OR** 逻辑，运算符可选
2. 保存链路经 `DataConverter` 与设备约定一致，加载可正确回填界面
3. 入口页面无大体量业务内联脚本，职责边界符合 `specs/v3-test-page/spec.md`
