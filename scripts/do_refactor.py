#!/usr/bin/env python3
"""
Architecture refactoring script.
Moves files to new directories, renames classes, updates includes.
"""
import os
import re
import shutil

ROOT = "/home/runner/work/term4k_sub/term4k_sub"

# ---------------------------------------------------------------------------
# File moves: relative to ROOT (old_path -> new_path)
# ---------------------------------------------------------------------------
FILE_MOVES = {
    # entities → data
    "src/entities/Chart.h":                 "src/data/Chart.h",
    "src/entities/Chart.cpp":               "src/data/Chart.cpp",
    "src/entities/GameplayChartData.h":     "src/data/GameplayChartData.h",
    "src/entities/GameplayChartData.cpp":   "src/data/GameplayChartData.cpp",
    "src/entities/GameplayResult.h":        "src/data/GameplayResult.h",
    "src/entities/GameplayResult.cpp":      "src/data/GameplayResult.cpp",
    "src/entities/Rating.h":               "src/data/Rating.h",
    "src/entities/Rating.cpp":             "src/data/Rating.cpp",
    "src/entities/SettingsDraft.h":        "src/data/SettingsDraft.h",
    "src/entities/SettingsDraft.cpp":      "src/data/SettingsDraft.cpp",
    "src/entities/User.h":                 "src/data/User.h",
    "src/entities/User.cpp":               "src/data/User.cpp",
    # dao → account
    "src/dao/UserAccountsDAO.h":           "src/account/UserStore.h",
    "src/dao/UserAccountsDAO.cpp":         "src/account/UserStore.cpp",
    "src/dao/ProofedRecordsDAO.h":         "src/account/RecordStore.h",
    "src/dao/ProofedRecordsDAO.cpp":       "src/account/RecordStore.cpp",
    # services user account → account
    "src/services/UserLoginService.h":          "src/account/UserSession.h",
    "src/services/UserLoginService.cpp":        "src/account/UserSession.cpp",
    "src/services/AuthenticatedUserService.h":  "src/account/UserContext.h",
    "src/services/AuthenticatedUserService.cpp":"src/account/UserContext.cpp",
    # services gameplay → gameplay
    "src/services/GameplaySessionService.h":    "src/gameplay/GameplaySession.h",
    "src/services/GameplaySessionService.cpp":  "src/gameplay/GameplaySession.cpp",
    "src/services/GameplayClockService.h":      "src/gameplay/GameplayClock.h",
    "src/services/GameplayClockService.cpp":    "src/gameplay/GameplayClock.cpp",
    "src/services/GameplayStatsService.h":      "src/gameplay/GameplayStats.h",
    "src/services/GameplayStatsService.cpp":    "src/gameplay/GameplayStats.cpp",
    "src/services/GameplayChartService.h":      "src/gameplay/GameplayChartParser.h",
    "src/services/GameplayChartService.cpp":    "src/gameplay/GameplayChartParser.cpp",
    "src/services/GameplayRecordService.h":     "src/gameplay/GameplayRecordWriter.h",
    "src/services/GameplayRecordService.cpp":   "src/gameplay/GameplayRecordWriter.cpp",
    # services chart → chart
    "src/services/ChartIOService.h":            "src/chart/ChartFileReader.h",
    "src/services/ChartIOService.cpp":          "src/chart/ChartFileReader.cpp",
    "src/services/ChartCatalogService.h":       "src/chart/ChartCatalog.h",
    "src/services/ChartCatalogService.cpp":     "src/chart/ChartCatalog.cpp",
    # services audio → audio
    "src/services/AudioService.h":             "src/audio/AudioPlayer.h",
    "src/services/AudioService.cpp":           "src/audio/AudioPlayer.cpp",
    # services platform → platform
    "src/services/I18nService.h":              "src/platform/I18n.h",
    "src/services/I18nService.cpp":            "src/platform/I18n.cpp",
    "src/services/ThemePresetService.h":       "src/platform/ThemeStore.h",
    "src/services/ThemePresetService.cpp":     "src/platform/ThemeStore.cpp",
    "src/services/SettingsService.h":          "src/platform/SettingsIO.h",
    "src/services/SettingsService.cpp":        "src/platform/SettingsIO.cpp",
    # utils platform → platform
    "src/utils/AppDirs.h":                     "src/platform/AppDirs.h",
    "src/utils/AppDirs.cpp":                   "src/platform/AppDirs.cpp",
    "src/utils/RuntimeConfigs.h":              "src/platform/RuntimeConfig.h",
    "src/utils/RuntimeConfigs.cpp":            "src/platform/RuntimeConfig.cpp",
    # models → scenes
    "src/models/ChartListInstance.h":           "src/scenes/ChartListScene.h",
    "src/models/ChartListInstance.cpp":         "src/scenes/ChartListScene.cpp",
    "src/models/UserStatInstance.h":            "src/scenes/UserStatScene.h",
    "src/models/UserStatInstance.cpp":          "src/scenes/UserStatScene.cpp",
    "src/models/AdminStatInstance.h":           "src/scenes/AdminStatScene.h",
    "src/models/AdminStatInstance.cpp":         "src/scenes/AdminStatScene.cpp",
    "src/models/SettingsInstance.h":            "src/scenes/SettingsScene.h",
    "src/models/SettingsInstance.cpp":          "src/scenes/SettingsScene.cpp",
    "src/models/GameplaySettlementInstance.h":  "src/scenes/GameplaySettlement.h",
    "src/models/GameplaySettlementInstance.cpp":"src/scenes/GameplaySettlement.cpp",
    # test files
    "tests/unit/models/AdminStatInstanceTests.cpp":       "tests/unit/scenes/AdminStatSceneTests.cpp",
    "tests/unit/models/ChartListInstanceTests.cpp":       "tests/unit/scenes/ChartListSceneTests.cpp",
    "tests/unit/models/GameplaySessionServiceTests.cpp":  "tests/unit/gameplay/GameplaySessionTests.cpp",
    "tests/unit/models/GameplaySettlementInstanceTests.cpp": "tests/unit/scenes/GameplaySettlementTests.cpp",
    "tests/unit/models/SettingsInstanceTests.cpp":        "tests/unit/scenes/SettingsSceneTests.cpp",
    "tests/unit/models/UserStatInstanceTests.cpp":        "tests/unit/scenes/UserStatSceneTests.cpp",
    "tests/unit/services/AuthenticatedUserServiceTests.cpp": "tests/unit/account/UserContextTests.cpp",
    "tests/unit/services/ChartCatalogServiceTests.cpp":   "tests/unit/chart/ChartCatalogTests.cpp",
    "tests/unit/services/ChartIOTests.cpp":               "tests/unit/chart/ChartFileReaderTests.cpp",
    "tests/unit/services/GameplayRecordServiceTests.cpp": "tests/unit/gameplay/GameplayRecordWriterTests.cpp",
    "tests/unit/services/I18nTests.cpp":                  "tests/unit/platform/I18nTests.cpp",
    "tests/unit/services/SongPlayerTests.cpp":            "tests/unit/audio/AudioPlayerTests.cpp",
    "tests/unit/services/ThemePresetServiceTests.cpp":    "tests/unit/platform/ThemeStoreTests.cpp",
    "tests/unit/services/UserServiceTests.cpp":           "tests/unit/account/UserSessionTests.cpp",
    "tests/unit/utils/AppDirsTests.cpp":                  "tests/unit/platform/AppDirsTests.cpp",
    "tests/unit/utils/RuntimeConfigsTests.cpp":           "tests/unit/platform/RuntimeConfigTests.cpp",
}

