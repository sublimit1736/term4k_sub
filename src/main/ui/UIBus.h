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

inline bool sceneUsesComponentLoop(const UIScene scene) {
    return scene == UIScene::StartMenu || scene == UIScene::ChartList ||
           scene == UIScene::Settings || scene == UIScene::UserStat;
}


class UIBus {
public:
    static int run();
};

} // namespace ui
