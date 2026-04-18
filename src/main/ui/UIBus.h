#pragma once

namespace ui {

enum class UIScene {
    StartMenu,
    ChartList,
    Settings,
    UserStat,
    Exit,
};

inline bool sceneNeedsDataDirs(const UIScene scene) {
    return scene == UIScene::ChartList || scene == UIScene::UserStat;
}


class UIBus {
public:
    static int run();
};

} // namespace ui
