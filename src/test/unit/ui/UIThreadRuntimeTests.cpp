#include "catch_amalgamated.hpp"

#include "TestSupport.h"
#include "dao/UserAccountsDAO.h"
#include "services/AuthenticatedUserService.h"
#include "services/ThemePresetService.h"
#include "ui/UIThreadRuntime.h"
#include "utils/LiteDBUtils.h"

#include <chrono>
#include <fstream>
#include <thread>

using namespace test_support;

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
    REQUIRE(runtime.isUserLoginRunning());
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

TEST_CASE("UserLoginUI delegates registration and login helpers", "[ui][UserLoginUI]") {
    TempDir temp("term4k_user_login_ui_inline");
    UserAccountsDAO::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    UserLoginInstance loginInstance;
    ui::UserLoginUI loginUI(&loginInstance);

    std::string error;
    REQUIRE(loginUI.registerUser("ui_login", "pw", &error));
    REQUIRE(error.empty());
    REQUIRE(loginUI.login("ui_login", "pw", &error));
    REQUIRE(error.empty());
    REQUIRE(loginUI.isLoggedIn());
    REQUIRE(loginUI.currentUser().has_value());

    loginUI.logout();
    REQUIRE_FALSE(loginUI.isLoggedIn());

    AuthenticatedUserService::logout();
    UserAccountsDAO::setDataDir(".");
}

TEST_CASE("UserLoginUI reports error when helpers are called without a bound instance", "[ui][UserLoginUI]") {
    ui::UserLoginUI loginUI;
    std::string error;

    REQUIRE_FALSE(loginUI.registerUser("alice", "pw", &error));
    REQUIRE_FALSE(error.empty());
    REQUIRE_FALSE(loginUI.login("alice", "pw", &error));
    REQUIRE_FALSE(error.empty());
    REQUIRE_FALSE(loginUI.isLoggedIn());
}

TEST_CASE("UserLoginUI submit API reports errors and exposes one-shot success signal", "[ui][UserLoginUI]") {
    TempDir temp("term4k_user_login_ui_submit");
    UserAccountsDAO::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    UserLoginInstance loginInstance;
    ui::UserLoginUI loginUI(&loginInstance);

    std::string error;
    REQUIRE_FALSE(loginUI.submit(&error));
    REQUIRE_FALSE(error.empty());
    REQUIRE_FALSE(loginUI.consumeSubmitSucceeded());

    loginUI.setSubmitModeLogin(false);
    loginUI.setUsernameInput("submit_user");
    loginUI.setPasswordInput("pw");
    REQUIRE(loginUI.submit(&error));
    REQUIRE(error.empty());
    REQUIRE(loginUI.consumeSubmitSucceeded());
    REQUIRE_FALSE(loginUI.consumeSubmitSucceeded());

    loginUI.logout();
    loginUI.setSubmitModeLogin(true);
    loginUI.setUsernameInput("submit_user");
    loginUI.setPasswordInput("pw");
    REQUIRE(loginUI.submit(&error));
    REQUIRE(error.empty());
    REQUIRE(loginUI.consumeSubmitSucceeded());

    AuthenticatedUserService::logout();
    UserAccountsDAO::setDataDir(".");
}

