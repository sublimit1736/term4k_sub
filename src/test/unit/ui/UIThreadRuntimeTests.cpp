#include "catch_amalgamated.hpp"

#include "ui/UIThreadRuntime.h"

#include <chrono>
#include <thread>

TEST_CASE("UIThreadRuntime starts and stops independent UI threads", "[ui][UIThreadRuntime]") {
    ui::StartMenuUI startMenu;
    ui::ChartListUI chartList;
    ui::GameplayUI gameplay;

    ChartListInstance chartListInstance;
    GameplayInstance gameplayInstance;

    ui::UIThreadRuntime runtime;
    runtime.bindStartMenu(&startMenu);
    runtime.bindChartList(&chartList, &chartListInstance);
    runtime.bindGameplay(&gameplay, &gameplayInstance);

    REQUIRE(runtime.startStartMenu());
    REQUIRE(runtime.startChartList());
    REQUIRE(runtime.startGameplay());

    REQUIRE(runtime.isStartMenuRunning());
    REQUIRE(runtime.isChartListRunning());
    REQUIRE(runtime.isGameplayRunning());

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    runtime.stopAll();

    REQUIRE_FALSE(runtime.isStartMenuRunning());
    REQUIRE_FALSE(runtime.isChartListRunning());
    REQUIRE_FALSE(runtime.isGameplayRunning());
}

TEST_CASE("UIThreadRuntime refuses duplicate start on same UI slot", "[ui][UIThreadRuntime]") {
    ui::UserLoginUI loginUI;
    ui::UserSettingsUI settingsUI;
    UserLoginInstance loginInstance;
    SettingsInstance settingsInstance;

    ui::UIThreadRuntime runtime;
    runtime.bindUserLogin(&loginUI, &loginInstance);
    runtime.bindUserSettings(&settingsUI, &settingsInstance);

    REQUIRE(runtime.startUserLogin());
    REQUIRE_FALSE(runtime.startUserLogin());

    runtime.stopUserLogin();
    REQUIRE_FALSE(runtime.isUserLoginRunning());
}

TEST_CASE("UIThreadRuntime can switch scene and keep only target running", "[ui][UIThreadRuntime]") {
    ui::StartMenuUI startMenu;
    ui::GameplayUI gameplay;
    GameplayInstance gameplayInstance;

    ui::UIThreadRuntime runtime;
    runtime.bindStartMenu(&startMenu);
    runtime.bindGameplay(&gameplay, &gameplayInstance);

    REQUIRE(runtime.switchTo(ui::UIThreadRuntime::UIScene::StartMenu));
    REQUIRE(runtime.currentScene() == ui::UIThreadRuntime::UIScene::StartMenu);
    REQUIRE(runtime.isStartMenuRunning());
    REQUIRE_FALSE(runtime.isGameplayRunning());

    REQUIRE(runtime.switchTo(ui::UIThreadRuntime::UIScene::Gameplay));
    REQUIRE(runtime.currentScene() == ui::UIThreadRuntime::UIScene::Gameplay);
    REQUIRE_FALSE(runtime.isStartMenuRunning());
    REQUIRE(runtime.isGameplayRunning());

    REQUIRE(runtime.switchTo(ui::UIThreadRuntime::UIScene::None));
    REQUIRE(runtime.currentScene() == ui::UIThreadRuntime::UIScene::None);
    REQUIRE_FALSE(runtime.isStartMenuRunning());
    REQUIRE_FALSE(runtime.isGameplayRunning());
}


