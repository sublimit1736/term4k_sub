#include "ui/SettingsUI.h"

#include "config/RuntimeConfigs.h"
#include "entities/SettingsDraft.h"
#include "instances/SettingsInstance.h"
#include "services/AuthenticatedUserService.h"
#include "services/I18nService.h"
#include "services/SettingsService.h"
#include "ui/UIColors.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

namespace ui {
namespace {


std::string boolLabel(const bool value, const std::string &onText, const std::string &offText) {
    return value ? onText : offText;
}

char keyCodeToDisplay(const uint8_t key) {
    if (key == 0) return '-';
    if (key >= 'a' && key <= 'z') return static_cast<char>(key - 'a' + 'A');
    if (key >= 'A' && key <= 'Z') return static_cast<char>(key);
    if (key >= '0' && key <= '9') return static_cast<char>(key);
    if (std::isprint(key)) return static_cast<char>(key);
    return '?';
}

std::string bindingSummary(const std::vector<uint8_t> &bindings) {
    std::ostringstream out;
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        if (i != 0) out << ',';
        out << keyCodeToDisplay(bindings[i]);
    }
    return out.str();
}

bool isBindableCharacter(const ftxui::Event &event, char *out) {
    if (!event.is_character()) return false;
    const std::string value = event.character();
    if (value.size() != 1) return false;
    const auto ch = static_cast<unsigned char>(value[0]);
    if (!std::isprint(ch)) return false;
    if (ch == 27 || ch == '\r' || ch == '\n' || ch == '\t') return false;
    *out = static_cast<char>(ch);
    return true;
}

} // namespace

