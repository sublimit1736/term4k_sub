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

} // namespace

int UIBus::run() {
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    // Shared state so the thin wrapper can always delegate to the live scene.
    struct SceneHolder {
        ftxui::Component active;
        UIScene          pending    = UIScene::Exit;
        bool             hasPending = false;
    };
    auto holder = std::make_shared<SceneHolder>();

    // onRoute is invoked by scenes from within OnEvent (on the FTXUI event
    // thread).  We defer the actual swap via PostEvent so the current component
    // is never destroyed while it is still on the call stack.
    std::function<void(UIScene)> onRoute = [holder, &screen](UIScene next) {
        if (next == UIScene::Exit) {
            screen.ExitLoopClosure()();
            return;
        }
        if (!holder->hasPending) {
            holder->pending    = next;
            holder->hasPending = true;
            screen.PostEvent(ftxui::Event::Custom);
        }
    };

    // Build the initial scene.
    prepareCommon();
    holder->active = buildSceneComponent(UIScene::StartMenu, screen, onRoute);

    // A thin persistent wrapper — always delegates render/events to the
    // currently live scene without ever restarting the Loop.
    auto root = ftxui::Renderer([holder] {
        return holder->active ? holder->active->Render()
                              : ftxui::text("") | ftxui::flex;
    });

    auto app = ftxui::CatchEvent(root, [holder, &screen, &onRoute](ftxui::Event e) {
        // On the first Custom event after a route change, perform the deferred
        // scene swap.  This runs on the FTXUI event thread, safely after the
        // previous component's OnEvent has already returned.
        if (holder->hasPending && e == ftxui::Event::Custom) {
            holder->hasPending = false;
            const UIScene next = holder->pending;
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
