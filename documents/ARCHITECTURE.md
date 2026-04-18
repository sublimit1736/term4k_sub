# term4k Architecture and Module Responsibilities

[Back to README](../README.md) | [中文版本](./ARCHITECTURE_zh_CN.md)

This document explains the major modules in `term4k`, their relationships, containment hierarchy, and the role of each class currently exposed under `src`.

> Scope note: this description is based on the current code layout and headers listed in `CMakeLists.txt` (`TERM4K_CORE_SOURCES` + public headers).

## 1) High-Level Layering

The project is organised around game-domain subsystems rather than technical tiers:

```text
UI layer         (ui/*)       — FTXUI rendering components and shared UI infrastructure
Scene layer      (scenes/*)   — per-screen state holders (data + orchestration)
Domain layer     (gameplay/*, chart/*, account/*, audio/*, platform/*)
Data layer       (data/*)     — plain value types shared across subsystems
Utility layer    (utils/*)    — cross-cutting helpers with no domain dependency
```

Typical runtime flow:

1. `UIBus` drives the top-level scene loop.
2. A `*UI` component renders the screen and delegates user input to its paired `*Scene`.
3. A scene coordinates domain subsystems (`GameplaySession`, `ChartCatalog`, `UserSession`, …).
4. Domain classes read/write persistent data via `account/*` and manipulate `data/*` value types.
5. `platform/*` provides global config, i18n, and app-directory resolution to any subsystem.
6. `utils/*` supplies generic helpers (JSON, crypto, trie, error reporting) used project-wide.

## 2) Directory Containment and Relationships

```text
src/
  data/       pure value types and gameplay snapshots (no subsystem dependency)
  gameplay/   game engine: session, clock, stats, chart parser, record writer
  chart/      chart catalog scanning and .t4k file parsing
  account/    user auth, session state, and score record storage
  audio/      audio playback wrapper (miniaudio)
  platform/   app dirs, runtime config, i18n, theme store, settings I/O
  scenes/     per-screen state holders (orchestrate domain subsystems)
  ui/         FTXUI rendering components + UI bus, colors, theme adapter
  utils/      cross-cutting helpers: JSON, crypto, trie, error reporting
```

Cross-module relationship highlights:

- `ui/*` renders data from `scenes/*`; each `*UI` class references its paired `*Scene`.
- `scenes/*` compose domain subsystems (`gameplay/`, `chart/`, `account/`, `platform/`).
- `gameplay/*` produces and consumes `data/*` value types.
- `account/*` and `chart/*` both depend on `data/*` for shared entity types.
- `platform/RuntimeConfig` and `platform/AppDirs` are accessed by most subsystems.
- `utils/*` has no dependency on any domain or scene layer.

## 3) Major Pipeline Responsibilities

- **Gameplay pipeline**: `GameplaySession` + `GameplayClock` + `GameplayStats` + `GameplayChartParser` manage chart parsing, timing, judgement, and final settlement.
- **Chart catalog pipeline**: `ChartCatalog` + `ChartListScene` + `PrefixTrie` support chart discovery, sorting, search, and compliance checks.
- **Account/session pipeline**: `UserStore`, `UserSession`, `UserContext`, and `UserStatScene` manage register/login/session state and score history.
- **Record pipeline**: `GameplayRecordWriter` and `RecordStore` persist score results and verify chain integrity.
- **Settings/i18n pipeline**: `RuntimeConfig`, `SettingsDraft`, `SettingsIO`, `SettingsScene`, and `I18n` manage runtime preferences and localization.
- **UI pipeline**: `UIBus` drives scene routing; `*UI` classes render scenes; `ThemeAdapter`, `UIColors`, `MessageOverlay`, and `TransitionBackdrop` provide shared UI infrastructure.

## 4) Class-by-Class Responsibilities

### 4.1 data

