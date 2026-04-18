# term4k 架构与模块分工说明

[返回 README](../README.md) | [English Version](./ARCHITECTURE.md)

本文档说明 `term4k` 当前各大模块的职责分工、结构联系、包含关系，以及 `src` 下各类（class/type）的作用。

> 说明范围：依据当前代码目录与 `CMakeLists.txt` 中 `TERM4K_CORE_SOURCES` / 公共头文件清单整理。

## 1）总体分层

项目按游戏领域子系统而非技术层次组织目录：

```text
UI 渲染层     (ui/*)       — FTXUI 渲染组件与共享 UI 基础设施
场景层        (scenes/*)   — 各屏幕的状态持有者（数据 + 编排）
领域层        (gameplay/*, chart/*, account/*, audio/*, platform/*)
数据层        (data/*)     — 跨子系统共享的纯值类型
工具层        (utils/*)    — 无领域依赖的通用工具
```

典型运行链路：

1. `UIBus` 驱动顶层场景循环。
2. `*UI` 组件负责屏幕渲染，并将用户输入委托给配对的 `*Scene`。
3. Scene 协调领域子系统（`GameplaySession`、`ChartCatalog`、`UserSession` 等）。
4. 领域类通过 `account/*` 读写持久化数据，并操作 `data/*` 值类型。
5. `platform/*` 为各子系统提供全局配置、国际化和应用目录解析。
6. `utils/*` 提供通用工具（JSON、加密、前缀树、错误上报），项目全局使用。

## 2）目录包含关系与结构联系

```text
src/
  data/       纯值类型与游戏快照（无子系统依赖）
  gameplay/   游戏引擎：会话、时钟、统计、谱面解析、成绩写入
  chart/      谱目扫描与 .t4k 文件解析
  account/    用户认证、会话状态与成绩记录存储
  audio/      音频播放封装（基于 miniaudio）
  platform/   应用目录、运行时配置、国际化、主题商店、设置 I/O
  scenes/     各屏幕状态持有者（编排领域子系统）
  ui/         FTXUI 渲染组件 + UI 总线、配色、主题适配器
  utils/      通用工具：JSON、加密、前缀树、错误上报
```

模块关系重点：

- `ui/*` 渲染 `scenes/*` 中的数据；每个 `*UI` 类引用对应的 `*Scene`。
- `scenes/*` 组合调用领域子系统（`gameplay/`、`chart/`、`account/`、`platform/`）。
- `gameplay/*` 生产与消费 `data/*` 值类型。
- `account/*` 与 `chart/*` 均依赖 `data/*` 共享实体类型。
- `platform/RuntimeConfig` 与 `platform/AppDirs` 被大多数子系统访问。
- `utils/*` 不依赖任何领域层或场景层。

## 3）大模块职责

- **游玩链路**：`GameplaySession` + `GameplayClock` + `GameplayStats` + `GameplayChartParser` 负责谱面解析、时钟推进、判定计分与最终结算。
- **曲目目录链路**：`ChartCatalog` + `ChartListScene` + `PrefixTrie` 负责曲目发现、排序、搜索与合规检查。
- **账户会话链路**：`UserStore`、`UserSession`、`UserContext`、`UserStatScene` 负责注册登录、会话状态与历史成绩。
- **成绩记录链路**：`GameplayRecordWriter` 与 `RecordStore` 负责成绩入库与链式校验。
- **设置与国际化链路**：`RuntimeConfig`、`SettingsDraft`、`SettingsIO`、`SettingsScene`、`I18n` 负责偏好设置与语言资源。
- **UI 链路**：`UIBus` 驱动场景路由；`*UI` 组件负责各屏幕渲染；`ThemeAdapter`、`UIColors`、`MessageOverlay`、`TransitionBackdrop` 提供共享 UI 基础设施。

## 4）各类作用（逐模块）

### 4.1 data

