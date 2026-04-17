#include "ui/UIBus.h"

#include "config/AppDirs.h"
#include "config/RuntimeConfigs.h"
#include "dao/ProofedRecordsDAO.h"
#include "services/I18nService.h"
#include "ui/ChartListUI.h"
#include "ui/SettingsUI.h"
#include "ui/StartMenuUI.h"
#include "ui/ThemeAdapter.h"
#include "ui/UserStatUI.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <cassert>
#include <functional>

namespace ui {
namespace {

void prepareCommon() {
    I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
    (void)ThemeAdapter::resolveFromRuntime();
}

void prepareDataDirs() {
    AppDirs::init();
    ProofedRecordsDAO::setDataDir(AppDirs::dataDir());
}

ftxui::Component buildSceneComponent(const UIScene scene,
                                     ftxui::ScreenInteractive &screen,
                                     std::function<void(UIScene)> onRoute) {
    switch (scene) {
    case UIScene::StartMenu:
        return StartMenuUI::component(std::move(onRoute));
    case UIScene::ChartList:
        return ChartListUI::component(screen, std::move(onRoute));
    case UIScene::Settings:
        return SettingsUI::component(std::move(onRoute));
    case UIScene::UserStat:
        return UserStatUI::component(std::move(onRoute));
    case UIScene::Exit:
        break;
    }

    assert(false && "buildSceneComponent called with unsupported scene");
    std::abort();
}

UIScene runComponentScene(ftxui::ScreenInteractive &screen, const UIScene scene) {
    UIScene next = UIScene::StartMenu;
    auto component = buildSceneComponent(scene, screen, [&](const UIScene route) {
        next = route;
        screen.ExitLoopClosure()();
    });
    screen.Loop(component);
    return next;
}

UIScene runScene(ftxui::ScreenInteractive &screen, const UIScene scene) {

    if (!sceneUsesComponentLoop(scene)) {
        return UIScene::Exit;
    }

    prepareCommon();
    if (sceneNeedsDataDirs(scene)) {
        prepareDataDirs();
    }

    return runComponentScene(screen, scene);
}

} // namespace

int UIBus::run() {
    auto screen = ftxui::ScreenInteractive::Fullscreen();
    UIScene scene = UIScene::StartMenu;
    while (scene != UIScene::Exit) {
        scene = runScene(screen, scene);
    }

    return 0;
}

} // namespace ui