| Class / Type | Responsibility |
|---|---|
| `Chart` | Chart metadata model (ID, display info, BPM, key count, preview, base tempo) with serialization helpers. |
| `GameplayTapNote` | One tap-note value (lane + time). |
| `GameplayHoldNote` | One hold-note value (lane + head/tail time). |
| `GameplayChartData` | Parsed chart payload used by gameplay session (taps, holds, end time, note count, max score). |
| `GameplayJudgement` | Judgement enum for note settlement (`Perfect`, `Great`, `Miss`). |
| `GameplaySnapshot` | Real-time gameplay snapshot used during the play loop and for rendering. |
| `GameplayFinalResult` | Final aggregated gameplay result used for settlement and persistence. |
| `GameplayClockSnapshot` | Audio/chart clock state snapshot with source mode and finish state. |
| `SettingsDraft` | Editable settings aggregate for user-level preference workflows. |
| `Rating` | Historical score/rating record model with serialization support. |
| `User` | User identity model (UID, username, password). |

### 4.2 gameplay

| Class | Responsibility |
|---|---|
| `GameplayChartParser` | Converts a `.t4k` chart file into `GameplayChartData` for session use. |
| `GameplayClock` | Manages audio/chart time synchronization and chart clock mode. |
| `GameplayStats` | Gameplay scoring/combo/accuracy accumulator; builds `GameplaySnapshot` and `GameplayFinalResult`. |
| `GameplaySession` | Core gameplay state machine: input handling, note judgement, lane windows, settle-ready checks. |
| `GameplayRecordWriter` | Persists the final gameplay result for the current authenticated user. |

### 4.3 chart

| Class / Type | Responsibility |
|---|---|
| `ChartEvent` | Parsed chart event unit (`type`, `data`) used in chart buffers. |
| `ChartFileReader` | Static `.t4k` chart file parser and hex helper for event decoding. |
| `ChartListSortKey` | Sort-key enum for chart list ordering. |
| `SortOrder` | Sort direction enum (`Ascending`, `Descending`). |
| `ChartDetectionIssue` | Chart folder scan failure reason enum. |
| `ChartPlayStats` | Per-chart play summary (play count, best score, best accuracy, FC/AP/ULT flags). |
| `ChartDetectionFailure` | Detailed chart detection failure payload for diagnostics and reporting. |
| `ChartCatalogEntry` | Combined catalog entry with metadata paths and aggregated user stats. |
| `ChartRecordEntry` | Business-level record entry mapped to chart/user context. |
| `ChartRecordCollection` | Record map + display order bundle. |
| `ChartCatalog` | Chart directory scanning, per-user catalog assembly, sorting, and compliance checks. |

### 4.4 account

| Class | Responsibility |
|---|---|
| `RecordStore` | Anti-tamper score database (all-static); uses chained hashes to protect historical records. |
| `UserStore` | User account file DAO (all-static); salted password hash verification and UID lookup. |
| `UserSession` | Static register/login/admin/guest/session service. |
| `UserContext` | Central authenticated-user state and record-loading bridge from login context. |

### 4.5 audio

| Class | Responsibility |
|---|---|
| `AudioPlayer` | Song load/play/pause/resume/seek/stop wrapper built on `miniaudio`. |

### 4.6 platform

| Class | Responsibility |
|---|---|
| `AppDirs` | Detects runtime mode (system/user), initializes and exposes data/charts/config/locale directories. |
| `RuntimeConfig` | Global runtime settings hub: defaults, per-user load/save, and test-time config dir override. |
| `I18n` | Singleton translation store and flat JSON language file loader. |
| `ThemeStore` | Loads theme preset JSON files from user and system theme directories. |
| `SettingsIO` | Converts `RuntimeConfig` to/from `SettingsDraft` and persists per-user settings. |

### 4.7 scenes

