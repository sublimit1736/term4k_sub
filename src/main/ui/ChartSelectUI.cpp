#include "ui/ChartSelectUI.h"

#include "config/AppDirs.h"
#include "config/RuntimeConfigs.h"
#include "dao/ProofedRecordsDAO.h"
#include "instances/ChartListInstance.h"
#include "services/I18nService.h"
#include "ui/ThemeAdapter.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace ui {
namespace {

ftxui::Color toColor(const Rgb &rgb) {
    return ftxui::Color::RGB(rgb.r, rgb.g, rgb.b);
}

std::string formatFloat(float value, int precision = 2) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

struct SearchModeOption {
    ChartSearchMode mode;
    std::string labelKey;
};

struct SortKeyOption {
    ChartListSortKey key;
    std::string ascLabelKey;
    std::string descLabelKey;
};

} // namespace

int ChartSelectUI::run() {
    using namespace ftxui;

    I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
    auto tr = [](const std::string &key) {
        return I18nService::instance().get(key);
    };

    ThemePalette palette = ThemeAdapter::resolveFromRuntime();

    AppDirs::init();
    ProofedRecordsDAO::setDataDir(AppDirs::dataDir());

    const char *uidEnv = std::getenv("TERM4K_UI_UID");
    const std::string statsUID = uidEnv ? uidEnv : "";

    ChartListInstance chartList;
    chartList.refresh(AppDirs::chartsDir(), statsUID);

    const std::array<SearchModeOption, 3> searchModes = {{
        {ChartSearchMode::DisplayName, "ui.chart_select.search_mode.display_name"},
        {ChartSearchMode::Artist, "ui.chart_select.search_mode.artist"},
        {ChartSearchMode::Charter, "ui.chart_select.search_mode.charter"},
    }};

    const std::array<SortKeyOption, 3> sortKeys = {{
        {ChartListSortKey::DisplayName,
         "ui.chart_select.sort.display_name_asc",
         "ui.chart_select.sort.display_name_desc"},
        {ChartListSortKey::Difficulty,
         "ui.chart_select.sort.difficulty_asc",
         "ui.chart_select.sort.difficulty_desc"},
        {ChartListSortKey::BestAccuracy,
         "ui.chart_select.sort.best_accuracy_asc",
         "ui.chart_select.sort.best_accuracy_desc"},
    }};

    std::size_t searchModeIndex = 0;
    std::size_t sortKeyIndex = 0;
    SortOrder sortOrder = SortOrder::Ascending;
    chartList.setSearchMode(searchModes[searchModeIndex].mode);
    chartList.sort(sortKeys[sortKeyIndex].key, sortOrder);

    std::string searchQuery;
    std::string appliedSearch;
    std::size_t selectedIndex = 0;
    bool focusSearch = true;
    std::string actionStatus = tr("ui.chart_select.action.idle");
    bool showStartConfirm = false;
    std::string pendingStartName;

    auto screen = ScreenInteractive::Fullscreen();
    auto searchInput = Input(&searchQuery, tr("ui.chart_select.search_placeholder"));
    auto container = Container::Vertical({searchInput});

    auto root = Renderer(container, [&] {
        if (appliedSearch != searchQuery) {
            chartList.setSearchQuery(searchQuery);
            appliedSearch = searchQuery;
            selectedIndex = 0;
        }

        const auto &ids = chartList.filteredOrderedChartIDs();
        if (!ids.empty()) selectedIndex = std::min(selectedIndex, ids.size() - 1);
        const bool hasSelection = !ids.empty();

        Elements listRows;
        if (!hasSelection) {
            listRows.push_back(text(tr("ui.chart_select.no_charts")) | color(toColor(palette.textMuted)));
        } else {
            for (std::size_t i = 0; i < ids.size(); ++i) {
                const auto it = chartList.items().find(ids[i]);
                std::string title = ids[i];
                if (it != chartList.items().end() && !it->second.chart.getDisplayName().empty()) {
                    title = it->second.chart.getDisplayName();
                }

                Element row = text(title);
                if (i == selectedIndex) {
                    row = row | bold | color(toColor(palette.surfaceBg)) | bgcolor(toColor(palette.accentPrimary));
                } else {
                    row = row | color(toColor(palette.textPrimary));
                }
                listRows.push_back(row);
            }
        }

        Element searchBox = vbox({
                               searchInput->Render() |
                                   color(toColor(searchQuery.empty() ? palette.textMuted : palette.textPrimary)),
                           });

        searchBox = window(text("  " + tr("ui.chart_select.search") + " "), searchBox) |
                    color(toColor(focusSearch ? palette.accentPrimary : palette.borderNormal)) |
                    bgcolor(toColor(palette.surfacePanel));

        Element listBox = vbox(std::move(listRows)) |
                          borderRounded |
                          color(toColor(!focusSearch ? palette.accentPrimary : palette.borderNormal)) |
                          bgcolor(toColor(palette.surfacePanel)) |
                          frame |
                          flex;

        Element leftColumn = vbox({
                                searchBox,
                                listBox,
                            }) |
                            size(WIDTH, EQUAL, 42);

        std::string idText = "-";
        std::string nameText = "-";
        std::string artistText = "-";
        std::string charterText = "-";
        std::string diffText = "-";
        std::string keyText = "-";
        std::string playCountText = "0";
        std::string bestScoreText = "0";
        std::string bestAccText = "0.00";

        if (hasSelection) {
            const auto it = chartList.items().find(ids[selectedIndex]);
            if (it != chartList.items().end()) {
                const auto &entry = it->second;
                const auto &chart = entry.chart;
                idText = chart.getID();
                nameText = chart.getDisplayName().empty() ? ids[selectedIndex] : chart.getDisplayName();
                artistText = chart.getArtist().empty() ? "-" : chart.getArtist();
                charterText = chart.getCharter().empty() ? "-" : chart.getCharter();
                diffText = formatFloat(chart.getDifficulty(), 1);
                keyText = std::to_string(chart.getKeyCount());
                playCountText = std::to_string(entry.stats.playCount);
                bestScoreText = std::to_string(entry.stats.bestScore);
                bestAccText = formatFloat(entry.stats.bestAccuracy * 100.0f, 2) + "%";
            }
        }

        const std::string selectedText = hasSelection
                                             ? (nameText.empty() || nameText == "-" ? ids[selectedIndex] : nameText)
                                             : tr("ui.chart_select.no_charts");

        auto infoRow = [&](const std::string &labelKey, const std::string &value) {
            return hbox({
                text(tr(labelKey)) | color(toColor(palette.textMuted)),
                text(value) | bold | color(toColor(palette.textPrimary)),
            });
        };

        Element metaPanel = vbox({
                               infoRow("ui.chart_select.meta.id", idText),
                               infoRow("ui.chart_select.meta.name", nameText),
                               infoRow("ui.chart_select.meta.artist", artistText),
                               infoRow("ui.chart_select.meta.charter", charterText),
                               infoRow("ui.chart_select.meta.difficulty", diffText),
                               infoRow("ui.chart_select.meta.keys", keyText),
                           });

        metaPanel = window(text("  " + tr("ui.chart_select.meta.title") + " "), metaPanel) |
                    color(toColor(palette.accentPrimary)) |
                    bgcolor(toColor(palette.surfacePanel)) |
                    size(HEIGHT, LESS_THAN, 12);

        Element scorePanel = vbox({
                                infoRow("ui.chart_select.score.play_count", playCountText),
                                infoRow("ui.chart_select.score.best_score", bestScoreText),
                                infoRow("ui.chart_select.score.best_accuracy", bestAccText),
                            });

        scorePanel = window(text("  " + tr("ui.chart_select.score.title") + " "), scorePanel) |
                     color(toColor(palette.accentPrimary)) |
                     bgcolor(toColor(palette.surfacePanel)) |
                     flex;

        Element rightColumn = vbox({
                                 metaPanel,
                                 scorePanel,
                             }) |
                             flex;

        Element content = hbox({
                              leftColumn,
                              text("  "),
                              rightColumn,
                          }) |
                          bgcolor(toColor(palette.surfacePanel)) |
                          flex;

        const std::string searchModeText = tr(searchModes[searchModeIndex].labelKey);
        const std::string sortText = tr(sortOrder == SortOrder::Ascending
                                            ? sortKeys[sortKeyIndex].ascLabelKey
                                            : sortKeys[sortKeyIndex].descLabelKey);
        const std::string sortTextWithArrow = sortText + (sortOrder == SortOrder::Ascending ? " ↑" : " ↓");

        Element topBar = hbox({
                            text(tr("ui.chart_select.title")) | bold | color(toColor(palette.accentPrimary)),
                            filler(),
                            text(tr("ui.chart_select.hint")) | color(toColor(palette.textMuted)),
                        });

        const auto capsule = [&](const std::string &caption, const std::string &value) {
            return text(" " + caption + value + " ") |
                   bold |
                   color(toColor(palette.surfaceBg)) |
                   bgcolor(toColor(palette.accentPrimary));
        };

        Element bottomBar = hbox({
                               text(tr("ui.chart_select.status.selected") + selectedText) |
                                   color(toColor(palette.textMuted)),
                               filler(),
                               capsule(tr("ui.chart_select.status.search_mode"), searchModeText),
                               text(" "),
                               capsule(tr("ui.chart_select.status.sort"), sortTextWithArrow),
                               filler(),
                               text(actionStatus) | color(toColor(palette.textMuted)),
                           });

        Element base = vbox({
                   topBar,
                   separator(),
                   content,
                   separator(),
                   bottomBar,
               }) |
               bgcolor(toColor(palette.surfaceBg)) |
               color(toColor(palette.textPrimary)) |
               flex;

        if (!showStartConfirm) return base;

        Element confirmSelected = text(pendingStartName) |
                                  bold |
                                  color(toColor(palette.accentPrimary));

        Element confirmBody = vbox({
                                hbox({
                                    text(tr("ui.chart_select.confirm.target")) |
                                        color(toColor(palette.textMuted)),
                                    confirmSelected,
                                }),
                                text(tr("ui.chart_select.confirm.hint")) |
                                    color(toColor(palette.textMuted)) |
                                    dim,
                            }) |
                            bgcolor(toColor(palette.surfacePanel));

        Element confirmPanel = window(
                                 text("  " + tr("ui.chart_select.confirm.title") + " "),
                                 confirmBody
                             ) |
                             color(toColor(palette.accentPrimary)) |
                                     bgcolor(toColor(palette.surfacePanel)) |
                             size(WIDTH, EQUAL, 58);

        Element overlay = hbox({filler(), confirmPanel, filler()}) | vcenter | flex;
        return dbox({base, overlay});
    });

    auto app = CatchEvent(root, [&](Event event) {
        if (showStartConfirm) {
            if (event == Event::Escape || event == Event::Character('n') || event == Event::Character('N')) {
                showStartConfirm = false;
                actionStatus = tr("ui.chart_select.action.cancelled");
                return true;
            }

            if (event == Event::Return || event == Event::Character('y') || event == Event::Character('Y')) {
                showStartConfirm = false;
                actionStatus = tr("ui.chart_select.action.start_placeholder") + pendingStartName;
                return true;
            }

            return true;
        }

        if (event == Event::Escape) {
            screen.ExitLoopClosure()();
            return true;
        }

        if (event == Event::Tab) {
            focusSearch = !focusSearch;
            return true;
        }

        if (!focusSearch && event == Event::Character('q')) {
            screen.ExitLoopClosure()();
            return true;
        }

        if (!focusSearch && (event == Event::Character('f') || event == Event::Character('F'))) {
            searchModeIndex = (searchModeIndex + 1) % searchModes.size();
            chartList.setSearchMode(searchModes[searchModeIndex].mode);
            selectedIndex = 0;
            actionStatus = tr("ui.chart_select.action.search_mode_switched") + tr(searchModes[searchModeIndex].labelKey);
            return true;
        }

        if (!focusSearch && (event == Event::Character('r') || event == Event::Character('R'))) {
            sortKeyIndex = (sortKeyIndex + 1) % sortKeys.size();
            chartList.sort(sortKeys[sortKeyIndex].key, sortOrder);
            selectedIndex = 0;
            actionStatus = tr("ui.chart_select.action.sort_key_switched") +
                           tr(sortOrder == SortOrder::Ascending
                                  ? sortKeys[sortKeyIndex].ascLabelKey
                                  : sortKeys[sortKeyIndex].descLabelKey);
            return true;
        }

        if (!focusSearch && (event == Event::Character('o') || event == Event::Character('O'))) {
            sortOrder = sortOrder == SortOrder::Ascending ? SortOrder::Descending : SortOrder::Ascending;
            chartList.sort(sortKeys[sortKeyIndex].key, sortOrder);
            selectedIndex = 0;
            actionStatus = tr("ui.chart_select.action.sort_order_switched") +
                           tr(sortOrder == SortOrder::Ascending
                                  ? sortKeys[sortKeyIndex].ascLabelKey
                                  : sortKeys[sortKeyIndex].descLabelKey);
            return true;
        }

        if (!focusSearch && (event == Event::ArrowUp || event == Event::Character('k'))) {
            const auto &ids = chartList.filteredOrderedChartIDs();
            if (!ids.empty()) {
                selectedIndex = (selectedIndex + ids.size() - 1) % ids.size();
            }
            return true;
        }

        if (!focusSearch && (event == Event::ArrowDown || event == Event::Character('j'))) {
            const auto &ids = chartList.filteredOrderedChartIDs();
            if (!ids.empty()) {
                selectedIndex = (selectedIndex + 1) % ids.size();
            }
            return true;
        }

        if (!focusSearch && event == Event::Return) {
            const auto &ids = chartList.filteredOrderedChartIDs();
            if (ids.empty()) {
                actionStatus = tr("ui.chart_select.action.no_selection");
                return true;
            }

            const auto it = chartList.items().find(ids[selectedIndex]);
            const std::string displayName =
                (it != chartList.items().end() && !it->second.chart.getDisplayName().empty())
                    ? it->second.chart.getDisplayName()
                    : ids[selectedIndex];

            pendingStartName = displayName;
            showStartConfirm = true;
            return true;
        }

        return focusSearch ? container->OnEvent(event) : false;
    });

    screen.Loop(app);
    return 0;
}

} // namespace ui

