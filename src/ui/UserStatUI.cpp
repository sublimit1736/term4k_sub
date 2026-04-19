#include "ui/UserStatUI.h"
#include "platform/AppDirs.h"
#include "platform/RuntimeConfig.h"
#include "account/RecordStore.h"
#include "account/UserStore.h"
#include "scenes/AdminStatScene.h"
#include "scenes/UserStatScene.h"
#include "account/UserContext.h"
#include "platform/I18n.h"
#include "ui/MessageOverlay.h"
#include "ui/TransitionBackdrop.h"
#include "ui/UIColors.h"
#include "ui/ThemeAdapter.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

namespace ui {
    namespace {
        ftxui::Element makeInfoRow(const ThemePalette &palette,
                                   const std::string &label,
                                   const std::string &value) {
            using namespace ftxui;
            return hbox({
                            text(label) | color(toColor(palette.textMuted)),
                            filler(),
                            text(value) | bold,
                        });
        }

        ftxui::Element makeTabLabel(const ThemePalette &palette,
                                    const std::string &label,
                                    const bool active) {
            using namespace ftxui;
            auto tab = text(" " + label + " ") | bold;
            if (active) {
                return tab | color(highContrastOn(palette.accentPrimary)) |
                       bgcolor(toColor(palette.accentPrimary));
            }
            return tab | color(toColor(palette.textMuted));
        }

        std::string formatDouble(const double value) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.3f", value);
            return {buf};
        }

        std::string formatFloat2(const float value) {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.2f%%", value >= 0.0f && value <= 1.0f ? value * 100.0f : value);
            return {buf};
        }

        std::string formatTimestamp(const uint32_t ts) {
            const auto t = static_cast<std::time_t>(ts);
            std::tm* tm_ptr     = std::localtime(&t);
            if (!tm_ptr) return std::to_string(ts);
            std::ostringstream oss;
            oss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
            return oss.str();
        }