| Class / Type | Responsibility |
|---|---|
| `ChartSearchMode` | Enum for chart search dimension (`DisplayName`, `Artist`, `Charter`). |
| `ChartListScene` | Chart list state: refresh, sort, prefix search index, filter, detection failure cache. |
| `UserStatScene` | User session wrapper (register/login/logout) + statistics: record loading, rating/potential. |
| `AdminRecordScope` | Admin query scope enum (`VerifiedOnly`, `AllRecords`). |
| `AdminPlayerStats` | Aggregated per-player record/stats container for admin views. |
| `AdminStatScene` | Admin statistics orchestration over verified/all record scopes with user/chart queries. |
| `GameplaySettlement` | Settlement scene guard: one-time result capture and save trigger. |
| `SettingsScene` | Settings editing state manager (committed vs draft, save/discard, dirty checking, theme import/export). |

### 4.8 ui

| Class / Type | Responsibility |
|---|---|
| `UIScene` | Enum of all navigable screens. |
| `GameplayRouteParams` | Parameter bundle populated by `ChartListUI` before routing to `UIScene::Gameplay`. |
| `SettlementRouteParams` | Parameter bundle populated by `GameplayUI` before routing to `UIScene::GameplaySettlement`. |
| `UIBus` | Top-level scene loop driver; owns the pending route parameter slots. |
| `UIColors` | Terminal color constants and palette helpers. |
| `Rgb` / `ThemePalette` | Resolved color palette value types used by all rendering components. |
| `ThemeAdapter` | Resolves `RuntimeConfig::theme` from preset files into a terminal-ready `ThemePalette`. |
| `MessageOverlay` | Shared message popup queue (info/warning/error) rendered on top of any scene. |
| `TransitionBackdrop` | Animated scene-transition backdrop rendered between screen changes. |
| `StartMenuUI` | FTXUI rendering component for the start/login menu screen. |
| `ChartListUI` | FTXUI rendering component for the chart list and search screen. |
| `GameplayUI` | FTXUI rendering component for the active gameplay screen. |
| `GameplaySettlementUI` | FTXUI rendering component for the post-gameplay result screen. |
| `SettingsUI` | FTXUI rendering component for the settings screen. |
| `UserStatUI` | FTXUI rendering component for the user statistics screen. |

### 4.9 utils

| Class / Namespace | Responsibility |
|---|---|
| `AppDirs` | *(see platform/AppDirs)* |
| `LiteDBUtils` | Key file lifecycle, hash/slow-hash, AES helpers, and hex conversion for lightweight storage protection. |
| `PrefixTrie` | Prefix-search index with incremental cursor optimization for interactive filtering. |
| `JsonUtils` | Flat JSON key-value parse/load/stringify helper. |
| `ErrorNotifier` | Centralized error/exception reporting sink abstraction. |
| `rating_utils` | Shared rating math (single-chart evaluation, accuracy normalization) used by catalog and stat scenes. |
| `string_utils` | Generic string predicates (digit-only check, etc.). |

## 5) Practical Dependency Examples

- **Chart browse**: `ChartListUI` renders `ChartListScene` -> `ChartCatalog` + `PrefixTrie` + `Chart`.
- **Gameplay**: `GameplayUI` drives `GameplaySession` -> (`GameplayChartParser`, `GameplayClock`, `GameplayStats`) -> `GameplayFinalResult`.
- **Result save**: `GameplaySettlement` -> `GameplayRecordWriter` -> `UserContext` + `RecordStore`.
- **Login**: `UserStatScene` -> `UserSession` -> `UserStore`.
- **Settings**: `SettingsScene` -> `SettingsIO` -> `RuntimeConfig` (+ per-user persistence via `AppDirs`).

## 6) Current Architecture Characteristics

- Directories are named after **game-domain subsystems**, not technical roles, so each folder has a single clear concern.
- The `scenes/` layer decouples rendering (`ui/`) from domain logic: a `*Scene` holds state and coordinates subsystems; its paired `*UI` class is a pure FTXUI rendering component.
- Most domain classes are static-utility style or thin state holders with no inheritance hierarchy.
- `data/*` value types have no dependency on any subsystem, making them safe to share across layers.
- `utils/*` has no dependency on any domain, scene, or UI layer.


