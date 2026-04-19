#pragma once

#include <cstdint>
#include <string>

namespace ui {

enum class UIScene {
    StartMenu,
    ChartList,
    Settings,
    UserStat,
    Gameplay,
    GameplaySettlement,
    Exit,
};

inline bool sceneNeedsDataDirs(const UIScene scene) {
    return scene == UIScene::ChartList || scene == UIScene::UserStat;
}

// Parameters populated by ChartListUI before routing to UIScene::Gameplay.
struct GameplayRouteParams {
    std::string chartFilePath;
    std::string musicFilePath;
    std::string chartID;
    std::string songName;
    uint16_t    keyCount    = 0;
    float       difficulty  = 0.0f;
    float       personalBestAccuracy = 0.0f;  // 0.0 if no record
};

// Parameters populated by GameplayUI before routing to UIScene::GameplaySettlement.
struct SettlementRouteParams {
    uint32_t    perfectCount    = 0;
    uint32_t    greatCount      = 0;
    uint32_t    missCount       = 0;
    uint32_t    earlyCount      = 0;
    uint32_t    lateCount       = 0;
    uint64_t    score           = 0;
    double      accuracy        = 0.0;
    uint32_t    maxCombo        = 0;
    uint32_t    chartNoteCount  = 0;
    uint64_t    chartMaxScore   = 0;
    bool        saveSucceeded   = false;
    std::string songName;
    std::string chartID;
    float       difficulty      = 0.0f;
};

class UIBus {
public:
    static int run();

    // Pending routing context — set before calling onRoute().
    static GameplayRouteParams  pendingGameplay;
    static SettlementRouteParams pendingSettlement;
};

} // namespace ui
