#pragma once

#include "ui/AdminUserStatUI.h"
#include "ui/ChartListUI.h"
#include "ui/GameplaySettlementUI.h"
#include "ui/GameplayUI.h"
#include "ui/StartMenuUI.h"
#include "ui/UserLoginUI.h"
#include "ui/UserSettingsUI.h"
#include "ui/UserStatUI.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace ui {

// Manages one UI-per-thread lifecycle while keeping UI implementations as placeholders.
class UIThreadRuntime {
public:
    enum class UIScene {
        None,
        StartMenu,
        AdminUserStat,
        ChartList,
        Gameplay,
        GameplaySettlement,
        UserStat,
        UserLogin,
        UserSettings,
    };

    UIThreadRuntime() = default;

    ~UIThreadRuntime();

    void bindStartMenu(StartMenuUI *ui);
    void bindAdminUserStat(AdminUserStatUI *ui, AdminStatInstance *instance);
    void bindChartList(ChartListUI *ui, ChartListInstance *instance);
    void bindGameplay(GameplayUI *ui, GameplayInstance *instance);
    void bindGameplaySettlement(GameplaySettlementUI *ui, GameplaySettlementInstance *instance);
    void bindUserStat(UserStatUI *ui, UserStatInstance *instance);
    void bindUserLogin(UserLoginUI *ui, UserLoginInstance *loginInstance);
    void bindUserSettings(UserSettingsUI *ui, SettingsInstance *settingsInstance);

    bool startStartMenu();
    bool startAdminUserStat();
    bool startChartList();
    bool startGameplay();
    bool startGameplaySettlement();
    bool startUserStat();
    bool startUserLogin();
    bool startUserSettings();

    void stopStartMenu();
    void stopAdminUserStat();
    void stopChartList();
    void stopGameplay();
    void stopGameplaySettlement();
    void stopUserStat();
    void stopUserLogin();
    void stopUserSettings();

    void startAll();
    void stopAll();

    bool isStartMenuRunning() const;
    bool isAdminUserStatRunning() const;
    bool isChartListRunning() const;
    bool isGameplayRunning() const;
    bool isGameplaySettlementRunning() const;
    bool isUserStatRunning() const;
    bool isUserLoginRunning() const;
    bool isUserSettingsRunning() const;

    bool switchTo(UIScene scene);

    UIScene currentScene() const;

private:
    struct UISlot {
        std::thread worker;
        mutable std::mutex mutex;
        std::condition_variable cv;
        bool stopRequested = false;
        bool running = false;
    };

    static bool startSlot(UISlot &slot, const std::function<void()> &onStart, const std::function<void()> &onStop);
    static void stopSlot(UISlot &slot);
    static bool slotRunning(const UISlot &slot);

    mutable std::mutex bindingMutex_;

    StartMenuUI *startMenuUI_ = nullptr;
    AdminUserStatUI *adminUserStatUI_ = nullptr;
    ChartListUI *chartListUI_ = nullptr;
    GameplayUI *gameplayUI_ = nullptr;
    GameplaySettlementUI *gameplaySettlementUI_ = nullptr;
    UserStatUI *userStatUI_ = nullptr;
    UserLoginUI *userLoginUI_ = nullptr;
    UserSettingsUI *userSettingsUI_ = nullptr;

    AdminStatInstance *adminStatInstance_ = nullptr;
    ChartListInstance *chartListInstance_ = nullptr;
    GameplayInstance *gameplayInstance_ = nullptr;
    GameplaySettlementInstance *gameplaySettlementInstance_ = nullptr;
    UserStatInstance *userStatInstance_ = nullptr;
    UserLoginInstance *userLoginInstance_ = nullptr;
    SettingsInstance *settingsInstance_ = nullptr;

    UISlot startMenuSlot_;
    UISlot adminUserStatSlot_;
    UISlot chartListSlot_;
    UISlot gameplaySlot_;
    UISlot gameplaySettlementSlot_;
    UISlot userStatSlot_;
    UISlot userLoginSlot_;
    UISlot userSettingsSlot_;

    mutable std::mutex sceneMutex_;
    UIScene currentScene_ = UIScene::None;
};

} // namespace ui