        enum class AuthMode { Login, Register };
    } // namespace


    ftxui::Component UserStatUI::component(
        ftxui::ScreenInteractive &screen,
        std::function<void(UIScene)> onRoute
        ) {
        using namespace ftxui;

        I18n::instance().ensureLocaleLoaded(RuntimeConfig::locale);
        auto tr = [](const std::string &key) { return I18n::instance().get(key); };

        AppDirs::init();
        RecordStore::setDataDir(AppDirs::dataDir());

        struct UserStatState {
            ThemePalette palette;
            bool isAdmin = false;
            bool hasUser = false;
            bool isGuest = true;
            std::optional<User> current;
            UserStatScene userStats;
            AdminStatScene adminStats;
            std::string username;
            std::string password;
            std::string authStatus;
            int authFocus = 0;
            AuthMode authMode = AuthMode::Login;
            int activeTab = 0;
            int historyScrollOffset = 0;
            std::size_t historyDataRevision = 0;
            int historyCacheOffset = -1;
            std::size_t historyCacheRevision = static_cast<std::size_t>(-1);
            Elements historyRowsCache;

            // Async refresh support
            std::atomic<bool> isRefreshing{false};
            std::atomic<bool> keepRunning{true};
            std::thread refreshThread;

            ~UserStatState() {
                keepRunning.store(false, std::memory_order_relaxed);
                if (refreshThread.joinable()) refreshThread.join();
            }
        };

        auto state = std::make_shared<UserStatState>();
        state->palette = ThemeAdapter::resolveFromRuntime();

        // Async-safe refresh: runs all heavy I/O (chart catalog scan + record
        // verification) on a background thread so the render loop stays fluid.
        // The caller must ensure no previous thread is running (events are
        // blocked while isRefreshing == true, so this is always satisfied for
        // user-triggered refresh calls).
        auto startRefreshAsync = [state, &screen]() {
            if (state->refreshThread.joinable()) state->refreshThread.join();
            state->isRefreshing.store(true, std::memory_order_relaxed);
            state->refreshThread = std::thread([state, &screen]() {
                state->hasUser = UserContext::hasLoggedInUser();
                state->isAdmin = UserContext::isAdminUser();
                state->isGuest = UserContext::isGuestUser() || !state->hasUser;
                state->current = UserContext::currentUser();

                I18n::instance().ensureLocaleLoaded(RuntimeConfig::locale);
                state->palette = ThemeAdapter::resolveFromRuntime();

                if (!state->keepRunning.load(std::memory_order_relaxed)) {
                    state->isRefreshing.store(false, std::memory_order_release);
                    screen.PostEvent(ftxui::Event::Custom);
                    return;
                }

                if (state->isAdmin) {
                    state->adminStats.refresh(AppDirs::chartsDir());
                } else if (!state->isGuest) {
                    state->userStats.refresh(AppDirs::chartsDir());

                    if (state->current.has_value()) {
                        const std::string uid = std::to_string(state->current->getUID());
                        try {
                            const auto allRecs = RecordStore::readAllRecordByUID(uid);
                            const std::size_t verifiedCount = state->userStats.records().order.size();
                            if (!allRecs.empty() && verifiedCount == 0) {
                                MessageOverlay::push(MessageLevel::Error,
                                    I18n::instance().get("popup.error.record_db_error"));
                            } else if (allRecs.size() > verifiedCount) {
                                MessageOverlay::push(MessageLevel::Error,
                                    I18n::instance().get("popup.warning.record_tampered"));
                            }
                        } catch (...) {
                            MessageOverlay::push(MessageLevel::Error,
                                I18n::instance().get("popup.error.record_db_error"));
                        }
                    }
                }

                state->historyScrollOffset = 0;
                ++state->historyDataRevision;
                state->historyCacheOffset = -1;
                state->isRefreshing.store(false, std::memory_order_release);
                screen.PostEvent(ftxui::Event::Custom);
            });
        };

        // Kick off the initial refresh asynchronously.
        startRefreshAsync();

        InputOption userOpt;
        userOpt.placeholder = tr("ui.login.username");
        userOpt.transform = [state](const InputState &input) {
            return input.element |
                   color(toColor(input.is_placeholder ? state->palette.textMuted : state->palette.textPrimary)) |
                   bgcolor(toColor(state->palette.surfacePanel));
        };
        auto usernameInput = Input(&state->username, userOpt);

        InputOption passOpt;
        passOpt.placeholder = tr("ui.login.password");
        passOpt.password = true;
        passOpt.transform = [state](const InputState &input) {
            return input.element |
                   color(toColor(input.is_placeholder ? state->palette.textMuted : state->palette.textPrimary)) |
                   bgcolor(toColor(state->palette.surfacePanel));
        };
        auto passwordInput = Input(&state->password, passOpt);

        auto authContainer = Container::Vertical({usernameInput, passwordInput});
        auto root = Renderer(authContainer, [state, tr, usernameInput, passwordInput] {
            // While background refresh is running, show a loading indicator so
            // the render thread never touches partially-written stats fields.
            if (state->isRefreshing.load(std::memory_order_acquire)) {
                Element loading = vbox({
                    filler(),
                    text(tr("ui.loading.text")) | bold
                        | color(toColor(state->palette.accentPrimary))
                        | center,
                    filler(),
                }) | bgcolor(toColor(state->palette.surfaceBg)) | flex;
                Element composed = dbox({loading, MessageOverlay::render(state->palette)});
                TransitionBackdrop::update(composed);
                return composed;
            }

            if (state->isGuest){
                const std::string title = state->authMode == AuthMode::Login
                                              ? tr("ui.auth.login_title")
                                              : tr("ui.auth.register_title");
                Element userRow = window(text(" " + tr("ui.login.username") + " "), usernameInput->Render())
                                  | size(WIDTH, GREATER_THAN, 30)
                                  | color(toColor(state->authFocus == 0 ? state->palette.accentPrimary : state->palette.borderNormal))
                                  | bgcolor(toColor(state->palette.surfacePanel));
                Element passRow = window(text(" " + tr("ui.login.password") + " "), passwordInput->Render())
                                  | size(WIDTH, GREATER_THAN, 30)
                                  | color(toColor(state->authFocus == 1 ? state->palette.accentPrimary : state->palette.borderNormal))
                                  | bgcolor(toColor(state->palette.surfacePanel));
                Element statusLine = state->authStatus.empty()
                                         ? text("")
                                         : text(state->authStatus) | color(toColor(state->palette.textMuted));
                Element guestBase = vbox({
                                             filler(),
                                             text(title) | bold | color(toColor(state->palette.accentPrimary)) | center,
                                             text(tr("ui.user_info.auth_switch_hint")) |
                                             color(toColor(state->palette.textMuted)) | center,
                                             text(""),
                                             hbox({filler(), vbox({userRow, passRow, statusLine}), filler()}),
                                             filler(),
                                         })
                                    | bgcolor(toColor(state->palette.surfacePanel))
                                    | color(toColor(state->palette.textPrimary))
                                    | flex;
                Element composed = dbox({guestBase, MessageOverlay::render(state->palette)});
                TransitionBackdrop::update(composed);
                return composed;
            }

            Element logoutBtn = text(" [ " + tr("ui.user_info.logout") + " ] ")
                                | color(Color::Red)
                                | bold;
            Element tabBar = hbox({
                                      makeTabLabel(state->palette, tr("ui.user_info.tab.info"), state->activeTab == 0),
                                      text("  "),
                                      makeTabLabel(state->palette, tr("ui.user_info.tab.history"), state->activeTab == 1),
                                      filler(),
                                      logoutBtn,
                                      text("  "),
                                       text(tr("ui.user_info.hint")) | color(toColor(state->palette.textMuted)),
                                  });
            Element body;
            if (state->activeTab == 0){
                // ---- Tab 0: Basic info ----
                if (state->isAdmin){
                    const auto &verified        = state->adminStats.playerStats(AdminRecordScope::VerifiedOnly);
                    const auto &all             = state->adminStats.playerStats(AdminRecordScope::AllRecords);
                    std::size_t verifiedRecords = 0;
                    for (const auto &[_, stats]: verified) verifiedRecords += stats.records.order.size();
                    std::size_t allRecords = 0;
                    for (const auto &[_, stats]: all) allRecords += stats.records.order.size();
                    body = vbox({
                                    text(tr("ui.user_info.admin_badge")) | bold | color(toColor(state->palette.accentPrimary)),
                                    separator(),
                                    makeInfoRow(state->palette, tr("ui.user_info.admin_users_verified"), std::to_string(verified.size())),
                                    makeInfoRow(state->palette, tr("ui.user_info.admin_records_verified"), std::to_string(verifiedRecords)),
                                    makeInfoRow(state->palette, tr("ui.user_info.admin_users_all"), std::to_string(all.size())),
                                    makeInfoRow(state->palette, tr("ui.user_info.admin_records_all"), std::to_string(allRecords)),
                                }) | color(toColor(state->palette.textPrimary));
                }
                else{
                    const std::string userName = state->current.has_value()
                                                     ? state->current->getUsername()
                                                     : tr("ui.login.user_unknown");
                    const std::string uid = state->current.has_value() ? std::to_string(state->current->getUID()) : "-";
                    const auto &records   = state->userStats.records();
                    body                  = vbox({
                                                     makeInfoRow(state->palette, tr("ui.user_info.username"), userName),
                                                     makeInfoRow(state->palette, tr("ui.user_info.uid"), uid),
                                                     makeInfoRow(state->palette, tr("ui.user_info.rating"),
                                                                 formatDouble(state->userStats.rating())),
                                                     makeInfoRow(state->palette, tr("ui.user_info.potential"),
                                                                 formatDouble(state->userStats.potential())),
                                                     makeInfoRow(state->palette, tr("ui.user_info.record_count"),
                                                                 std::to_string(records.order.size())),
                                                 }) | color(toColor(state->palette.textPrimary));
                }
            }
            else{
                // ---- Tab 1: Play history (earliest to latest) ----
                const auto &records = state->userStats.records();
                const auto &order   = records.order; // currently sorted newest-first
                if (state->historyCacheRevision != state->historyDataRevision ||
                    state->historyCacheOffset != state->historyScrollOffset) {
                    Elements histRows;
                    if (order.empty()){
                        histRows.push_back(
                                           text(tr("ui.user_info.history.no_records")) | color(toColor(state->palette.textMuted)) |
                                           center
                                          );
                    }
                    else{
                        // Iterate from index 0 (newest first, order is newest-first)
                        const int total         = static_cast<int>(order.size());
                        const int visibleLines  = 20; // approximate visible rows
                        const int maxOffset     = std::max(0, total - 1);
                        const int clampedOffset = std::min(std::max(state->historyScrollOffset, 0), maxOffset);
                        for (int idx = clampedOffset; idx < total; ++idx){
                            const auto it = records.records.find(order[static_cast<std::size_t>(idx)]);
                            if (it == records.records.end()) continue;
                            const auto &entry           = it->second;
                            const int displayNum        = idx + 1; // 1-based from newest
                            const std::string chartName = entry.chart.getDisplayName().empty()
                                                              ? entry.chartID
                                                              : entry.chart.getDisplayName();
                            Element row = vbox({
                                                   hbox({
                                                            text(std::to_string(displayNum) + ". ") | bold |
                                                            color(toColor(state->palette.accentPrimary)),
                                                            text(tr("ui.user_info.history.chart")) |
                                                            color(toColor(state->palette.textMuted)),
                                                            text(chartName) | bold,
                                                            filler(),
                                                            text(formatTimestamp(entry.timestamp)) |
                                                            color(toColor(state->palette.textMuted)),
                                                        }),
                                                   hbox({
                                                            text("   "),
                                                            text(tr("ui.user_info.history.score")) |
                                                            color(toColor(state->palette.textMuted)),
                                                            text(std::to_string(entry.score)) | bold,
                                                            text("  "),
                                                            text(tr("ui.user_info.history.accuracy")) |
                                                            color(toColor(state->palette.textMuted)),
                                                            text(formatFloat2(entry.accuracy)) | bold,
                                                            text("  "),
                                                            text(tr("ui.user_info.history.combo")) |
                                                            color(toColor(state->palette.textMuted)),
                                                            text(std::to_string(entry.maxCombo)) | bold,
                                                        }),
                                                   separator() | color(toColor(state->palette.borderNormal)),
                                               });
                            histRows.push_back(row);
                            if (static_cast<int>(histRows.size()) >= visibleLines) break;
                        }
                    }
                    state->historyRowsCache = std::move(histRows);
                    state->historyCacheRevision = state->historyDataRevision;
                    state->historyCacheOffset = state->historyScrollOffset;
                }
                body = vbox(state->historyRowsCache) | color(toColor(state->palette.textPrimary)) | frame | flex;
            }
            Element base = window(
                                  text("  " + tr("ui.user_info.title") + "  ") | bold |
                                  color(toColor(state->palette.accentPrimary)),
                                  vbox({
                                           tabBar,
                                           separator(),
                                           body | flex,
                                       }) | bgcolor(toColor(state->palette.surfaceBg)) | color(toColor(state->palette.textPrimary))
                                 )
                           | color(toColor(state->palette.borderNormal))
                           | bgcolor(toColor(state->palette.surfaceBg))
                           | flex;
            Element composed = dbox({base, MessageOverlay::render(state->palette)});
            TransitionBackdrop::update(composed);
            return composed;
        });

        auto handleGuestEvent = [state, tr, usernameInput, passwordInput, startRefreshAsync](const Event &event) {
            if (event == Event::TabReverse){
                state->authMode = state->authMode == AuthMode::Login ? AuthMode::Register : AuthMode::Login;
                state->authStatus.clear();
                state->username.clear();
                state->password.clear();
                state->authFocus = 0;
                return true;
            }
            if (event == Event::Tab || event == Event::ArrowUp || event == Event::ArrowDown){
                state->authFocus = (state->authFocus + 1) % 2;
                return true;
            }
            if (event == Event::Return){
                if (state->username.empty()){
                    state->authFocus = 0;
                    return true;
                }
                if (state->password.empty()){
                    state->authFocus = 1;
                    return true;
                }
                if (state->authMode == AuthMode::Login){
                    if (state->userStats.login(state->username, state->password)){
                        state->authStatus.clear();
                        startRefreshAsync();
                        MessageOverlay::push(MessageLevel::Info,
                            I18n::instance().get("popup.info.login_succeeded"));
                    }
                    else{
                        // E9: check if login failure is due to account DB being inaccessible.
                        if (!UserStore::isAccountDBAccessible()) {
                            MessageOverlay::push(MessageLevel::Error,
                                I18n::instance().get("popup.error.account_db_inaccessible"));
                        }
                        state->authStatus = tr("ui.user_info.auth_login_failed");
                        state->username.clear();
                        state->password.clear();
                        state->authFocus = 0;
                    }
                }
                else{
                    std::string err;
                    if (state->userStats.registerUser(state->username, state->password, &err)){
                        state->authMode = AuthMode::Login;
                        state->authStatus.clear();
                        state->username.clear();
                        state->password.clear();
                        state->authFocus = 0;
                        MessageOverlay::push(MessageLevel::Info,
                            I18n::instance().get("popup.info.register_succeeded"));
                    }
                    else{
                        // E9: check if register failure is due to account DB being inaccessible.
                        if (!UserStore::isAccountDBAccessible()) {
                            MessageOverlay::push(MessageLevel::Error,
                                I18n::instance().get("popup.error.account_db_inaccessible"));
                        }
                        state->authStatus = err.empty() ? tr("ui.login.result.register_failed") : err;
                    }
                }
                return true;
            }
            if (state->authFocus == 0) return usernameInput->OnEvent(event);
            return passwordInput->OnEvent(event);
        };

        auto handleLoggedInEvent = [state, startRefreshAsync](const Event &event) {
            // Logout
            if (event == Event::Character('L')){
                state->userStats.logout();
                startRefreshAsync();
                return true;
            }
            // Logged-in user tab switching
            if (event == Event::Tab){
                state->activeTab           = (state->activeTab + 1) % 2;
                state->historyScrollOffset = 0;
                return true;
            }
            // History scrolling
            if (state->activeTab == 1){
                const auto &records = state->userStats.records();
                const int total     = static_cast<int>(records.order.size());
                if (event == Event::Character('j') || event == Event::ArrowDown){
                    state->historyScrollOffset = std::min(state->historyScrollOffset + 1, std::max(0, total - 1));
                    return true;
                }
                if (event == Event::Character('k') || event == Event::ArrowUp){
                    state->historyScrollOffset = std::max(state->historyScrollOffset - 1, 0);
                    return true;
                }
            }
            return false;
        };

        auto app = CatchEvent(root, [state,
                                     handleGuestEvent,
                                     handleLoggedInEvent,
                                     onRoute = std::move(onRoute)](const Event &event) {
            if (MessageOverlay::handleEvent(event)){
                return true;
            }

            // While the background refresh is running, only Escape/q is
            // allowed so the user can leave; all other events are suppressed
            // to prevent accessing partially-written stats fields.
            if (state->isRefreshing.load(std::memory_order_acquire)) {
                if (event == Event::Escape || event == Event::Character('q')) {
                    state->keepRunning.store(false, std::memory_order_relaxed);
                    if (state->refreshThread.joinable()) state->refreshThread.detach();
                    onRoute(UIScene::StartMenu);
                    return true;
                }
                return false;
            }

            if (event == Event::Escape || event == Event::Character('q')){
                onRoute(UIScene::StartMenu);
                return true;
            }
            if (state->isGuest) return handleGuestEvent(event);
            return handleLoggedInEvent(event);
        });

        return app;
    }
} // namespace ui