# ---------------------------------------------------------------------------
# Global full-path include substitutions (applied to ALL files)
# Applied as exact string replacement within #include "..." directives.
# ---------------------------------------------------------------------------
INCLUDE_SUBS = [
    # entities → data
    ('"entities/Chart.h"',              '"data/Chart.h"'),
    ('"entities/GameplayChartData.h"',  '"data/GameplayChartData.h"'),
    ('"entities/GameplayResult.h"',     '"data/GameplayResult.h"'),
    ('"entities/Rating.h"',             '"data/Rating.h"'),
    ('"entities/SettingsDraft.h"',      '"data/SettingsDraft.h"'),
    ('"entities/User.h"',               '"data/User.h"'),
    # dao → account
    ('"dao/UserAccountsDAO.h"',         '"account/UserStore.h"'),
    ('"dao/ProofedRecordsDAO.h"',       '"account/RecordStore.h"'),
    # services → account
    ('"services/UserLoginService.h"',        '"account/UserSession.h"'),
    ('"services/AuthenticatedUserService.h"','"account/UserContext.h"'),
    # services → gameplay
    ('"services/GameplaySessionService.h"',  '"gameplay/GameplaySession.h"'),
    ('"services/GameplayClockService.h"',    '"gameplay/GameplayClock.h"'),
    ('"services/GameplayStatsService.h"',    '"gameplay/GameplayStats.h"'),
    ('"services/GameplayChartService.h"',    '"gameplay/GameplayChartParser.h"'),
    ('"services/GameplayRecordService.h"',   '"gameplay/GameplayRecordWriter.h"'),
    # services → chart
    ('"services/ChartIOService.h"',          '"chart/ChartFileReader.h"'),
    ('"services/ChartCatalogService.h"',     '"chart/ChartCatalog.h"'),
    # services → audio
    ('"services/AudioService.h"',            '"audio/AudioPlayer.h"'),
    # services → platform
    ('"services/I18nService.h"',             '"platform/I18n.h"'),
    ('"services/ThemePresetService.h"',      '"platform/ThemeStore.h"'),
    ('"services/SettingsService.h"',         '"platform/SettingsIO.h"'),
    # utils → platform
    ('"utils/AppDirs.h"',                    '"platform/AppDirs.h"'),
    ('"utils/RuntimeConfigs.h"',             '"platform/RuntimeConfig.h"'),
    # models → scenes
    ('"models/ChartListInstance.h"',         '"scenes/ChartListScene.h"'),
    ('"models/UserStatInstance.h"',          '"scenes/UserStatScene.h"'),
    ('"models/AdminStatInstance.h"',         '"scenes/AdminStatScene.h"'),
    ('"models/SettingsInstance.h"',          '"scenes/SettingsScene.h"'),
    ('"models/GameplaySettlementInstance.h"','"scenes/GameplaySettlement.h"'),
]

