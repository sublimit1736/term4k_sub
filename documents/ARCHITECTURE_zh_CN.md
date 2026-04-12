# term4k 架构与模块分工说明

[返回 README](../README.md) | [English Version](./ARCHITECTURE.md)

本文档说明 `term4k` 当前各大模块的职责分工、结构联系、包含关系，以及 `src/main` 下各类（class/type）的作用。

> 说明范围：依据当前代码目录与 `CMakeLists.txt` 中 `TERM4K_CORE_SOURCES` / 公共头文件清单整理。

## 1）总体分层

项目整体采用分层结构：

```text
Instance 场景编排层 (instances/*)
    -> Service 业务服务层 (services/*)
        -> DAO + Config + Utils + Entities (dao/*, config/*, utils/*, entities/*)
```

典型运行链路：

1. Instance 组织一个场景的状态与流程。
2. Service 执行业务逻辑与计算。
3. DAO 负责持久化读写。
4. Entities 作为跨层数据载体。
5. Config/Utils 提供全局配置与通用能力。

## 2）目录包含关系与结构联系

```text
src/main/
  config/      全局路径与运行配置来源
  dao/         持久化存储访问
  entities/    数据模型与游戏快照
  instances/   场景级编排对象
  services/    核心业务逻辑
  utils/       通用工具组件
```

模块关系重点：

- `instances/*` 主要组合调用 `services/*`。
- `services/*` 依赖 `entities/*`、`dao/*`、`utils/*`、`config/*`。
- `dao/*` 使用 `utils/LiteDBUtils` 提供密钥、哈希、加密能力。
- `config/AppDirs` 与 `config/RuntimeConfigs` 为多个服务提供路径与配置上下文。

## 3）大模块职责

- **游玩链路**：`GameplayInstance` + `GameplaySessionService` + `Gameplay*Service` + `Gameplay*Result` 负责谱面解析、时钟推进、判定计分与结算。
- **曲目目录链路**：`ChartCatalogService` + `ChartListInstance` + `PrefixTrie` 负责曲目发现、排序、搜索与合规检查。
- **账户会话链路**：`UserAccountsDAO`、`UserLoginService`、`AuthenticatedUserService`、`UserLoginInstance` 负责注册登录与会话身份状态。
- **成绩记录链路**：`GameplayRecordService` 与 `ProofedRecordsDAO` 负责成绩入库与链式校验。
- **设置与国际化链路**：`RuntimeConfigs`、`SettingsDraft`、`SettingsService`、`SettingsInstance`、`I18nService` 负责偏好设置与语言资源。

## 4）各类作用（逐模块）

### 4.1 config

| 类 / 类型 | 作用 |
|---|---|
| `AppDirs` | 检测系统/用户模式并初始化、提供 data/charts/config/locale 目录路径。 |
| `ChartEndTimingMode` | 结算时机枚举（`AfterAudioEnd` / `AfterChartEnd`）。 |
| `RuntimeConfigs` | 全局运行时配置中心：默认值、按用户加载/保存、测试目录覆盖。 |

### 4.2 dao

| 类 | 作用 |
|---|---|
| `ProofedRecordsDAO` | 防篡改成绩数据库访问层，维护哈希链并提供按条件查询。 |
| `UserAccountsDAO` | 用户账户文件访问层，负责加盐密码校验与 UID 查询。 |

### 4.3 entities

| 类 / 类型 | 作用 |
|---|---|
| `Chart` | 谱面元数据模型（ID、显示名、BPM、键数、预览段、基础节奏等），含序列化能力。 |
| `GameplayTapNote` | Tap 音符数据（轨道 + 时间）。 |
| `GameplayHoldNote` | Hold 音符数据（轨道 + 起止时间）。 |
| `GameplayChartData` | 游玩用谱面数据容器（tap/hold、结尾时刻、音符数、最大分）。 |
| `GameplayJudgement` | 判定枚举（`Perfect`、`Great`、`Miss`）。 |
| `GameplaySnapshot` | 游玩过程中的实时状态快照。 |
| `GameplayFinalResult` | 结算后的最终结果模型。 |
| `GameplayClockSnapshot` | 音频/谱面时钟快照（驱动模式 + 完成状态）。 |
| `SettingsDraft` | 可编辑设置草稿模型，用于设置页改动管理。 |
| `Rating` | 历史成绩记录模型，支持序列化。 |
| `User` | 用户模型（UID、用户名、密码）。 |

### 4.4 instances

