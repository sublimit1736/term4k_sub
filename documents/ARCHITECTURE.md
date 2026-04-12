# term4k Architecture and Module Responsibilities

[Back to README](../README.md) | [中文版本](./ARCHITECTURE_zh_CN.md)

This document explains the major modules in `term4k`, their relationships, containment hierarchy, and the role of each class currently exposed under `src/main`.

> Scope note: this description is based on the current code layout and headers listed in `CMakeLists.txt` (`TERM4K_CORE_SOURCES` + public headers).

## 1) High-Level Layering

The project follows a layered style:

```text
Instance layer (instances/*)
    -> Service layer (services/*)
        -> DAO + Config + Utils + Entities (dao/*, config/*, utils/*, entities/*)
```

Typical runtime flow:

1. Instance orchestrates one business scene.
2. Services execute use cases and calculations.
3. DAOs read/write persistent data.
4. Entities carry structured data across layers.
5. Config/Utils provide shared runtime infrastructure.

## 2) Directory Containment and Relationships

```text
src/main/
  config/      global runtime path and setting sources
  dao/         persistent storage access
  entities/    data models and gameplay snapshots
  instances/   scene-level orchestration objects
  services/    business logic and domain services
  utils/       cross-cutting helper components
```

Cross-module relationship highlights:

- `instances/*` classes mainly compose `services/*` classes.
- `services/*` classes depend on `entities/*`, `dao/*`, `utils/*`, and `config/*`.
- `dao/*` uses `utils/LiteDBUtils` for key management, hashing, and crypto helpers.
- `config/AppDirs` and `config/RuntimeConfigs` provide global path/config state used by multiple services.

## 3) Major Module Responsibilities

- **Gameplay pipeline**: `GameplayInstance` + `GameplaySessionService` + `Gameplay*Service` + `Gameplay*Result` entities manage chart parsing, timing, judgement, and final settlement.
- **Chart catalog pipeline**: `ChartCatalogService` + `ChartListInstance` + `PrefixTrie` support chart discovery, sorting, search, and conflict checks.
- **Account/session pipeline**: `UserAccountsDAO`, `UserLoginService`, `AuthenticatedUserService`, and `UserLoginInstance` manage register/login/session state.
- **Record pipeline**: `GameplayRecordService` and `ProofedRecordsDAO` persist score results and verify chain integrity.
- **Settings/i18n pipeline**: `RuntimeConfigs`, `SettingsDraft`, `SettingsService`, `SettingsInstance`, and `I18nService` manage runtime preferences and localization.

## 4) Class-by-Class Responsibilities

### 4.1 config

| Class / Type | Responsibility |
|---|---|
| `AppDirs` | Detects runtime mode (system/user), initializes and exposes data/charts/config/locale directories. |
| `ChartEndTimingMode` | Enum for gameplay completion timing strategy (`AfterAudioEnd` or `AfterChartEnd`). |
| `RuntimeConfigs` | Global runtime settings hub: defaults, per-user load/save, and test-time config dir override. |

### 4.2 dao

| Class | Responsibility |
|---|---|
| `ProofedRecordsDAO` | Anti-tamper score record storage with hash-chain verification and filtered read APIs. |
| `UserAccountsDAO` | User account file DAO with salted password hash verification and UID lookup. |

### 4.3 entities

| Class / Type | Responsibility |
|---|---|
| `Chart` | Chart metadata model (ID, display info, BPM, key count, preview, base tempo) with serialization helpers. |
| `GameplayTapNote` | One tap-note model (lane + time). |
| `GameplayHoldNote` | One hold-note model (lane + head/tail time). |
| `GameplayChartData` | Parsed chart payload used by gameplay session (taps, holds, end time, note count, max score). |
| `GameplayJudgement` | Judgement enum for note settlement (`Perfect`, `Great`, `Miss`). |
| `GameplaySnapshot` | Real-time gameplay snapshot used during play loop and rendering. |
| `GameplayFinalResult` | Final aggregated gameplay result used for settlement and persistence. |
| `GameplayClockSnapshot` | Audio/chart clock state snapshot with source mode and finish state. |
| `SettingsDraft` | Editable settings aggregate for user-level preference workflows. |
| `Rating` | Historical score/rating record model with serialization support. |
| `User` | User identity model (UID, username, password). |

### 4.4 instances

