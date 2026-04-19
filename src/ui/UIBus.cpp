#include "ui/UIBus.h"

#include "platform/AppDirs.h"
#include "platform/RuntimeConfig.h"
#include "account/RecordStore.h"
#include "account/UserStore.h"
#include "platform/I18n.h"
#include "ui/ChartListUI.h"
#include "ui/GameplaySettlementUI.h"
#include "ui/GameplayUI.h"
#include "ui/SettingsUI.h"
#include "ui/StartMenuUI.h"
#include "ui/ThemeAdapter.h"
#include "ui/UserStatUI.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <cassert>
#include <functional>
#include <optional>

namespace ui {

GameplayRouteParams   UIBus::pendingGameplay;
SettlementRouteParams UIBus::pendingSettlement;

namespace {

void prepareCommon() {
    I18n::instance().ensureLocaleLoaded(RuntimeConfig::locale);
    (void)ThemeAdapter::resolveFromRuntime();
}

void prepareDataDirs() {
    AppDirs::init();
    RecordStore::setDataDir(AppDirs::dataDir());
    UserStore::setDataDir(AppDirs::dataDir());
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
    case UIScene::Gameplay:
        return GameplayUI::component(screen, UIBus::pendingGameplay, std::move(onRoute));
    case UIScene::GameplaySettlement:
        return GameplaySettlementUI::component(UIBus::pendingSettlement, std::move(onRoute));
    case UIScene::Exit:
        break;
    }

    assert(false && "buildSceneComponent called with unsupported scene");
    std::abort();
}

} // namespace

int UIBus::run() {
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    struct SceneHolder {
        ftxui::Component        active;
        std::optional<UIScene>  pending;
    };
    auto holder = std::make_shared<SceneHolder>();

    std::function<void(UIScene)> onRoute = [holder, &screen](UIScene next) {
        if (next == UIScene::Exit) {
            screen.ExitLoopClosure()();
            return;
        }
        if (!holder->pending) {
            holder->pending = next;
            screen.PostEvent(ftxui::Event::Custom);
        }
    };

    prepareCommon();
    holder->active = buildSceneComponent(UIScene::StartMenu, screen, onRoute);

    auto app = ftxui::CatchEvent(
        ftxui::Renderer([holder] {
            return holder->active ? holder->active->Render()
                                  : ftxui::text("") | ftxui::flex;
        }),
        [holder, &screen, &onRoute](ftxui::Event e) {
            if (holder->pending && e == ftxui::Event::Custom) {
                const UIScene next = *std::exchange(holder->pending, std::nullopt);
                prepareCommon();
                if (sceneNeedsDataDirs(next)) {
                    prepareDataDirs();
                }
                holder->active = buildSceneComponent(next, screen, onRoute);
                return true;
            }
            return holder->active ? holder->active->OnEvent(e) : false;
        });

    screen.Loop(app);
    return 0;
}

} // namespace ui