TEST_CASE("UIThreadRuntime auto-switches from UserLogin to StartMenu after successful submit", "[ui][UIThreadRuntime]") {
    TempDir temp("term4k_runtime_login_auto_switch");
    UserAccountsDAO::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    ui::StartMenuUI startMenu;
    UserLoginInstance loginInstance;
    ui::UserLoginUI loginUI(&loginInstance);

    std::string error;
    REQUIRE(loginUI.registerUser("runtime_user", "pw", &error));
    REQUIRE(error.empty());

    ui::UIThreadRuntime runtime;
    REQUIRE(runtime.loginSuccessTargetScene() == ui::UIThreadRuntime::UIScene::StartMenu);
    runtime.bindStartMenu(&startMenu);
    runtime.bindUserLogin(&loginUI, &loginInstance);

    REQUIRE(runtime.switchTo(ui::UIThreadRuntime::UIScene::UserLogin));
    REQUIRE(runtime.currentScene() == ui::UIThreadRuntime::UIScene::UserLogin);
    REQUIRE(runtime.isUserLoginRunning());

    loginUI.setSubmitModeLogin(true);
    loginUI.setUsernameInput("runtime_user");
    loginUI.setPasswordInput("pw");
    REQUIRE(loginUI.submit(&error));
    REQUIRE(error.empty());

    bool switched = false;
    for (int i = 0; i < 50; ++i) {
        if (runtime.currentScene() == ui::UIThreadRuntime::UIScene::StartMenu && runtime.isStartMenuRunning()) {
            switched = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    REQUIRE(switched);

    runtime.stopAll();
    AuthenticatedUserService::logout();
    UserAccountsDAO::setDataDir(".");
}

TEST_CASE("UIThreadRuntime login success target scene is configurable", "[ui][UIThreadRuntime]") {
    TempDir temp("term4k_runtime_login_auto_switch_target");
    UserAccountsDAO::setDataDir(temp.path().string());
    LiteDBUtils::setKeyFile((temp.path() / "key.bin").string());
    REQUIRE(LiteDBUtils::ensureKey());

    ui::StartMenuUI startMenu;
    ui::ChartListUI chartList;
    ChartListInstance chartListInstance;
    UserLoginInstance loginInstance;
    ui::UserLoginUI loginUI(&loginInstance);

    std::string error;
    REQUIRE(loginUI.registerUser("runtime_target_user", "pw", &error));
    REQUIRE(error.empty());

    ui::UIThreadRuntime runtime;
    runtime.bindStartMenu(&startMenu);
    runtime.bindChartList(&chartList, &chartListInstance);
    runtime.bindUserLogin(&loginUI, &loginInstance);

    runtime.setLoginSuccessTargetScene(ui::UIThreadRuntime::UIScene::ChartList);
    REQUIRE(runtime.loginSuccessTargetScene() == ui::UIThreadRuntime::UIScene::ChartList);

    REQUIRE(runtime.switchTo(ui::UIThreadRuntime::UIScene::UserLogin));
    REQUIRE(runtime.currentScene() == ui::UIThreadRuntime::UIScene::UserLogin);

    loginUI.setSubmitModeLogin(true);
    loginUI.setUsernameInput("runtime_target_user");
    loginUI.setPasswordInput("pw");
    REQUIRE(loginUI.submit(&error));
    REQUIRE(error.empty());

    bool switched = false;
    for (int i = 0; i < 50; ++i) {
        if (runtime.currentScene() == ui::UIThreadRuntime::UIScene::ChartList && runtime.isChartListRunning()) {
            switched = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    REQUIRE(switched);

    runtime.stopAll();
    AuthenticatedUserService::logout();
    UserAccountsDAO::setDataDir(".");
}

TEST_CASE("UIThreadRuntime user settings success target scene is configurable", "[ui][UIThreadRuntime]") {
    TempDir userThemes("term4k_runtime_settings_user_themes");
    TempDir systemThemes("term4k_runtime_settings_system_themes");
    ThemePresetService::setThemeDirOverridesForTesting(userThemes.path().string(), systemThemes.path().string());

    {
        std::ofstream out((systemThemes.path() / "tomorrow-night.json").string(), std::ios::trunc);
        REQUIRE(out.is_open());
        out << R"({"id":"tomorrow-night","text.primary":"#ffffff"})";
        REQUIRE(out.good());
    }

    ui::StartMenuUI startMenu;
    ui::ChartListUI chartList;
    ChartListInstance chartListInstance;
    SettingsInstance settingsInstance;
    settingsInstance.loadFromRuntime();
    ui::UserSettingsUI settingsUI(&settingsInstance);

    ui::UIThreadRuntime runtime;
    runtime.bindStartMenu(&startMenu);
    runtime.bindChartList(&chartList, &chartListInstance);
    runtime.bindUserSettings(&settingsUI, &settingsInstance);

    runtime.setUserSettingsSuccessTargetScene(ui::UIThreadRuntime::UIScene::ChartList);
    REQUIRE(runtime.userSettingsSuccessTargetScene() == ui::UIThreadRuntime::UIScene::ChartList);

    REQUIRE(runtime.switchTo(ui::UIThreadRuntime::UIScene::UserSettings));
    REQUIRE(runtime.currentScene() == ui::UIThreadRuntime::UIScene::UserSettings);

    std::string error;
    REQUIRE(settingsUI.selectTheme("tomorrow-night", &error));
    REQUIRE(error.empty());

    bool switched = false;
    for (int i = 0; i < 50; ++i) {
        if (runtime.currentScene() == ui::UIThreadRuntime::UIScene::ChartList && runtime.isChartListRunning()) {
            switched = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    REQUIRE(switched);

    runtime.stopAll();
    ThemePresetService::clearThemeDirOverridesForTesting();
}