int SettingsUI::run() {
    using namespace ftxui;

    I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
    auto tr = [](const std::string &key) { return I18nService::instance().get(key); };

    ThemePalette palette = ThemeAdapter::resolveFromRuntime();
    SettingsInstance instance;
    instance.loadFromRuntime();

    SettingsDraft draft = instance.draft();
    SettingsDraft committed = draft;

    std::vector<std::string> themeIds = instance.availableThemeIds();
    if (themeIds.empty()) {
        themeIds.push_back(draft.getTheme().empty() ? std::string("tomorrow-night") : draft.getTheme());
    }

    const std::array<std::string, 2> locales = {"zh_CN", "en_US"};
    const std::array<uint32_t, 8> bufferOptions = {64, 128, 256, 512, 1024, 2048, 4096, 8192};

    int tabIndex = 0;
    int rowIndex = 0;
    std::string status = tr("ui.settings.action.idle");

    bool showKeyEditor = false;
    int keyEditorLane = 0;
    bool keyCaptureMode = false;

    // Helper: compute a summary of duplicate-key conflicts.
    // Returns empty string when no conflicts exist.
    auto computeConflictWarning = [&]() -> std::string {
        const std::vector<uint8_t> &bindings = draft.getKeyBindings();
        std::string result;
        for (std::size_t i = 0; i < bindings.size(); ++i) {
            if (bindings[i] == 0) continue;
            // Skip keys already described in an earlier iteration.
            bool alreadySeen = false;
            for (std::size_t k = 0; k < i; ++k) {
                if (bindings[k] == bindings[i]) { alreadySeen = true; break; }
            }
            if (alreadySeen) continue;
            // Collect all lanes sharing this key.
            std::string lanes;
            int count = 0;
            for (std::size_t m = i; m < bindings.size(); ++m) {
                if (bindings[m] == bindings[i]) {
                    if (!lanes.empty()) lanes += ",";
                    lanes += std::to_string(m);
                    ++count;
                }
            }
            if (count >= 2) {
                if (!result.empty()) result += " | ";
                result += std::string(1, keyCodeToDisplay(bindings[i]));
                result += ":";
                result += lanes;
            }
        }
        return result;
    };

    auto ensureTenKeySlots = [&] {
        std::vector<uint8_t> bindings = draft.getKeyBindings();
        const std::array<uint8_t, 10> laneDefaults = {'D', 'F', 'H', 'J', 0, 0, 0, 0, 0, 0};
        if (bindings.size() < 10) {
            bindings.resize(10);
            for (std::size_t i = 0; i < 10; ++i) {
                if (bindings[i] == 0) bindings[i] = laneDefaults[i];
            }
        }
        if (bindings.size() > 10) bindings.resize(10);
        draft.setKeyBindings(bindings);
    };
    // Ensure exactly 10 key slots once at startup; not repeated every render frame.
    ensureTenKeySlots();

    // Conflict-warning cache: recomputed only when key bindings change.
    std::string cachedConflictWarning;
    bool conflictWarningDirty = true;

    auto saveAndRefresh = [&]() {
        instance.setDraft(draft);
        const auto user = AuthenticatedUserService::currentUser();
        const std::string username = user.has_value() ? user->getUsername() : "local";
        if (instance.saveDraftForUser(username)) {
            committed = draft;
            SettingsService::applyToRuntime(committed);
            I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
            palette = ThemeAdapter::resolveFromRuntime();
            status = tr("ui.settings.action.saved");
            return true;
        }
        status = tr("ui.settings.action.save_failed");
        return false;
    };

    auto clampRow = [&] {
        const int maxRowsByTab[3] = {2, 3, 8};
        rowIndex = std::clamp(rowIndex, 0, maxRowsByTab[tabIndex] - 1);
    };

    auto cycleTheme = [&](const int delta) {
        auto it = std::find(themeIds.begin(), themeIds.end(), draft.getTheme());
        int index = (it == themeIds.end()) ? 0 : static_cast<int>(std::distance(themeIds.begin(), it));
        index = (index + delta + static_cast<int>(themeIds.size())) % static_cast<int>(themeIds.size());
        draft.setTheme(themeIds[static_cast<std::size_t>(index)]);
    };

    auto cycleLocale = [&](const int delta) {
        int index = 0;
        for (std::size_t i = 0; i < locales.size(); ++i) {
            if (draft.getLocale() == locales[i]) index = static_cast<int>(i);
        }
        index = (index + delta + static_cast<int>(locales.size())) % static_cast<int>(locales.size());
        draft.setLocale(locales[static_cast<std::size_t>(index)]);
    };

    auto cycleBuffer = [&](const int delta) {
        int index = 0;
        for (std::size_t i = 0; i < bufferOptions.size(); ++i) {
            if (draft.getAudioBufferSize() == bufferOptions[i]) index = static_cast<int>(i);
        }
        index = (index + delta + static_cast<int>(bufferOptions.size())) % static_cast<int>(bufferOptions.size());
        draft.setAudioBufferSize(bufferOptions[static_cast<std::size_t>(index)]);
    };

    auto modifyCurrentField = [&](const int delta, const bool fineAdjust) {
        if (tabIndex == 0) {
            if (rowIndex == 0) cycleTheme(delta);
            if (rowIndex == 1) cycleLocale(delta);
            return;
        }

        if (tabIndex == 1) {
            const float volumeStep = fineAdjust ? 0.01f : 0.05f;
            if (rowIndex == 0) draft.setMusicVolume(std::clamp(draft.getMusicVolume() + volumeStep * static_cast<float>(delta), 0.0f, 1.0f));
            if (rowIndex == 1) draft.setHitSoundVolume(std::clamp(draft.getHitSoundVolume() + volumeStep * static_cast<float>(delta), 0.0f, 1.0f));
            if (rowIndex == 2) cycleBuffer(delta);
            return;
        }

        if (rowIndex == 0) draft.setShowEarlyLate(!draft.isShowEarlyLate());
        if (rowIndex == 1) draft.setShowAPIndicator(!draft.isShowAPIndicator());
        if (rowIndex == 2) draft.setShowFCIndicator(!draft.isShowFCIndicator());
        const int offsetStep = fineAdjust ? 1 : 5;
        if (rowIndex == 3) draft.setChartOffsetMs(draft.getChartOffsetMs() + delta * offsetStep);
        if (rowIndex == 4) draft.setChartDisplayOffsetMs(draft.getChartDisplayOffsetMs() + delta * offsetStep);
        if (rowIndex == 5) {
            const int preloadStep = fineAdjust ? 10 : 100;
            const int preload = static_cast<int>(draft.getChartPreloadMs()) + delta * preloadStep;
            draft.setChartPreloadMs(static_cast<uint32_t>(std::clamp(preload, 0, 60000)));
        }
        if (rowIndex == 6) {
            draft.setChartEndTimingMode(draft.getChartEndTimingMode() == ChartEndTimingMode::AfterAudioEnd
                                            ? ChartEndTimingMode::AfterChartEnd
                                            : ChartEndTimingMode::AfterAudioEnd);
        }
    };

    auto row = [&](const std::string &label, const std::string &value, const bool selected) -> Element {
        if (selected) {
            // When highlighted: apply bold + high-contrast fg + accent bg to the whole row,
            // without per-element colour overrides so every part stays readable.
            return hbox({
                text(label),
                filler(),
                text(value) | bold,
            }) | bold | color(highContrastOn(palette.accentPrimary)) | bgcolor(toColor(palette.accentPrimary));
        }
        return hbox({
            text(label) | color(toColor(palette.textMuted)),
            filler(),
            text(value) | bold | color(toColor(palette.textPrimary)),
        });
    };

    auto screen = ScreenInteractive::Fullscreen();
    auto root = Renderer([&] {
        const bool dirty = (draft != committed);
        const std::array<std::string, 3> tabs = {
            tr("ui.settings.tab.appearance"),
            tr("ui.settings.tab.audio"),
            tr("ui.settings.tab.gameplay"),
        };

        // ensureTenKeySlots is called once at startup; not repeated every frame.
        const std::vector<uint8_t> &bindings = draft.getKeyBindings();

        Elements tabElements;
        for (int i = 0; i < static_cast<int>(tabs.size()); ++i) {
            Element t = text(" " + tabs[static_cast<std::size_t>(i)] + " ") | bold;
            if (i == tabIndex) {
                t = t | color(highContrastOn(palette.accentPrimary)) | bgcolor(toColor(palette.accentPrimary));
            } else {
                t = t | color(toColor(palette.textMuted));
            }
            tabElements.push_back(t);
            tabElements.push_back(text(" "));
        }

        Elements logicalRows;
        if (tabIndex == 0) {
            logicalRows.push_back(row(tr("ui.settings.field.theme"), draft.getTheme(), rowIndex == 0));
            logicalRows.push_back(row(tr("ui.settings.field.locale"), draft.getLocale(), rowIndex == 1));
        } else if (tabIndex == 1) {
            logicalRows.push_back(row(tr("ui.settings.field.music_volume"), std::to_string(static_cast<int>(draft.getMusicVolume() * 100.0f)) + "%", rowIndex == 0));
            logicalRows.push_back(row(tr("ui.settings.field.hit_volume"), std::to_string(static_cast<int>(draft.getHitSoundVolume() * 100.0f)) + "%", rowIndex == 1));
            logicalRows.push_back(row(tr("ui.settings.field.buffer_size"), std::to_string(draft.getAudioBufferSize()), rowIndex == 2));
        } else {
            logicalRows.push_back(row(tr("ui.settings.field.show_early_late"), boolLabel(draft.isShowEarlyLate(), tr("ui.settings.value.on"), tr("ui.settings.value.off")), rowIndex == 0));
            logicalRows.push_back(row(tr("ui.settings.field.show_ap"), boolLabel(draft.isShowAPIndicator(), tr("ui.settings.value.on"), tr("ui.settings.value.off")), rowIndex == 1));
            logicalRows.push_back(row(tr("ui.settings.field.show_fc"), boolLabel(draft.isShowFCIndicator(), tr("ui.settings.value.on"), tr("ui.settings.value.off")), rowIndex == 2));
            logicalRows.push_back(row(tr("ui.settings.field.chart_offset"), std::to_string(draft.getChartOffsetMs()), rowIndex == 3));
            logicalRows.push_back(row(tr("ui.settings.field.chart_display_offset"), std::to_string(draft.getChartDisplayOffsetMs()), rowIndex == 4));
            logicalRows.push_back(row(tr("ui.settings.field.chart_preload"), std::to_string(draft.getChartPreloadMs()), rowIndex == 5));
            logicalRows.push_back(row(tr("ui.settings.field.end_timing"),
                                     draft.getChartEndTimingMode() == ChartEndTimingMode::AfterAudioEnd
                                         ? tr("ui.settings.value.after_audio_end")
                                         : tr("ui.settings.value.after_chart_end"),
                                     rowIndex == 6));
            logicalRows.push_back(row(tr("ui.settings.field.key_bindings"), bindingSummary(bindings), rowIndex == 7));
        }

        Elements rowsWithGap;
        for (std::size_t i = 0; i < logicalRows.size(); ++i) {
            rowsWithGap.push_back(logicalRows[i]);
            if (i + 1 < logicalRows.size()) rowsWithGap.push_back(text(""));
        }

        Element panel = vbox(std::move(rowsWithGap)) |
                        borderRounded |
                        color(toColor(palette.borderNormal)) |
                        bgcolor(toColor(palette.surfacePanel));

        Element top = hbox({
            text(tr("ui.settings.title")) | bold | color(toColor(palette.accentPrimary)),
            filler(),
            text(dirty ? tr("ui.settings.unsaved") : "") | color(toColor(palette.textMuted)),
            text("  "),
            text(tr("ui.settings.hint")) | color(toColor(palette.textMuted)),
        });

        Element body = vbox({separator(), panel | flex});

        if (showKeyEditor) {
            // Recompute conflict warning only when bindings changed (dirty flag).
            if (conflictWarningDirty) {
                cachedConflictWarning = computeConflictWarning();
                conflictWarningDirty = false;
            }

            Elements keyRows;
            for (int lane = 0; lane < 10; ++lane) {
                const std::string laneLabel = tr("ui.settings.field.key_slot") + std::to_string(lane);
                const std::string value(1, keyCodeToDisplay(bindings[static_cast<std::size_t>(lane)]));
                Element r = hbox({text(laneLabel), filler(), text(value) | bold});
                if (lane == keyEditorLane) {
                    r = r | color(highContrastOn(palette.accentPrimary)) | bgcolor(toColor(palette.accentPrimary));
                } else {
                    r = r | color(toColor(palette.textPrimary));
                }
                keyRows.push_back(r);
                if (lane < 9) keyRows.push_back(text(""));
            }

            Element editorBody = vbox({
                text(tr("ui.settings.key_editor.title")) | bold | color(toColor(palette.accentPrimary)),
                text(keyCaptureMode ? tr("ui.settings.key_editor.capturing") : tr("ui.settings.key_editor.ready")) |
                    color(toColor(palette.textMuted)),
                cachedConflictWarning.empty()
                    ? text("")
                    : text(tr("ui.settings.key_editor.conflict_prefix") + cachedConflictWarning) | color(Color::Red),
                separator(),
                vbox(std::move(keyRows)),
            });

            Element popup = window(text(" " + tr("ui.settings.field.key_bindings") + " "), editorBody) |
                            color(toColor(palette.accentPrimary)) |
                            bgcolor(toColor(palette.surfacePanel)) |
                            size(WIDTH, EQUAL, 56);

            body = dbox({body, hbox({filler(), popup, filler()}) | vcenter | flex});
        }

        Element bottom = text(status) | color(toColor(palette.textMuted));

        return vbox({
                   top,
                   separator(),
                   hbox(std::move(tabElements)),
                   body | flex,
                   separator(),
                   bottom,
               }) |
               bgcolor(toColor(palette.surfaceBg)) |
               color(toColor(palette.textPrimary)) |
               flex;
    });

    auto app = CatchEvent(root, [&](const Event &event) {
        if (showKeyEditor) {
            if (keyCaptureMode) {
                char captured = 0;
                if (isBindableCharacter(event, &captured)) {
                    std::vector<uint8_t> bindings = draft.getKeyBindings();
                    bindings[static_cast<std::size_t>(keyEditorLane)] = static_cast<uint8_t>(captured);
                    draft.setKeyBindings(bindings);
                    keyCaptureMode = false;
                    conflictWarningDirty = true;
                    return true;
                }
                if (event == Event::Backspace) {
                    std::vector<uint8_t> bindings = draft.getKeyBindings();
                    bindings[static_cast<std::size_t>(keyEditorLane)] = 0;
                    draft.setKeyBindings(bindings);
                    keyCaptureMode = false;
                    conflictWarningDirty = true;
                    return true;
                }
                if (event == Event::Escape) {
                    keyCaptureMode = false;
                    return true;
                }
                return true;
            }

            if (event == Event::Escape) {
                showKeyEditor = false;
                return true;
            }
            if (event == Event::ArrowUp || event == Event::Character('k')) {
                keyEditorLane = (keyEditorLane + 9) % 10;
                return true;
            }
            if (event == Event::ArrowDown || event == Event::Character('j')) {
                keyEditorLane = (keyEditorLane + 1) % 10;
                return true;
            }
            if (event == Event::Return) {
                keyCaptureMode = true;
                return true;
            }
            if (event == Event::Character('s') || event == Event::Character('S')) {
                saveAndRefresh();
                showKeyEditor = false;
                return true;
            }
            return true;
        }

        if (event == Event::Escape || event == Event::Character('q')) {
            screen.ExitLoopClosure()();
            return true;
        }
        if (event == Event::Tab || event == Event::Character('l') || event == Event::ArrowRight) {
            tabIndex = (tabIndex + 1) % 3;
            clampRow();
            return true;
        }
        if (event == Event::Character('h') || event == Event::ArrowLeft) {
            tabIndex = (tabIndex + 2) % 3;
            clampRow();
            return true;
        }
        if (event == Event::ArrowUp || event == Event::Character('k')) {
            --rowIndex;
            clampRow();
            return true;
        }
        if (event == Event::ArrowDown || event == Event::Character('j')) {
            ++rowIndex;
            clampRow();
            return true;
        }
        if (event == Event::Character('a')) {
            modifyCurrentField(-1, false);
            return true;
        }
        if (event == Event::Character('d')) {
            modifyCurrentField(1, false);
            return true;
        }
        if (event == Event::Character('A')) {
            modifyCurrentField(-1, true);
            return true;
        }
        if (event == Event::Character('D')) {
            modifyCurrentField(1, true);
            return true;
        }
        if (event == Event::Return) {
            if (tabIndex == 0 && rowIndex == 0) status = tr("ui.settings.desc.theme");
            if (tabIndex == 0 && rowIndex == 1) status = tr("ui.settings.desc.locale");
            if (tabIndex == 1 && rowIndex == 0) status = tr("ui.settings.desc.music_volume");
            if (tabIndex == 1 && rowIndex == 1) status = tr("ui.settings.desc.hit_volume");
            if (tabIndex == 1 && rowIndex == 2) status = tr("ui.settings.desc.buffer_size");
            if (tabIndex == 2 && rowIndex == 0) status = tr("ui.settings.desc.show_early_late");
            if (tabIndex == 2 && rowIndex == 1) status = tr("ui.settings.desc.show_ap");
            if (tabIndex == 2 && rowIndex == 2) status = tr("ui.settings.desc.show_fc");
            if (tabIndex == 2 && rowIndex == 3) status = tr("ui.settings.desc.chart_offset");
            if (tabIndex == 2 && rowIndex == 4) status = tr("ui.settings.desc.chart_display_offset");
            if (tabIndex == 2 && rowIndex == 5) status = tr("ui.settings.desc.chart_preload");
            if (tabIndex == 2 && rowIndex == 6) status = tr("ui.settings.desc.end_timing");
            if (tabIndex == 2 && rowIndex == 7) status = tr("ui.settings.desc.key_bindings");

            if (tabIndex == 2 && rowIndex == 7) {
                showKeyEditor = true;
                keyEditorLane = 0;
                keyCaptureMode = false;
                return true;
            }
            return true;
        }
        if (event == Event::Character('s') || event == Event::Character('S')) {
            saveAndRefresh();
            return true;
        }
        return false;
    });

    screen.Loop(app);
    return 0;
}

} // namespace ui