# ---------------------------------------------------------------------------
# Per-file bare include fixes (applied AFTER global subs, keyed by old src path)
# These are bare (same-dir) includes that cross directories after the move.
# ---------------------------------------------------------------------------
PER_FILE_BARE_SUBS = {
    # ChartCatalogService.cpp → chart/ChartCatalog.cpp
    "src/services/ChartCatalogService.cpp": [
        ('"ChartCatalogService.h"',  '"ChartCatalog.h"'),
        ('"ChartIOService.h"',       '"ChartFileReader.h"'),
        ('"I18nService.h"',          '"platform/I18n.h"'),
    ],
    # AuthenticatedUserService.h → account/UserContext.h
    "src/services/AuthenticatedUserService.h": [
        ('"ChartCatalogService.h"',  '"chart/ChartCatalog.h"'),
    ],
    # AuthenticatedUserService.cpp → account/UserContext.cpp
    "src/services/AuthenticatedUserService.cpp": [
        ('"AuthenticatedUserService.h"', '"UserContext.h"'),
        ('"ChartCatalogService.h"',      '"chart/ChartCatalog.h"'),
        ('"UserLoginService.h"',         '"UserSession.h"'),
    ],
    # UserLoginService.cpp → account/UserSession.cpp
    "src/services/UserLoginService.cpp": [
        ('"UserLoginService.h"', '"UserSession.h"'),
        ('"I18nService.h"',      '"platform/I18n.h"'),
    ],
    # AudioService.cpp → audio/AudioPlayer.cpp
    "src/services/AudioService.cpp": [
        ('"AudioService.h"', '"AudioPlayer.h"'),
        ('"I18nService.h"',  '"platform/I18n.h"'),
    ],
    # ChartIOService.cpp → chart/ChartFileReader.cpp
    "src/services/ChartIOService.cpp": [
        ('"ChartIOService.h"', '"ChartFileReader.h"'),
        ('"I18nService.h"',    '"platform/I18n.h"'),
    ],
    # I18nService.cpp → platform/I18n.cpp (same dir, just self-rename)
    "src/services/I18nService.cpp": [
        ('"I18nService.h"', '"I18n.h"'),
    ],
    # ThemePresetService.cpp → platform/ThemeStore.cpp
    "src/services/ThemePresetService.cpp": [
        ('"ThemePresetService.h"', '"ThemeStore.h"'),
    ],
    # SettingsService.cpp → platform/SettingsIO.cpp
    "src/services/SettingsService.cpp": [
        ('"SettingsService.h"', '"SettingsIO.h"'),
    ],
    # GameplaySessionService.cpp → gameplay/GameplaySession.cpp
    "src/services/GameplaySessionService.cpp": [
        ('"GameplaySessionService.h"', '"GameplaySession.h"'),
    ],
    # GameplayClockService.cpp → gameplay/GameplayClock.cpp
    "src/services/GameplayClockService.cpp": [
        ('"GameplayClockService.h"', '"GameplayClock.h"'),
    ],
    # GameplayStatsService.cpp → gameplay/GameplayStats.cpp
    "src/services/GameplayStatsService.cpp": [
        ('"GameplayStatsService.h"', '"GameplayStats.h"'),
    ],
    # GameplayChartService.cpp → gameplay/GameplayChartParser.cpp
    "src/services/GameplayChartService.cpp": [
        ('"GameplayChartService.h"', '"GameplayChartParser.h"'),
    ],
    # GameplayRecordService.cpp → gameplay/GameplayRecordWriter.cpp
    "src/services/GameplayRecordService.cpp": [
        ('"GameplayRecordService.h"', '"GameplayRecordWriter.h"'),
    ],
    # ProofedRecordsDAO.cpp → account/RecordStore.cpp
    "src/dao/ProofedRecordsDAO.cpp": [
        ('"ProofedRecordsDAO.h"', '"RecordStore.h"'),
    ],
    # UserAccountsDAO.cpp → account/UserStore.cpp
    "src/dao/UserAccountsDAO.cpp": [
        ('"UserAccountsDAO.h"', '"UserStore.h"'),
    ],
    # RuntimeConfigs.cpp → platform/RuntimeConfig.cpp
    "src/utils/RuntimeConfigs.cpp": [
        ('"RuntimeConfigs.h"', '"RuntimeConfig.h"'),
        # "AppDirs.h" stays bare (both end up in platform/ — same dir)
    ],
    # AppDirs.cpp → platform/AppDirs.cpp (self-include stays bare — same dir)
    # models → scenes (bare self-includes)
    "src/models/ChartListInstance.cpp": [
        ('"ChartListInstance.h"', '"ChartListScene.h"'),
    ],
    "src/models/UserStatInstance.cpp": [
        ('"UserStatInstance.h"', '"UserStatScene.h"'),
    ],
    "src/models/AdminStatInstance.cpp": [
        ('"AdminStatInstance.h"', '"AdminStatScene.h"'),
    ],
    "src/models/SettingsInstance.cpp": [
        ('"SettingsInstance.h"', '"SettingsScene.h"'),
    ],
    "src/models/GameplaySettlementInstance.cpp": [
        ('"GameplaySettlementInstance.h"', '"GameplaySettlement.h"'),
    ],
}