| 类 / 类型 | 作用 |
|---|---|
| `Chart` | 谱面元数据模型（ID、显示名、BPM、键数、预览段、基础节奏等），含序列化能力。 |
| `GameplayTapNote` | Tap 音符值（轨道 + 时间）。 |
| `GameplayHoldNote` | Hold 音符值（轨道 + 起止时间）。 |
| `GameplayChartData` | 游玩用谱面数据容器（tap/hold、结尾时刻、音符数、最大分）。 |
| `GameplayJudgement` | 判定枚举（`Perfect`、`Great`、`Miss`）。 |
| `GameplaySnapshot` | 游玩过程中的实时状态快照，用于渲染层读取。 |
| `GameplayFinalResult` | 结算后的最终结果模型，用于成绩持久化。 |
| `GameplayClockSnapshot` | 音频/谱面时钟快照（驱动模式 + 完成状态）。 |
| `SettingsDraft` | 可编辑设置草稿模型，用于设置页改动管理。 |
| `Rating` | 历史成绩记录模型，支持序列化。 |
| `User` | 用户模型（UID、用户名、密码）。 |

### 4.2 gameplay

| 类 | 作用 |
|---|---|
| `GameplayChartParser` | 将 `.t4k` 谱面文件转换为供会话使用的 `GameplayChartData`。 |
| `GameplayClock` | 管理音频时钟与谱面时钟的同步策略。 |
| `GameplayStats` | 判定计分、连击与准确率累计，生成 `GameplaySnapshot` 与 `GameplayFinalResult`。 |
| `GameplaySession` | 核心游玩状态机：按键输入、判定结算、轨道窗口推进、结果就绪判断。 |
| `GameplayRecordWriter` | 将当前认证用户的结算结果写入成绩记录。 |

### 4.3 chart

| 类 / 类型 | 作用 |
|---|---|
| `ChartEvent` | 谱面事件单元（`type` + `data`）。 |
| `ChartFileReader` | `.t4k` 谱面文件静态解析与十六进制辅助转换。 |
| `ChartListSortKey` | 曲目排序字段枚举。 |
| `SortOrder` | 排序方向枚举（升序 / 降序）。 |
| `ChartDetectionIssue` | 曲目目录扫描失败类型枚举。 |
| `ChartPlayStats` | 单曲游玩统计（次数、最高分、最佳准度、FC/AP/ULT 标志）。 |
| `ChartDetectionFailure` | 目录扫描失败明细（路径、原因、文案）。 |
| `ChartCatalogEntry` | 曲目目录项（元数据路径 + 用户统计）。 |
| `ChartRecordEntry` | 业务层成绩记录项（用户、曲目、分数、时间等）。 |
| `ChartRecordCollection` | 记录集合（Map + 展示顺序）。 |
| `ChartCatalog` | 曲目扫描、用户统计聚合、排序与合规检查服务。 |

### 4.4 account

| 类 | 作用 |
|---|---|
| `RecordStore` | 防篡改成绩数据库（全静态）；以哈希链保护历史成绩，提供按条件查询。 |
| `UserStore` | 用户账户文件访问层（全静态）；负责加盐密码校验与 UID 查询。 |
| `UserSession` | 静态用户会话服务（注册/登录/管理员/访客/会话）。 |
| `UserContext` | 当前认证用户上下文管理，提供记录读取入口。 |

### 4.5 audio

| 类 | 作用 |
|---|---|
| `AudioPlayer` | 基于 `miniaudio` 的音频加载、播放、暂停、跳转与停止封装。 |

### 4.6 platform

| 类 | 作用 |
|---|---|
| `AppDirs` | 检测系统/用户模式并初始化、提供 data/charts/config/locale 目录路径。 |
| `RuntimeConfig` | 全局运行时配置中心：默认值、按用户加载/保存、测试目录覆盖。 |
| `I18n` | 轻量国际化单例：加载并查询扁平 JSON 文案文件。 |
| `ThemeStore` | 从用户和系统主题目录加载主题预设 JSON 文件。 |
| `SettingsIO` | `RuntimeConfig` 与 `SettingsDraft` 的双向转换及按用户持久化。 |

### 4.7 scenes

