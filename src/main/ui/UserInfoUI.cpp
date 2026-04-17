#include "ui/UserInfoUI.h"
#include "config/AppDirs.h"
#include "config/RuntimeConfigs.h"
#include "dao/ProofedRecordsDAO.h"
#include "instances/AdminStatInstance.h"
#include "instances/UserLoginInstance.h"
#include "instances/UserStatInstance.h"
#include "services/AuthenticatedUserService.h"
#include "services/I18nService.h"
#include "ui/MessageOverlay.h"
#include "ui/TransitionBackdrop.h"
#include "ui/UIColors.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
namespace ui {
namespace {
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
    const std::time_t t = static_cast<std::time_t>(ts);
    std::tm *tm_ptr = std::localtime(&t);
    if (!tm_ptr) return std::to_string(ts);
    std::ostringstream oss;
    oss << std::put_time(tm_ptr, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
enum class AuthMode { Login, Register };
} // namespace
int UserInfoUI::run() {
    auto screen = ftxui::ScreenInteractive::Fullscreen();
    return run(screen);
}

int UserInfoUI::run(ftxui::ScreenInteractive &screen) {
    using namespace ftxui;
    I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
    auto tr = [](const std::string &key) { return I18nService::instance().get(key); };
    ThemePalette palette = ThemeAdapter::resolveFromRuntime();
    AppDirs::init();
    ProofedRecordsDAO::setDataDir(AppDirs::dataDir());
    bool isAdmin = false;
    bool hasUser = false;
    bool isGuest = true;
    std::optional<User> current;
    UserStatInstance userStats;
    AdminStatInstance adminStats;
    UserLoginInstance loginInstance;
    std::string username;
    std::string password;
    std::string authStatus;
    int authFocus = 0;
    AuthMode authMode = AuthMode::Login;
    // Tab state: 0 = basic info, 1 = play history
    int activeTab = 0;
    int historyScrollOffset = 0;
    auto refreshSession = [&] {
        hasUser = AuthenticatedUserService::hasLoggedInUser();
        isAdmin = AuthenticatedUserService::isAdminUser();
        isGuest = AuthenticatedUserService::isGuestUser() || !hasUser;
        current = AuthenticatedUserService::currentUser();

        // UserLoginService loads RuntimeConfigs on login; refresh locale/theme immediately.
        I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
        palette = ThemeAdapter::resolveFromRuntime();

        if (isAdmin) {
            adminStats.refresh(AppDirs::chartsDir());
        } else if (!isGuest) {
            userStats.refresh(AppDirs::chartsDir());
        }
        historyScrollOffset = 0;
    };
    refreshSession();
    InputOption userOpt;
    userOpt.placeholder = tr("ui.login.username");
    userOpt.transform = [&](const InputState &state) {
        return state.element |
               color(toColor(state.is_placeholder ? palette.textMuted : palette.textPrimary)) |
               bgcolor(toColor(palette.surfacePanel));
    };
    auto usernameInput = Input(&username, userOpt);
    InputOption passOpt;
    passOpt.placeholder = tr("ui.login.password");
    passOpt.password = true;
    passOpt.transform = [&](const InputState &state) {
        return state.element |
               color(toColor(state.is_placeholder ? palette.textMuted : palette.textPrimary)) |
               bgcolor(toColor(palette.surfacePanel));
    };
    auto passwordInput = Input(&password, passOpt);
    auto authContainer = Container::Vertical({usernameInput, passwordInput});
    auto root = Renderer(authContainer, [&] {
        if (isGuest) {
            const std::string title = authMode == AuthMode::Login
                                          ? tr("ui.auth.login_title")
                                          : tr("ui.auth.register_title");
            Element userRow = window(text(" " + tr("ui.login.username") + " "), usernameInput->Render())
                              | size(WIDTH, GREATER_THAN, 30)
                              | color(toColor(authFocus == 0 ? palette.accentPrimary : palette.borderNormal))
                              | bgcolor(toColor(palette.surfacePanel));
            Element passRow = window(text(" " + tr("ui.login.password") + " "), passwordInput->Render())
                              | size(WIDTH, GREATER_THAN, 30)
                              | color(toColor(authFocus == 1 ? palette.accentPrimary : palette.borderNormal))
                              | bgcolor(toColor(palette.surfacePanel));
            Element statusLine = authStatus.empty()
                                     ? text("")
                                     : text(authStatus) | color(toColor(palette.textMuted));
            Element guestBase = vbox({
                       filler(),
                       text(title) | bold | color(toColor(palette.accentPrimary)) | center,
                       text(tr("ui.user_info.auth_switch_hint")) | color(toColor(palette.textMuted)) | center,
                       text(""),
                       hbox({filler(), vbox({userRow, passRow, statusLine}), filler()}),
                       filler(),
                   })
                   | bgcolor(toColor(palette.surfacePanel))
                   | color(toColor(palette.textPrimary))
                   | flex;
            Element composed = dbox({guestBase, MessageOverlay::render(palette)});
            TransitionBackdrop::update(composed);
            return composed;
        }
        // Helper: one info row — label left-aligned, value right-aligned.
        auto infoRow = [&](const std::string &label, const std::string &value) -> Element {
            return hbox({
                text(label) | color(toColor(palette.textMuted)),
                filler(),
                text(value) | bold,
            });
        };
        // ---- Tab bar ----
        auto tabLabel = [&](const std::string &label, const int idx) -> Element {
            const bool active = (activeTab == idx);
            Element t = text(" " + label + " ") | bold;
            if (active) {
                t = t | color(highContrastOn(palette.accentPrimary)) | bgcolor(toColor(palette.accentPrimary));
            } else {
                t = t | color(toColor(palette.textMuted));
            }
            return t;
        };
        Element logoutBtn = text(" [ " + tr("ui.user_info.logout") + " ] ")
                            | color(ftxui::Color::Red)
                            | bold;
        Element tabBar = hbox({
            tabLabel(tr("ui.user_info.tab.info"), 0),
            text("  "),
            tabLabel(tr("ui.user_info.tab.history"), 1),
            filler(),
            logoutBtn,
            text("  "),
            text(tr("ui.user_info.hint")) | color(toColor(palette.textMuted)),
        });
        Element body;
        if (activeTab == 0) {
            // ---- Tab 0: Basic info ----
            if (isAdmin) {
                const auto &verified = adminStats.playerStats(AdminRecordScope::VerifiedOnly);
                const auto &all = adminStats.playerStats(AdminRecordScope::AllRecords);
                std::size_t verifiedRecords = 0;
                for (const auto &[_, stats] : verified) verifiedRecords += stats.records.order.size();
                std::size_t allRecords = 0;
                for (const auto &[_, stats] : all) allRecords += stats.records.order.size();
                body = vbox({
                    text(tr("ui.user_info.admin_badge")) | bold | color(toColor(palette.accentPrimary)),
                    separator(),
                    infoRow(tr("ui.user_info.admin_users_verified"), std::to_string(verified.size())),
                    infoRow(tr("ui.user_info.admin_records_verified"), std::to_string(verifiedRecords)),
                    infoRow(tr("ui.user_info.admin_users_all"), std::to_string(all.size())),
                    infoRow(tr("ui.user_info.admin_records_all"), std::to_string(allRecords)),
                }) | color(toColor(palette.textPrimary));
            } else {
                const std::string userName = current.has_value() ? current->getUsername() : tr("ui.login.user_unknown");
                const std::string uid = current.has_value() ? std::to_string(current->getUID()) : "-";
                const auto &records = userStats.records();
                body = vbox({
                    infoRow(tr("ui.user_info.username"), userName),
                    infoRow(tr("ui.user_info.uid"), uid),
                    infoRow(tr("ui.user_info.rating"), formatDouble(userStats.rating())),
                    infoRow(tr("ui.user_info.potential"), formatDouble(userStats.potential())),
                    infoRow(tr("ui.user_info.record_count"), std::to_string(records.order.size())),
                }) | color(toColor(palette.textPrimary));
            }
        } else {
            // ---- Tab 1: Play history (earliest to latest) ----
            const auto &records = userStats.records();
            const auto &order = records.order; // currently sorted newest-first
            Elements histRows;
            if (order.empty()) {
                histRows.push_back(
                    text(tr("ui.user_info.history.no_records")) | color(toColor(palette.textMuted)) | center
                );
            } else {
                // Iterate in reverse to show earliest first
                const int total = static_cast<int>(order.size());
                const int visibleLines = 20; // approximate visible rows
                const int maxOffset = std::max(0, total - 1);
                const int clampedOffset = std::min(std::max(historyScrollOffset, 0), maxOffset);
                for (int idx = total - 1 - clampedOffset; idx >= 0; --idx) {
                    const auto it = records.records.find(order[static_cast<std::size_t>(idx)]);
                    if (it == records.records.end()) continue;
                    const auto &entry = it->second;
                    const int displayNum = total - idx; // 1-based from earliest
                    const std::string chartName = entry.chart.getDisplayName().empty()
                        ? entry.chartID
                        : entry.chart.getDisplayName();
                    Element row = vbox({
                        hbox({
                            text(std::to_string(displayNum) + ". ") | bold | color(toColor(palette.accentPrimary)),
                            text(tr("ui.user_info.history.chart")) | color(toColor(palette.textMuted)),
                            text(chartName) | bold,
                            filler(),
                            text(formatTimestamp(entry.timestamp)) | color(toColor(palette.textMuted)),
                        }),
                        hbox({
                            text("   "),
                            text(tr("ui.user_info.history.score")) | color(toColor(palette.textMuted)),
                            text(std::to_string(entry.score)) | bold,
                            text("  "),
                            text(tr("ui.user_info.history.accuracy")) | color(toColor(palette.textMuted)),
                            text(formatFloat2(entry.accuracy)) | bold,
                            text("  "),
                            text(tr("ui.user_info.history.combo")) | color(toColor(palette.textMuted)),
                            text(std::to_string(entry.maxCombo)) | bold,
                        }),
                        separator() | color(toColor(palette.borderNormal)),
                    });
                    histRows.push_back(row);
                    if (static_cast<int>(histRows.size()) >= visibleLines) break;
                }
            }
            body = vbox(std::move(histRows)) | color(toColor(palette.textPrimary)) | frame | flex;
        }
        Element base = window(
                   text("  " + tr("ui.user_info.title") + "  ") | bold | color(toColor(palette.accentPrimary)),
                   vbox({
                       tabBar,
                       separator(),
                       body | flex,
                   }) | bgcolor(toColor(palette.surfaceBg)) | color(toColor(palette.textPrimary))
               )
               | color(toColor(palette.borderNormal))
               | bgcolor(toColor(palette.surfaceBg))
               | flex;
        Element composed = dbox({base, MessageOverlay::render(palette)});
        TransitionBackdrop::update(composed);
        return composed;
    });
    auto app = CatchEvent(root, [&](const Event &event) {
        if (MessageOverlay::handleEvent(event)) {
            return true;
        }

        if (event == Event::Escape || event == Event::Character('q')) {
            screen.ExitLoopClosure()();
            return true;
        }
        if (isGuest) {
            if (event == Event::TabReverse) {
                authMode = authMode == AuthMode::Login ? AuthMode::Register : AuthMode::Login;
                authStatus.clear();
                username.clear();
                password.clear();
                authFocus = 0;
                return true;
            }
            if (event == Event::Tab || event == Event::ArrowUp || event == Event::ArrowDown) {
                authFocus = (authFocus + 1) % 2;
                return true;
            }
            if (event == Event::Return) {
                if (username.empty()) {
                    authFocus = 0;
                    return true;
                }
                if (password.empty()) {
                    authFocus = 1;
                    return true;
                }
                if (authMode == AuthMode::Login) {
                    if (loginInstance.login(username, password)) {
                        authStatus.clear();
                        refreshSession();
                    } else {
                        authStatus = tr("ui.user_info.auth_login_failed");
                        username.clear();
                        password.clear();
                        authFocus = 0;
                    }
                } else {
                    std::string err;
                    if (loginInstance.registerUser(username, password, &err)) {
                        authMode = AuthMode::Login;
                        authStatus.clear();
                        username.clear();
                        password.clear();
                        authFocus = 0;
                    } else {
                        authStatus = err.empty() ? tr("ui.login.result.register_failed") : err;
                    }
                }
                return true;
            }
            if (authFocus == 0) return usernameInput->OnEvent(event);
            return passwordInput->OnEvent(event);
        }
        // Logout
        if (event == Event::Character('L')) {
            loginInstance.logout();
            refreshSession();
            return true;
        }
        // Logged-in user tab switching
        if (event == Event::Tab) {
            activeTab = (activeTab + 1) % 2;
            historyScrollOffset = 0;
            return true;
        }
        // History scrolling
        if (activeTab == 1) {
            const auto &records = userStats.records();
            const int total = static_cast<int>(records.order.size());
            if (event == Event::Character('j') || event == Event::ArrowDown) {
                historyScrollOffset = std::min(historyScrollOffset + 1, std::max(0, total - 1));
                return true;
            }
            if (event == Event::Character('k') || event == Event::ArrowUp) {
                historyScrollOffset = std::max(historyScrollOffset - 1, 0);
                return true;
            }
        }
        return false;
    });
    screen.Loop(app);
    return 0;
}
} // namespace ui
