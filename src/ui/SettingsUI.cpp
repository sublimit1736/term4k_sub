#include "ui/SettingsUI.h"

#include "utils/RuntimeConfigs.h"
#include "entities/SettingsDraft.h"
#include "models/SettingsInstance.h"
#include "services/AuthenticatedUserService.h"
#include "services/I18nService.h"
#include "services/SettingsService.h"
#include "ui/MessageOverlay.h"
#include "ui/TransitionBackdrop.h"
#include "ui/UIColors.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <memory>
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


ftxui::Component SettingsUI::component(
    std::function<void(UIScene)> onRoute
    ) {
    using namespace ftxui;

    I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
    auto tr = [](const std::string &key) { return I18nService::instance().get(key); };

    struct SettingsState {
        ThemePalette palette;
        SettingsInstance instance;
        SettingsDraft draft;
        SettingsDraft committed;
        std::vector<std::string> themeIds;
        int tabIndex = 0;
        int rowIndex = 0;
        std::string status;
        bool showKeyEditor = false;
        int keyEditorLane = 0;
        bool keyCaptureMode = false;
        std::string cachedConflictWarning;
        bool conflictWarningDirty = true;
    };

    auto state = std::make_shared<SettingsState>();
    state->palette = ThemeAdapter::resolveFromRuntime();
    state->instance.loadFromRuntime();
    state->draft = state->instance.draft();
    state->committed = state->draft;
    state->themeIds = state->instance.availableThemeIds();
    if (state->themeIds.empty()) {
        state->themeIds.push_back(state->draft.getTheme().empty() ? std::string("tomorrow-night") : state->draft.getTheme());
    }
    state->status = tr("ui.settings.action.idle");

    const std::array<std::string, 2> locales = {"zh_CN", "en_US"};
    const std::array<uint32_t, 8> bufferOptions = {64, 128, 256, 512, 1024, 2048, 4096, 8192};

    // Helper: compute a summary of duplicate-key conflicts.
    // Returns empty string when no conflicts exist.
    auto computeConflictWarning = [state]() -> std::string {
        const std::vector<uint8_t> &bindings = state->draft.getKeyBindings();
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

    auto ensureTenKeySlots = [state] {
        std::vector<uint8_t> bindings = state->draft.getKeyBindings();
        const std::array<uint8_t, 10> laneDefaults = {'D', 'F', 'H', 'J', 0, 0, 0, 0, 0, 0};
        if (bindings.size() < 10) {
            bindings.resize(10);
            for (std::size_t i = 0; i < 10; ++i) {
                if (bindings[i] == 0) bindings[i] = laneDefaults[i];
            }
        }
        if (bindings.size() > 10) bindings.resize(10);
        state->draft.setKeyBindings(bindings);
    };
    // Ensure exactly 10 key slots once at startup; not repeated every render frame.
    ensureTenKeySlots();

    auto saveAndRefresh = [state, tr]() {
        state->instance.setDraft(state->draft);
        const auto user = AuthenticatedUserService::currentUser();
        const std::string username = user.has_value() ? user->getUsername() : "local";
        if (state->instance.saveDraftForUser(username)) {
            state->committed = state->draft;
            SettingsService::applyToRuntime(state->committed);
            I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
            state->palette = ThemeAdapter::resolveFromRuntime();
            state->status = tr("ui.settings.action.saved");
            return true;
        }
        state->status = tr("ui.settings.action.save_failed");
        return false;
    };

    auto clampRow = [state] {
        const int maxRowsByTab[3] = {2, 3, 8};
        state->rowIndex = std::clamp(state->rowIndex, 0, maxRowsByTab[state->tabIndex] - 1);
    };

    auto cycleTheme = [state](const int delta) {
        auto it = std::ranges::find(state->themeIds, state->draft.getTheme());
        int index = (it == state->themeIds.end()) ? 0 : static_cast<int>(std::distance(state->themeIds.begin(), it));
        index = (index + delta + static_cast<int>(state->themeIds.size())) % static_cast<int>(state->themeIds.size());
        state->draft.setTheme(state->themeIds[static_cast<std::size_t>(index)]);
    };

    auto cycleLocale = [state, locales](const int delta) {
        int index = 0;
        for (std::size_t i = 0; i < locales.size(); ++i) {
            if (state->draft.getLocale() == locales[i]) index = static_cast<int>(i);
        }
        index = (index + delta + static_cast<int>(locales.size())) % static_cast<int>(locales.size());
        state->draft.setLocale(locales[static_cast<std::size_t>(index)]);
    };

    auto cycleBuffer = [state, bufferOptions](const int delta) {
        int index = 0;
        for (std::size_t i = 0; i < bufferOptions.size(); ++i) {
            if (state->draft.getAudioBufferSize() == bufferOptions[i]) index = static_cast<int>(i);
        }
        index = (index + delta + static_cast<int>(bufferOptions.size())) % static_cast<int>(bufferOptions.size());
        state->draft.setAudioBufferSize(bufferOptions[static_cast<std::size_t>(index)]);
    };

    auto modifyCurrentField = [state, cycleTheme, cycleLocale, cycleBuffer](const int delta, const bool fineAdjust) {
        if (state->tabIndex == 0) {
            if (state->rowIndex == 0) cycleTheme(delta);
            if (state->rowIndex == 1) cycleLocale(delta);
            return;
        }

        if (state->tabIndex == 1) {
            const float volumeStep = fineAdjust ? 0.01f : 0.05f;
            if (state->rowIndex == 0) state->draft.setMusicVolume(std::clamp(state->draft.getMusicVolume() + volumeStep * static_cast<float>(delta), 0.0f, 1.0f));
            if (state->rowIndex == 1) state->draft.setHitSoundVolume(std::clamp(state->draft.getHitSoundVolume() + volumeStep * static_cast<float>(delta), 0.0f, 1.0f));
            if (state->rowIndex == 2) cycleBuffer(delta);
            return;
        }

        if (state->rowIndex == 0) state->draft.setShowEarlyLate(!state->draft.isShowEarlyLate());
        if (state->rowIndex == 1) state->draft.setShowAPIndicator(!state->draft.isShowAPIndicator());
        if (state->rowIndex == 2) state->draft.setShowFCIndicator(!state->draft.isShowFCIndicator());
        const int offsetStep = fineAdjust ? 1 : 5;
        if (state->rowIndex == 3) state->draft.setChartOffsetMs(state->draft.getChartOffsetMs() + delta * offsetStep);
        if (state->rowIndex == 4) state->draft.setChartDisplayOffsetMs(state->draft.getChartDisplayOffsetMs() + delta * offsetStep);
        if (state->rowIndex == 5) {
            const int preloadStep = fineAdjust ? 10 : 100;
            const int preload = static_cast<int>(state->draft.getChartPreloadMs()) + delta * preloadStep;
            state->draft.setChartPreloadMs(static_cast<uint32_t>(std::clamp(preload, 0, 60000)));
        }
        if (state->rowIndex == 6) {
            state->draft.setChartEndTimingMode(state->draft.getChartEndTimingMode() == ChartEndTimingMode::AfterAudioEnd
                                            ? ChartEndTimingMode::AfterChartEnd
                                            : ChartEndTimingMode::AfterAudioEnd);
        }
    };

    auto row = [state](const std::string &label, const std::string &value, const bool selected) -> Element {
        if (selected) {
            // When highlighted: apply bold + high-contrast fg + accent bg to the whole row,
            // without per-element colour overrides so every part stays readable.
            return hbox({
                text(label),
                filler(),
                text(value) | bold,
            }) | bold | color(highContrastOn(state->palette.accentPrimary)) | bgcolor(toColor(state->palette.accentPrimary));
        }
        return hbox({
            text(label) | color(toColor(state->palette.textMuted)),
            filler(),
            text(value) | bold | color(toColor(state->palette.textPrimary)),
        });
    };

    auto root = Renderer([state, tr, row, computeConflictWarning] {
        const bool dirty = (state->draft != state->committed);
        const std::array<std::string, 3> tabs = {
            tr("ui.settings.tab.appearance"),
            tr("ui.settings.tab.audio"),
            tr("ui.settings.tab.gameplay"),
        };

        // ensureTenKeySlots is called once at startup; not repeated every frame.
        const std::vector<uint8_t> &bindings = state->draft.getKeyBindings();

        Elements tabElements;
        for (int i = 0; i < static_cast<int>(tabs.size()); ++i) {
            Element t = text(" " + tabs[static_cast<std::size_t>(i)] + " ") | bold;
            if (i == state->tabIndex) {
                t = t | color(highContrastOn(state->palette.accentPrimary)) | bgcolor(toColor(state->palette.accentPrimary));
            } else {
                t = t | color(toColor(state->palette.textMuted));
            }
            tabElements.push_back(t);
            tabElements.push_back(text(" "));
        }

        Elements logicalRows;
        if (state->tabIndex == 0) {
            logicalRows.push_back(row(tr("ui.settings.field.theme"), state->draft.getTheme(), state->rowIndex == 0));
            logicalRows.push_back(row(tr("ui.settings.field.locale"), state->draft.getLocale(), state->rowIndex == 1));
        } else if (state->tabIndex == 1) {
            logicalRows.push_back(row(tr("ui.settings.field.music_volume"), std::to_string(static_cast<int>(state->draft.getMusicVolume() * 100.0f)) + "%", state->rowIndex == 0));
            logicalRows.push_back(row(tr("ui.settings.field.hit_volume"), std::to_string(static_cast<int>(state->draft.getHitSoundVolume() * 100.0f)) + "%", state->rowIndex == 1));
            logicalRows.push_back(row(tr("ui.settings.field.buffer_size"), std::to_string(state->draft.getAudioBufferSize()), state->rowIndex == 2));
        } else {
            logicalRows.push_back(row(tr("ui.settings.field.show_early_late"), boolLabel(state->draft.isShowEarlyLate(), tr("ui.settings.value.on"), tr("ui.settings.value.off")), state->rowIndex == 0));
            logicalRows.push_back(row(tr("ui.settings.field.show_ap"), boolLabel(state->draft.isShowAPIndicator(), tr("ui.settings.value.on"), tr("ui.settings.value.off")), state->rowIndex == 1));
            logicalRows.push_back(row(tr("ui.settings.field.show_fc"), boolLabel(state->draft.isShowFCIndicator(), tr("ui.settings.value.on"), tr("ui.settings.value.off")), state->rowIndex == 2));
            logicalRows.push_back(row(tr("ui.settings.field.chart_offset"), std::to_string(state->draft.getChartOffsetMs()), state->rowIndex == 3));
            logicalRows.push_back(row(tr("ui.settings.field.chart_display_offset"), std::to_string(state->draft.getChartDisplayOffsetMs()), state->rowIndex == 4));
            logicalRows.push_back(row(tr("ui.settings.field.chart_preload"), std::to_string(state->draft.getChartPreloadMs()), state->rowIndex == 5));
            logicalRows.push_back(row(tr("ui.settings.field.end_timing"),
                                     state->draft.getChartEndTimingMode() == ChartEndTimingMode::AfterAudioEnd
                                         ? tr("ui.settings.value.after_audio_end")
                                         : tr("ui.settings.value.after_chart_end"),
                                     state->rowIndex == 6));
            logicalRows.push_back(row(tr("ui.settings.field.key_bindings"), bindingSummary(bindings), state->rowIndex == 7));
        }

        Elements rowsWithGap;
        for (std::size_t i = 0; i < logicalRows.size(); ++i) {
            rowsWithGap.push_back(logicalRows[i]);
            if (i + 1 < logicalRows.size()) rowsWithGap.push_back(text(""));
        }

        Element panel = vbox(std::move(rowsWithGap)) |
                        borderRounded |
                        color(toColor(state->palette.borderNormal)) |
                        bgcolor(toColor(state->palette.surfacePanel));

        Element top = hbox({
            text(tr("ui.settings.title")) | bold | color(toColor(state->palette.accentPrimary)),
            filler(),
            text(dirty ? tr("ui.settings.unsaved") : "") | color(toColor(state->palette.textMuted)),
            text("  "),
            text(tr("ui.settings.hint")) | color(toColor(state->palette.textMuted)),
        });

        Element body = vbox({separator(), panel | flex});

        if (state->showKeyEditor) {
            // Recompute conflict warning only when bindings changed (dirty flag).
            if (state->conflictWarningDirty) {
                state->cachedConflictWarning = computeConflictWarning();
                state->conflictWarningDirty = false;
            }

            Elements keyRows;
            for (int lane = 0; lane < 10; ++lane) {
                const std::string laneLabel = tr("ui.settings.field.key_slot") + std::to_string(lane);
                const std::string value(1, keyCodeToDisplay(bindings[static_cast<std::size_t>(lane)]));
                Element r = hbox({text(laneLabel), filler(), text(value) | bold});
                if (lane == state->keyEditorLane) {
                    r = r | color(highContrastOn(state->palette.accentPrimary)) | bgcolor(toColor(state->palette.accentPrimary));
                } else {
                    r = r | color(toColor(state->palette.textPrimary));
                }
                keyRows.push_back(r);
                if (lane < 9) keyRows.push_back(text(""));
            }

            Elements editorRows = {
                text(tr("ui.settings.key_editor.title")) | bold | color(toColor(state->palette.accentPrimary)),
                text(state->keyCaptureMode ? tr("ui.settings.key_editor.capturing") : tr("ui.settings.key_editor.ready")) |
                    color(toColor(state->palette.textMuted)),
            };
            if (!state->cachedConflictWarning.empty()) {
                editorRows.push_back(
                    text(tr("ui.settings.key_editor.conflict_prefix") + state->cachedConflictWarning) | color(Color::Red)
                );
            }
            editorRows.push_back(separator());
            editorRows.push_back(vbox(std::move(keyRows)));

            Element editorBody = vbox(std::move(editorRows));

            Element popup = window(text(" " + tr("ui.settings.field.key_bindings") + " "), editorBody) |
                            color(toColor(state->palette.accentPrimary)) |
                            bgcolor(toColor(state->palette.surfacePanel)) |
                            size(WIDTH, EQUAL, 56);

            body = dbox({body, hbox({filler(), popup, filler()}) | vcenter | flex});
        }

        Element bottom = text(state->status) | color(toColor(state->palette.textMuted));

        Element base = vbox({
                   top,
                   separator(),
                   hbox(std::move(tabElements)),
                   body | flex,
                   separator(),
                   bottom,
               }) |
               bgcolor(toColor(state->palette.surfaceBg)) |
               color(toColor(state->palette.textPrimary)) |
               flex;

        Element composed = dbox({base, MessageOverlay::render(state->palette)});
        TransitionBackdrop::update(composed);
        return composed;
    });

    auto app = CatchEvent(root, [state,
                                 saveAndRefresh,
                                 clampRow,
                                 modifyCurrentField,
                                 onRoute = std::move(onRoute),
                                 tr](const Event &event) {
        if (MessageOverlay::handleEvent(event)) {
            return true;
        }

        if (state->showKeyEditor) {
            if (state->keyCaptureMode) {
                char captured = 0;
                if (isBindableCharacter(event, &captured)) {
                    std::vector<uint8_t> bindings = state->draft.getKeyBindings();
                    bindings[static_cast<std::size_t>(state->keyEditorLane)] = static_cast<uint8_t>(captured);
                    state->draft.setKeyBindings(bindings);
                    state->keyCaptureMode = false;
                    state->conflictWarningDirty = true;
                    return true;
                }
                if (event == Event::Backspace) {
                    std::vector<uint8_t> bindings = state->draft.getKeyBindings();
                    bindings[static_cast<std::size_t>(state->keyEditorLane)] = 0;
                    state->draft.setKeyBindings(bindings);
                    state->keyCaptureMode = false;
                    state->conflictWarningDirty = true;
                    return true;
                }
                if (event == Event::Escape) {
                    state->keyCaptureMode = false;
                    return true;
                }
                return true;
            }

            if (event == Event::Escape) {
                state->showKeyEditor = false;
                return true;
            }
            if (event == Event::ArrowUp || event == Event::Character('k')) {
                state->keyEditorLane = (state->keyEditorLane + 9) % 10;
                return true;
            }
            if (event == Event::ArrowDown || event == Event::Character('j')) {
                state->keyEditorLane = (state->keyEditorLane + 1) % 10;
                return true;
            }
            if (event == Event::Return) {
                state->keyCaptureMode = true;
                return true;
            }
            if (event == Event::Character('s') || event == Event::Character('S')) {
                saveAndRefresh();
                state->showKeyEditor = false;
                return true;
            }
            return true;
        }

        if (event == Event::Escape || event == Event::Character('q')) {
            onRoute(UIScene::StartMenu);
            return true;
        }
        if (event == Event::Tab || event == Event::Character('l') || event == Event::ArrowRight) {
            state->tabIndex = (state->tabIndex + 1) % 3;
            clampRow();
            return true;
        }
        if (event == Event::Character('h') || event == Event::ArrowLeft) {
            state->tabIndex = (state->tabIndex + 2) % 3;
            clampRow();
            return true;
        }
        if (event == Event::ArrowUp || event == Event::Character('k')) {
            --state->rowIndex;
            clampRow();
            return true;
        }
        if (event == Event::ArrowDown || event == Event::Character('j')) {
            ++state->rowIndex;
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
            if (state->tabIndex == 0 && state->rowIndex == 0) state->status = tr("ui.settings.desc.theme");
            if (state->tabIndex == 0 && state->rowIndex == 1) state->status = tr("ui.settings.desc.locale");
            if (state->tabIndex == 1 && state->rowIndex == 0) state->status = tr("ui.settings.desc.music_volume");
            if (state->tabIndex == 1 && state->rowIndex == 1) state->status = tr("ui.settings.desc.hit_volume");
            if (state->tabIndex == 1 && state->rowIndex == 2) state->status = tr("ui.settings.desc.buffer_size");
            if (state->tabIndex == 2 && state->rowIndex == 0) state->status = tr("ui.settings.desc.show_early_late");
            if (state->tabIndex == 2 && state->rowIndex == 1) state->status = tr("ui.settings.desc.show_ap");
            if (state->tabIndex == 2 && state->rowIndex == 2) state->status = tr("ui.settings.desc.show_fc");
            if (state->tabIndex == 2 && state->rowIndex == 3) state->status = tr("ui.settings.desc.chart_offset");
            if (state->tabIndex == 2 && state->rowIndex == 4) state->status = tr("ui.settings.desc.chart_display_offset");
            if (state->tabIndex == 2 && state->rowIndex == 5) state->status = tr("ui.settings.desc.chart_preload");
            if (state->tabIndex == 2 && state->rowIndex == 6) state->status = tr("ui.settings.desc.end_timing");
            if (state->tabIndex == 2 && state->rowIndex == 7) state->status = tr("ui.settings.desc.key_bindings");

            if (state->tabIndex == 2 && state->rowIndex == 7) {
                state->showKeyEditor = true;
                state->keyEditorLane = 0;
                state->keyCaptureMode = false;
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

    return app;
}

} // namespace ui