# ---------------------------------------------------------------------------
# Class name renames (whole-word, applied globally to all files)
# Order matters: longer/more-specific names first to avoid partial matches.
# ---------------------------------------------------------------------------
CLASS_RENAMES = [
    ("GameplaySettlementInstance",  "GameplaySettlement"),
    ("AuthenticatedUserService",    "UserContext"),
    ("GameplaySessionService",      "GameplaySession"),
    ("GameplayRecordService",       "GameplayRecordWriter"),
    ("GameplayChartService",        "GameplayChartParser"),
    ("GameplayClockService",        "GameplayClock"),
    ("GameplayStatsService",        "GameplayStats"),
    ("ChartCatalogService",         "ChartCatalog"),
    ("ThemePresetService",          "ThemeStore"),
    ("UserAccountsDAO",             "UserStore"),
    ("ProofedRecordsDAO",           "RecordStore"),
    ("UserLoginService",            "UserSession"),
    ("ChartListInstance",           "ChartListScene"),
    ("AdminStatInstance",           "AdminStatScene"),
    ("UserStatInstance",            "UserStatScene"),
    ("SettingsInstance",            "SettingsScene"),
    ("ChartIOService",              "ChartFileReader"),
    ("SettingsService",             "SettingsIO"),
    ("AudioService",                "AudioPlayer"),
    ("I18nService",                 "I18n"),
    ("RuntimeConfigs",              "RuntimeConfig"),
]