| Class / Type | Responsibility |
|---|---|
| `ChartSearchMode` | Enum for chart search dimension (`DisplayName`, `Artist`, `Charter`). |
| `ChartListInstance` | Scene-level chart list state: refresh, sort, prefix search index, filter, detection failure cache. |
| `UserLoginInstance` | Scene-level wrapper around registration/login/logout and current user status. |
| `UserStatInstance` | User statistic scene model: loads records and calculates rating/potential. |
| `AdminRecordScope` | Admin query scope enum (`VerifiedOnly`, `AllRecords`). |
| `AdminPlayerStats` | Aggregated per-player record/stats container for admin views. |
| `AdminStatInstance` | Admin statistics orchestration over verified/all record scopes with user/chart queries. |
| `GameplayInstance` | Scene-level gameplay facade over `GameplaySessionService`. |
| `GameplaySettlementInstance` | Settlement scene guard: one-time result capture and save trigger. |
| `SettingsInstance` | Settings editing state manager (committed vs draft, save/discard, dirty checking). |

### 4.5 services

| Class / Type | Responsibility |
|---|---|
| `ChartEvent` | Parsed chart event unit (`type`, `data`) used in chart buffers. |
| `ChartIOService` | Static chart file parser and hex helper for `.t4k` event decoding. |
| `AudioService` | Song load/play/pause/resume/seek/stop wrapper built on `miniaudio`. |
| `I18nService` | Singleton translation store and flat JSON language file loader. |
| `AuthenticatedUserService` | Central authenticated-user state and record-loading bridge from login context. |
| `UserLoginService` | Static register/login/admin/guest/session service. |
| `GameplayChartService` | Converts chart files into `GameplayChartData`. |
| `GameplayStatsService` | Gameplay scoring/combo/accuracy accumulator and snapshot/final-result builder. |
| `GameplayClockService` | Manages audio/chart time synchronization and chart clock mode. |
| `GameplaySessionService` | Core gameplay state machine: input handling, note judgement, lane windows, settle-ready checks. |
| `GameplayRecordService` | Persists final gameplay result for current authenticated user. |
| `SettingsService` | Converts runtime config to/from `SettingsDraft` and persists per-user settings. |
| `ChartListSortKey` | Sort-key enum for chart list ordering. |
| `SortOrder` | Sort direction enum (`Ascending`, `Descending`). |
| `ChartDetectionIssue` | Chart folder scan failure reason enum. |
| `ChartPlayStats` | Per-chart play summary (play count, best score, best accuracy). |
| `ChartDetectionFailure` | Detailed chart detection failure payload for diagnostics/reporting. |
| `ChartCatalogEntry` | Catalog item with metadata paths and aggregated user stats. |
| `ChartRecordEntry` | Business-level record entry mapped to chart/user context. |
| `ChartRecordCollection` | Record map + display order bundle. |
| `PlayableNoteConflict` | Compliance check result for same-lane/same-time playable-note conflicts. |
| `ChartCatalogService` | Chart directory scanning, per-user catalog assembly, sorting, and compliance checks. |

### 4.6 utils

| Class | Responsibility |
|---|---|
| `LiteDBUtils` | Key file lifecycle, hash/slow-hash, AES helpers, and hex conversion for lightweight storage protection. |
| `PrefixTrie` | Prefix-search index with incremental cursor optimization for interactive filtering. |
| `JsonUtils` | Flat JSON key-value parse/load/stringify helper. |
| `ErrorNotifier` | Centralized error/exception reporting sink abstraction. |

## 5) Practical Dependency Examples

- **Chart browse**: `ChartListInstance` -> `ChartCatalogService` + `PrefixTrie` + `Chart`.
- **Gameplay**: `GameplayInstance` -> `GameplaySessionService` -> (`GameplayChartService`, `GameplayClockService`, `GameplayStatsService`) -> `GameplayFinalResult`.
- **Result save**: `GameplaySettlementInstance` -> `GameplayRecordService` -> `AuthenticatedUserService` + `ProofedRecordsDAO`.
- **Login**: `UserLoginInstance` -> `UserLoginService` -> `UserAccountsDAO`.
- **Settings**: `SettingsInstance` -> `SettingsService` -> `RuntimeConfigs` (+ per-user persistence).

## 6) Current Architecture Characteristics

- Layer boundaries are explicit and mostly one-way (Instance -> Service -> Data/Utility).
- Most business services are static utility style; instance objects act as scene state holders.
- Domain/gameplay and persistence modules are structured for headless embedding and extension.