| 类 / 类型 | 作用 |
|---|---|
| `ChartSearchMode` | 曲目搜索维度枚举（曲名 / 作曲 / 谱师）。 |
| `ChartListScene` | 曲目列表状态：刷新、排序、前缀搜索、过滤、失败信息缓存。 |
| `UserStatScene` | 登录会话封装（注册/登录/登出）+ 统计：加载成绩并计算 rating/potential。 |
| `AdminRecordScope` | 管理端记录范围枚举（仅验证 / 全量）。 |
| `AdminPlayerStats` | 管理端按玩家聚合统计数据结构。 |
| `AdminStatScene` | 管理统计场景编排，支持按用户/曲目查询。 |
| `GameplaySettlement` | 结算场景状态机：保证结果保存仅触发一次。 |
| `SettingsScene` | 设置场景状态管理（已提交/草稿、脏检查、保存/丢弃、主题导入导出）。 |

### 4.8 ui

| 类 / 类型 | 作用 |
|---|---|
| `UIScene` | 所有可导航屏幕的枚举。 |
| `GameplayRouteParams` | `ChartListUI` 在跳转到 `UIScene::Gameplay` 前填充的参数包。 |
| `SettlementRouteParams` | `GameplayUI` 在跳转到 `UIScene::GameplaySettlement` 前填充的参数包。 |
| `UIBus` | 顶层场景循环驱动器，持有待路由参数槽。 |
| `UIColors` | 终端颜色常量与调色板辅助工具。 |
| `Rgb` / `ThemePalette` | 供所有渲染组件使用的已解析颜色值类型。 |
| `ThemeAdapter` | 从预设文件解析 `RuntimeConfig::theme`，返回终端就绪的 `ThemePalette`。 |
| `MessageOverlay` | 共享消息弹窗队列（信息/警告/错误），叠加渲染在任意场景之上。 |
| `TransitionBackdrop` | 场景切换动画背景，在屏幕跳转时渲染。 |
| `StartMenuUI` | 开始/登录菜单屏幕的 FTXUI 渲染组件。 |
| `ChartListUI` | 曲目列表与搜索屏幕的 FTXUI 渲染组件。 |
| `GameplayUI` | 游玩屏幕的 FTXUI 渲染组件。 |
| `GameplaySettlementUI` | 游玩结算结果屏幕的 FTXUI 渲染组件。 |
| `SettingsUI` | 设置屏幕的 FTXUI 渲染组件。 |
| `UserStatUI` | 用户统计屏幕的 FTXUI 渲染组件。 |

### 4.9 utils

| 类 / 命名空间 | 作用 |
|---|---|
| `LiteDBUtils` | 轻量存储安全工具：密钥文件、哈希/慢哈希、AES、Hex 编解码。 |
| `PrefixTrie` | 前缀检索索引，支持增量游标优化的实时搜索。 |
| `JsonUtils` | 扁平 JSON 键值对象的解析/加载/序列化工具。 |
| `ErrorNotifier` | 统一错误/异常上报通道（可替换输出 sink）。 |
| `rating_utils` | 共享 rating 数学（单曲评分、准确度标准化），被曲目目录与统计场景共用。 |
| `string_utils` | 通用字符串谓词（纯数字判断等）。 |

## 5）典型依赖示例

- **曲目浏览**：`ChartListUI` 渲染 `ChartListScene` -> `ChartCatalog` + `PrefixTrie` + `Chart`。
- **游玩流程**：`GameplayUI` 驱动 `GameplaySession` -> (`GameplayChartParser`、`GameplayClock`、`GameplayStats`) -> `GameplayFinalResult`。
- **结果保存**：`GameplaySettlement` -> `GameplayRecordWriter` -> `UserContext` + `RecordStore`。
- **登录流程**：`UserStatScene` -> `UserSession` -> `UserStore`。
- **设置流程**：`SettingsScene` -> `SettingsIO` -> `RuntimeConfig`（并通过 `AppDirs` 按用户持久化）。

## 6）当前架构特征

- 目录以**游戏领域子系统**命名，而非技术角色，每个文件夹职责单一清晰。
- `scenes/` 层将渲染（`ui/`）与领域逻辑解耦：`*Scene` 持有状态并协调子系统，配对的 `*UI` 类是纯 FTXUI 渲染组件。
- 大多数领域类为全静态工具风格或轻量状态持有者，无继承体系。
- `data/*` 值类型对任何子系统均无依赖，可安全跨层共享。
- `utils/*` 不依赖任何领域层、场景层或 UI 层。