def apply_include_subs(content, subs):
    for old, new in subs:
        content = content.replace(old, new)
    return content


def apply_class_renames(content):
    for old_name, new_name in CLASS_RENAMES:
        content = re.sub(r'\b' + re.escape(old_name) + r'\b', new_name, content)
    return content


def collect_all_files():
    """Collect all .h/.cpp files under src/ and tests/."""
    files = []
    for dirpath in [os.path.join(ROOT, "src"), os.path.join(ROOT, "tests")]:
        for dirroot, _, filenames in os.walk(dirpath):
            for fn in filenames:
                if fn.endswith((".h", ".cpp")):
                    full = os.path.join(dirroot, fn)
                    rel = os.path.relpath(full, ROOT)
                    files.append(rel)
    return files


def main():
    all_files = collect_all_files()
    print(f"Found {len(all_files)} source files to process")

    # Build set of old paths that will be moved (to avoid double-writing)
    moved_old_paths = set(FILE_MOVES.keys())

    # Create target directories
    target_dirs = set()
    for new_rel in FILE_MOVES.values():
        target_dirs.add(os.path.dirname(os.path.join(ROOT, new_rel)))
    for d in sorted(target_dirs):
        os.makedirs(d, exist_ok=True)
        print(f"  mkdir {d}")

    # Process files that are being MOVED (write to new location, then delete old)
    for old_rel, new_rel in FILE_MOVES.items():
        old_path = os.path.join(ROOT, old_rel)
        new_path = os.path.join(ROOT, new_rel)
        if not os.path.exists(old_path):
            print(f"  SKIP (not found): {old_rel}")
            continue

        with open(old_path, "r", encoding="utf-8") as f:
            content = f.read()

        # 1. Global full-path include substitutions
        content = apply_include_subs(content, INCLUDE_SUBS)

        # 2. Per-file bare include substitutions (based on OLD path key)
        if old_rel in PER_FILE_BARE_SUBS:
            content = apply_include_subs(content, PER_FILE_BARE_SUBS[old_rel])

        # 3. Class name renames
        content = apply_class_renames(content)

        with open(new_path, "w", encoding="utf-8") as f:
            f.write(content)
        print(f"  MOVED  {old_rel} -> {new_rel}")

    # Process files that STAY in place (ui/, remaining utils/, main.cpp)
    for rel in all_files:
        if rel in moved_old_paths:
            continue  # already handled above
        full_path = os.path.join(ROOT, rel)

        with open(full_path, "r", encoding="utf-8") as f:
            content = f.read()

        new_content = apply_include_subs(content, INCLUDE_SUBS)
        new_content = apply_class_renames(new_content)

        if new_content != content:
            with open(full_path, "w", encoding="utf-8") as f:
                f.write(new_content)
            print(f"  UPDATED {rel}")

    # Delete old source files (after all new files are written)
    for old_rel in FILE_MOVES.keys():
        old_path = os.path.join(ROOT, old_rel)
        if os.path.exists(old_path):
            os.remove(old_path)

    # Remove now-empty old directories
    old_dirs_to_remove = [
        os.path.join(ROOT, "src/entities"),
        os.path.join(ROOT, "src/dao"),
        os.path.join(ROOT, "src/services"),
        os.path.join(ROOT, "src/models"),
        os.path.join(ROOT, "tests/unit/models"),
        os.path.join(ROOT, "tests/unit/services"),
    ]
    for d in old_dirs_to_remove:
        if os.path.isdir(d) and not os.listdir(d):
            os.rmdir(d)
            print(f"  RMDIR  {d}")
        elif os.path.isdir(d):
            remaining = os.listdir(d)
            print(f"  KEPT   {d} (non-empty: {remaining})")

    print("\nRefactoring complete!")


if __name__ == "__main__":
    main()