| 类 / 类型 | 作用 |
|---|---|
| `ChartSearchMode` | 曲目搜索维度枚举（曲名/作曲/谱师）。 |
| `ChartListInstance` | 曲目列表场景状态：刷新、排序、前缀搜索、过滤、失败信息缓存。 |
| `UserLoginInstance` | 登录场景编排：注册、登录、登出、当前用户状态。 |
| `UserStatInstance` | 普通用户统计场景：加载记录并计算 rating/potential。 |
| `AdminRecordScope` | 管理端记录范围枚举（仅验证 / 全量）。 |
| `AdminPlayerStats` | 管理端按玩家聚合统计数据结构。 |
| `AdminStatInstance` | 管理统计场景编排，支持按用户/曲目查询。 |
| `GameplayInstance` | 游玩场景门面，封装 `GameplaySessionService`。 |
| `GameplaySettlementInstance` | 结算场景状态机，保证结果保存触发一次。 |
| `SettingsInstance` | 设置场景状态管理（已提交/草稿、脏检查、保存/丢弃）。 |

### 4.5 services

| 类 / 类型 | 作用 |
|---|---|
| `ChartEvent` | 谱面事件单元（`type` + `data`）。 |
| `ChartIOService` | `.t4k` 谱面文件静态解析与十六进制辅助转换。 |
| `AudioService` | 基于 `miniaudio` 的音频加载、播放、暂停、跳转与停止。 |
| `I18nService` | 轻量单例国际化服务，加载并查询扁平 JSON 文案。 |
| `AuthenticatedUserService` | 当前认证用户上下文管理，并提供记录读取入口。 |
| `UserLoginService` | 静态用户登录服务（注册/登录/管理员/访客/会话）。 |
| `GameplayChartService` | 将谱面文件转换为 `GameplayChartData`。 |
| `GameplayStatsService` | 判定计分、连击与准确率累计，生成快照与最终结果。 |
| `GameplayClockService` | 管理音频时钟与谱面时钟同步策略。 |
| `GameplaySessionService` | 核心游玩状态机：按键输入、判定结算、轨道窗口推进、结果就绪判断。 |
| `GameplayRecordService` | 将当前认证用户的结算结果写入记录。 |
| `SettingsService` | `RuntimeConfigs` 与 `SettingsDraft` 的双向转换及按用户保存。 |
| `ChartListSortKey` | 曲目排序字段枚举。 |
| `SortOrder` | 排序方向枚举。 |
| `ChartDetectionIssue` | 曲目目录扫描失败类型枚举。 |
| `ChartPlayStats` | 单曲游玩统计（次数、最高分、最佳准度）。 |
| `ChartDetectionFailure` | 目录扫描失败明细（路径、原因、文案）。 |
| `ChartCatalogEntry` | 曲目目录项（元数据、资源路径、统计数据）。 |
| `ChartRecordEntry` | 业务层成绩记录项（用户、曲目、分数、时间等）。 |
| `ChartRecordCollection` | 记录集合（Map + 展示顺序）。 |
| `PlayableNoteConflict` | 可演奏音符冲突检查结果（同轨同刻冲突）。 |
| `ChartCatalogService` | 曲目扫描、用户统计聚合、排序与合规检查服务。 |

### 4.6 utils

| 类 | 作用 |
|---|---|
| `LiteDBUtils` | 轻量存储安全工具：密钥文件、哈希/慢哈希、AES、Hex 编解码。 |
| `PrefixTrie` | 前缀检索索引，支持增量游标优化的实时搜索。 |
| `JsonUtils` | 扁平 JSON 键值对象的解析/加载/序列化工具。 |
| `ErrorNotifier` | 统一错误/异常上报通道（可替换输出 sink）。 |

## 5）典型依赖示例

- **曲目浏览**：`ChartListInstance` -> `ChartCatalogService` + `PrefixTrie` + `Chart`。
- **游玩流程**：`GameplayInstance` -> `GameplaySessionService` -> (`GameplayChartService`、`GameplayClockService`、`GameplayStatsService`) -> `GameplayFinalResult`。
- **结果保存**：`GameplaySettlementInstance` -> `GameplayRecordService` -> `AuthenticatedUserService` + `ProofedRecordsDAO`。
- **登录流程**：`UserLoginInstance` -> `UserLoginService` -> `UserAccountsDAO`。
- **设置流程**：`SettingsInstance` -> `SettingsService` -> `RuntimeConfigs`（并按用户持久化）。

## 6）当前架构特征

- 层级边界较明确，依赖方向基本单向（Instance -> Service -> Data/Utility）。
- 业务服务多为静态工具风格，`Instance` 负责场景状态承接。
- 领域逻辑与数据层已具备可扩展结构，可被 headless 程序或外部前端复用。


