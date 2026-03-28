#include "UIThreadRuntime.h"

#include <functional>

namespace ui {

UIThreadRuntime::~UIThreadRuntime() {
    stopAll();
}

void UIThreadRuntime::bindStartMenu(StartMenuUI *ui) {
    std::lock_guard<std::mutex> lock(bindingMutex_);
    startMenuUI_ = ui;
}

void UIThreadRuntime::bindAdminUserStat(AdminUserStatUI *ui, AdminStatInstance *instance) {
    std::lock_guard<std::mutex> lock(bindingMutex_);
    adminUserStatUI_ = ui;
    adminStatInstance_ = instance;
    if (adminUserStatUI_) adminUserStatUI_->bindInstance(adminStatInstance_);
}

void UIThreadRuntime::bindChartList(ChartListUI *ui, ChartListInstance *instance) {
    std::lock_guard<std::mutex> lock(bindingMutex_);
    chartListUI_ = ui;
    chartListInstance_ = instance;
    if (chartListUI_) chartListUI_->bindInstance(chartListInstance_);
}

void UIThreadRuntime::bindGameplay(GameplayUI *ui, GameplayInstance *instance) {
    std::lock_guard<std::mutex> lock(bindingMutex_);
    gameplayUI_ = ui;
    gameplayInstance_ = instance;
    if (gameplayUI_) gameplayUI_->bindInstance(gameplayInstance_);
}

void UIThreadRuntime::bindGameplaySettlement(GameplaySettlementUI *ui,
                                             GameplaySettlementInstance *instance) {
    std::lock_guard<std::mutex> lock(bindingMutex_);
    gameplaySettlementUI_ = ui;
    gameplaySettlementInstance_ = instance;
    if (gameplaySettlementUI_) gameplaySettlementUI_->bindInstance(gameplaySettlementInstance_);
}

void UIThreadRuntime::bindUserStat(UserStatUI *ui, UserStatInstance *instance) {
    std::lock_guard<std::mutex> lock(bindingMutex_);
    userStatUI_ = ui;
    userStatInstance_ = instance;
    if (userStatUI_) userStatUI_->bindInstance(userStatInstance_);
}

void UIThreadRuntime::bindUserLogin(UserLoginUI *ui, UserLoginInstance *loginInstance) {
    std::lock_guard<std::mutex> lock(bindingMutex_);
    userLoginUI_ = ui;
    userLoginInstance_ = loginInstance;
    if (userLoginUI_) userLoginUI_->bindInstance(userLoginInstance_);
}

void UIThreadRuntime::bindUserSettings(UserSettingsUI *ui, SettingsInstance *settingsInstance) {
    std::lock_guard<std::mutex> lock(bindingMutex_);
    userSettingsUI_ = ui;
    settingsInstance_ = settingsInstance;
    if (userSettingsUI_) userSettingsUI_->bindInstance(settingsInstance_);
}

bool UIThreadRuntime::startStartMenu() {
    StartMenuUI *ui = nullptr;
    {
        std::lock_guard<std::mutex> lock(bindingMutex_);
        ui = startMenuUI_;
    }
    if (!ui) return false;
    return startSlot(startMenuSlot_, [ui]() { ui->show(); }, [ui]() { ui->hide(); });
}

bool UIThreadRuntime::startAdminUserStat() {
    AdminUserStatUI *ui = nullptr;
    {
        std::lock_guard<std::mutex> lock(bindingMutex_);
        ui = adminUserStatUI_;
    }
    if (!ui) return false;
    return startSlot(adminUserStatSlot_, [ui]() { ui->show(); }, [ui]() { ui->hide(); });
}

bool UIThreadRuntime::startChartList() {
    ChartListUI *ui = nullptr;
    {
        std::lock_guard<std::mutex> lock(bindingMutex_);
        ui = chartListUI_;
    }
    if (!ui) return false;
    return startSlot(chartListSlot_, [ui]() { ui->show(); }, [ui]() { ui->hide(); });
}

bool UIThreadRuntime::startGameplay() {
    GameplayUI *ui = nullptr;
    {
        std::lock_guard<std::mutex> lock(bindingMutex_);
        ui = gameplayUI_;
    }
    if (!ui) return false;
    return startSlot(gameplaySlot_, [ui]() { ui->show(); }, [ui]() { ui->hide(); });
}

bool UIThreadRuntime::startGameplaySettlement() {
    GameplaySettlementUI *ui = nullptr;
    {
        std::lock_guard<std::mutex> lock(bindingMutex_);
        ui = gameplaySettlementUI_;
    }
    if (!ui) return false;
    return startSlot(gameplaySettlementSlot_, [ui]() { ui->show(); }, [ui]() { ui->hide(); });
}

bool UIThreadRuntime::startUserStat() {
    UserStatUI *ui = nullptr;
    {
        std::lock_guard<std::mutex> lock(bindingMutex_);
        ui = userStatUI_;
    }
    if (!ui) return false;
    return startSlot(userStatSlot_, [ui]() { ui->show(); }, [ui]() { ui->hide(); });
}

bool UIThreadRuntime::startUserLogin() {
    UserLoginUI *ui = nullptr;
    {
        std::lock_guard<std::mutex> lock(bindingMutex_);
        ui = userLoginUI_;
    }
    if (!ui) return false;
    return startSlot(userLoginSlot_, [ui]() { ui->show(); }, [ui]() { ui->hide(); });
}

bool UIThreadRuntime::startUserSettings() {
    UserSettingsUI *ui = nullptr;
    {
        std::lock_guard<std::mutex> lock(bindingMutex_);
        ui = userSettingsUI_;
    }
    if (!ui) return false;
    return startSlot(userSettingsSlot_, [ui]() { ui->show(); }, [ui]() { ui->hide(); });
}

void UIThreadRuntime::stopStartMenu() { stopSlot(startMenuSlot_); }
void UIThreadRuntime::stopAdminUserStat() { stopSlot(adminUserStatSlot_); }
void UIThreadRuntime::stopChartList() { stopSlot(chartListSlot_); }
void UIThreadRuntime::stopGameplay() { stopSlot(gameplaySlot_); }
void UIThreadRuntime::stopGameplaySettlement() { stopSlot(gameplaySettlementSlot_); }
void UIThreadRuntime::stopUserStat() { stopSlot(userStatSlot_); }
void UIThreadRuntime::stopUserLogin() { stopSlot(userLoginSlot_); }
void UIThreadRuntime::stopUserSettings() { stopSlot(userSettingsSlot_); }

void UIThreadRuntime::startAll() {
    startStartMenu();
    startAdminUserStat();
    startChartList();
    startGameplay();
    startGameplaySettlement();
    startUserStat();
    startUserLogin();
    startUserSettings();
}

void UIThreadRuntime::stopAll() {
    stopStartMenu();
    stopAdminUserStat();
    stopChartList();
    stopGameplay();
    stopGameplaySettlement();
    stopUserStat();
    stopUserLogin();
    stopUserSettings();

    std::lock_guard<std::mutex> lock(sceneMutex_);
    currentScene_ = UIScene::None;
}

bool UIThreadRuntime::isStartMenuRunning() const { return slotRunning(startMenuSlot_); }
bool UIThreadRuntime::isAdminUserStatRunning() const { return slotRunning(adminUserStatSlot_); }
bool UIThreadRuntime::isChartListRunning() const { return slotRunning(chartListSlot_); }
bool UIThreadRuntime::isGameplayRunning() const { return slotRunning(gameplaySlot_); }
bool UIThreadRuntime::isGameplaySettlementRunning() const { return slotRunning(gameplaySettlementSlot_); }
bool UIThreadRuntime::isUserStatRunning() const { return slotRunning(userStatSlot_); }
bool UIThreadRuntime::isUserLoginRunning() const { return slotRunning(userLoginSlot_); }
bool UIThreadRuntime::isUserSettingsRunning() const { return slotRunning(userSettingsSlot_); }

bool UIThreadRuntime::switchTo(const UIScene scene) {
    stopAll();

    bool started = false;
    switch (scene){
        case UIScene::None:
            started = true;
            break;
        case UIScene::StartMenu:
            started = startStartMenu();
            break;
        case UIScene::AdminUserStat:
            started = startAdminUserStat();
            break;
        case UIScene::ChartList:
            started = startChartList();
            break;
        case UIScene::Gameplay:
            started = startGameplay();
            break;
        case UIScene::GameplaySettlement:
            started = startGameplaySettlement();
            break;
        case UIScene::UserStat:
            started = startUserStat();
            break;
        case UIScene::UserLogin:
            started = startUserLogin();
            break;
        case UIScene::UserSettings:
            started = startUserSettings();
            break;
    }

    std::lock_guard<std::mutex> lock(sceneMutex_);
    currentScene_ = started ? scene : UIScene::None;
    return started;
}

UIThreadRuntime::UIScene UIThreadRuntime::currentScene() const {
    std::lock_guard<std::mutex> lock(sceneMutex_);
    return currentScene_;
}

bool UIThreadRuntime::startSlot(UISlot &slot,
                                const std::function<void()> &onStart,
                                const std::function<void()> &onStop) {
    std::lock_guard<std::mutex> lock(slot.mutex);
    if (slot.running) return false;

    slot.stopRequested = false;
    slot.running = true;

    slot.worker = std::thread([&slot, onStart, onStop]() {
        onStart();

        std::unique_lock<std::mutex> waitLock(slot.mutex);
        slot.cv.wait(waitLock, [&slot]() { return slot.stopRequested; });
        waitLock.unlock();

        onStop();

        std::lock_guard<std::mutex> doneLock(slot.mutex);
        slot.running = false;
    });

    return true;
}

void UIThreadRuntime::stopSlot(UISlot &slot) {
    std::thread worker;
    {
        std::lock_guard<std::mutex> lock(slot.mutex);
        if (!slot.running && !slot.worker.joinable()) return;
        slot.stopRequested = true;
        slot.cv.notify_all();
        worker = std::move(slot.worker);
    }

    if (worker.joinable()) worker.join();
}

bool UIThreadRuntime::slotRunning(const UISlot &slot) {
    std::lock_guard<std::mutex> lock(slot.mutex);
    return slot.running;
}

} // namespace ui


