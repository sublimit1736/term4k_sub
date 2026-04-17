#include "ui/RegisterUI.h"

#include "config/RuntimeConfigs.h"
#include "instances/UserLoginInstance.h"
#include "services/I18nService.h"
#include "ui/UIColors.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <string>

namespace ui {

int RegisterUI::run() {
    auto screen = ftxui::ScreenInteractive::Fullscreen();
    return run(screen);
}

int RegisterUI::run(ftxui::ScreenInteractive &screen) {
    using namespace ftxui;

    I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
    auto tr = [](const std::string &key) { return I18nService::instance().get(key); };
    const ThemePalette palette = ThemeAdapter::resolveFromRuntime();

    std::string username;
    std::string password;
    std::string status = tr("ui.auth.status.idle");

    InputOption userOpt;
    userOpt.placeholder = tr("ui.login.username");
    userOpt.transform = [&](const InputState &state) {
        return state.element |
               color(toColor(state.is_placeholder ? palette.textMuted : palette.textPrimary)) |
               bgcolor(toColor(palette.surfacePanel));
    };
    auto usernameInput = Input(&username, userOpt);

    InputOption passOpt;
    passOpt.password = true;
    passOpt.placeholder = tr("ui.login.password");
    passOpt.transform = [&](const InputState &state) {
        return state.element |
               color(toColor(state.is_placeholder ? palette.textMuted : palette.textPrimary)) |
               bgcolor(toColor(palette.surfacePanel));
    };
    auto passwordInput = Input(&password, passOpt);

    auto container = Container::Vertical({usernameInput, passwordInput});
    int focusIndex = 0;

    auto root = Renderer(container, [&] {
        Element user = window(text(" " + tr("ui.login.username") + " "), usernameInput->Render()) |
                       size(WIDTH, GREATER_THAN, 30) |
                       color(toColor(focusIndex == 0 ? palette.accentPrimary : palette.borderNormal)) |
                       bgcolor(toColor(palette.surfacePanel));
        Element pass = window(text(" " + tr("ui.login.password") + " "), passwordInput->Render()) |
                       size(WIDTH, GREATER_THAN, 30) |
                       color(toColor(focusIndex == 1 ? palette.accentPrimary : palette.borderNormal)) |
                       bgcolor(toColor(palette.surfacePanel));

        Element hint = text(tr("ui.register.hint")) | color(toColor(palette.textMuted));
        Element statusLine = text(status) | color(toColor(palette.textMuted));

        Element submit = text(" " + tr("ui.auth.action.register") + " ") |
                         bold |
                         color(highContrastOn(palette.accentPrimary)) |
                         bgcolor(toColor(palette.accentPrimary));

        return vbox({
                   text(tr("ui.auth.register_title")) | bold | color(toColor(palette.accentPrimary)),
                   separator(),
                   user,
                   pass,
                   text(""),
                   hbox({submit, text("  "), hint}),
                   separator(),
                   statusLine,
               }) |
               borderRounded |
               bgcolor(toColor(palette.surfaceBg)) |
               color(toColor(palette.textPrimary)) |
               center;
    });

    UserLoginInstance loginInstance;
    auto app = CatchEvent(root, [&](const Event &event) {
        if (event == Event::Escape || event == Event::Character('q')) {
            screen.ExitLoopClosure()();
            return true;
        }
        if (event == Event::Tab) {
            focusIndex = (focusIndex + 1) % 2;
            return true;
        }
        if (event == Event::ArrowUp || event == Event::Character('k')) {
            focusIndex = (focusIndex + 1) % 2;
            return true;
        }
        if (event == Event::ArrowDown || event == Event::Character('j')) {
            focusIndex = (focusIndex + 1) % 2;
            return true;
        }
        if (event == Event::Return) {
            std::string err;
            if (loginInstance.registerUser(username, password, &err)) {
                status = tr("ui.login.result.register_succeeded");
            } else {
                status = err.empty() ? tr("ui.login.result.register_failed") : err;
            }
            return true;
        }
        if (focusIndex == 0) return usernameInput->OnEvent(event);
        return passwordInput->OnEvent(event);
    });

    screen.Loop(app);
    return 0;
}

} // namespace ui
