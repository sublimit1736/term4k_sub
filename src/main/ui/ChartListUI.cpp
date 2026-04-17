#include "ui/ChartListUI.h"

#include "config/AppDirs.h"
#include "config/RuntimeConfigs.h"
#include "dao/ProofedRecordsDAO.h"
#include "instances/ChartListInstance.h"
#include "services/AuthenticatedUserService.h"
#include "services/I18nService.h"
#include "ui/MessageOverlay.h"
#include "ui/TransitionBackdrop.h"
#include "ui/UIBus.h"
#include "ui/UIColors.h"
#include "utils/StringUtils.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <vector>

namespace ui {
    namespace {
        constexpr const char *kBarEmptyHead = "";
        constexpr const char *kBarEmptyMid  = "";
        constexpr const char *kBarEmptyTail = "";
        constexpr const char *kBarFillHead  = "";
        constexpr const char *kBarFillMid   = "";
        constexpr const char *kBarFillTail  = "";
        constexpr std::size_t kProgressBarSlots = 24;

        std::string formatFloat(const float value, const int precision = 2) {
            std::ostringstream out;
            out << std::fixed << std::setprecision(precision) << value;
            return out.str();
        }

        std::size_t countChartFiles(const std::string &chartsRoot) {
            namespace fs = std::filesystem;
            std::error_code ec;
            if (!fs::exists(chartsRoot, ec) || !fs::is_directory(chartsRoot, ec)) return 0;

            std::size_t count = 0;
            for (fs::recursive_directory_iterator it(chartsRoot, ec), end; it != end && !ec; it.increment(ec)) {
                if (it->is_regular_file(ec) && it->path().filename() == "chart.t4k") {
                    ++count;
                }
            }
            return count;
        }

        std::size_t countVerifiedRecordLines(const std::string &dataDir) {
            namespace fs = std::filesystem;
            std::string proofPath = dataDir;
            if (!proofPath.empty() && proofPath.back() != '/' && proofPath.back() != '\\') proofPath.push_back('/');
            proofPath += "proof.db";

            std::error_code ec;
            if (!fs::exists(proofPath, ec) || !fs::is_regular_file(proofPath, ec)) return 0;

            std::ifstream in(proofPath);
            if (!in.is_open()) return 0;

            std::size_t count = 0;
            std::string line;
            while (std::getline(in, line)) {
                if (!line.empty()) ++count;
            }
            return count;
        }

        std::string buildProgressBar(const int percent) {
            constexpr std::size_t slots = kProgressBarSlots;

            const int clamped = std::max(0, std::min(100, percent));
            const auto filled = static_cast<std::size_t>((static_cast<long long>(clamped) * slots) / 100);

            std::string out;
            out.reserve(slots * 3);
            for (std::size_t i = 0; i < slots; ++i) {
                const bool solid = i < filled;
                if (i == 0) out += solid ? kBarFillHead : kBarEmptyHead;
                else if (i + 1 == slots) out += solid ? kBarFillTail : kBarEmptyTail;
                else out += solid ? kBarFillMid : kBarEmptyMid;
            }
            return out;
        }

        struct ChartLoadResult {
            ChartListInstance chartList;
            std::unordered_map<std::string, std::string> fileSizeCache;
        };

        struct ChartListLoadSession {
            std::atomic<std::size_t> chartsLoaded{0};
            std::atomic<std::size_t> recordsLoaded{0};
            std::atomic<std::size_t> totalChartFiles{0};
            std::atomic<std::size_t> totalRecordLines{0};
            std::mutex loadMutex;
            std::vector<std::string> progressiveIDs;
            std::optional<ChartLoadResult> pendingChartResult;
            std::optional<std::vector<std::string>> pendingRecords;
            std::atomic<bool> chartsDone{false};
            std::atomic<bool> recordsDone{false};
            std::atomic<bool> loadFailed{false};
            std::string loadError;
            bool loadCompleted = false;
            std::atomic<bool> keepRunning{true};
        };

        std::string getCurrentExceptionMessage() {
            try {
                throw;
            } catch (const std::exception &e) {
                return e.what();
            } catch (...) {
                return "unknown error";
            }
        }

        std::vector<std::string> discoverChartIDs(const std::string &chartsRoot,
                                                  std::atomic<std::size_t> &loaded,
                                                  ftxui::ScreenInteractive *screen,
                                                  const std::atomic<bool> &keepRunning) {
            namespace fs = std::filesystem;
            std::vector<std::string> ids;

            std::error_code ec;
            if (!fs::exists(chartsRoot, ec) || !fs::is_directory(chartsRoot, ec)) return ids;

            for (fs::recursive_directory_iterator it(chartsRoot, ec), end;
                 it != end && !ec && keepRunning.load(std::memory_order_relaxed);
                 it.increment(ec)) {
                if (!it->is_regular_file(ec) || it->path().filename() != "chart.t4k") continue;
                ids.push_back(it->path().parent_path().filename().string());
                ++loaded;
                if (screen && (loaded.load(std::memory_order_relaxed) % 32 == 0)) {
                    screen->PostEvent(ftxui::Event::Custom);
                }
            }

            return ids;
        }

        float accuracyPercent(const float accuracy) {
            if (accuracy >= 0.0f && accuracy <= 1.0f) return accuracy * 100.0f;
            return accuracy;
        }

        std::string formatFileSize(const std::uintmax_t sizeBytes) {
            constexpr double kb = 1024.0;
            constexpr double mb = 1024.0 * 1024.0;
            if (sizeBytes < 1024) return std::to_string(sizeBytes) + "B";
            std::ostringstream out;
            out << std::fixed << std::setprecision(1);
            if (static_cast<double>(sizeBytes) >= mb){
                out << (static_cast<double>(sizeBytes) / mb) << "MB";
            }
            else{
                out << (static_cast<double>(sizeBytes) / kb) << "KB";
            }
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

        struct GradeVisual {
            std::string label;
            ftxui::Color color = ftxui::Color::Default;
            bool rainbow       = false;
        };

        struct AchievementVisual {
            std::string label;
            ftxui::Color color = ftxui::Color::Default;
            bool rainbow       = false;
        };

        struct SelectedChartSnapshot {
            bool hasSelection = false;
            std::size_t index = 0;
            std::string chartId;
            bool hasEntry = false;
        };

        struct RightPanelState {
            std::string nameText = "-";
            std::string artistText = "-";
            std::string charterText = "-";
            std::string diffText = "-";
            std::string keyText = "-";
            std::string playCountText = "0";
            std::string bestScoreText = "0";
            std::string bestAccText = "0.00";
            std::string bestRatingText;
            bool hasPlayRecord = false;
            GradeVisual currentGrade;
            AchievementVisual currentAchievement;
        };

        void tryFinalizeLoadSession(ChartListLoadSession &session,
                                    ChartListInstance &chartList,
                                    std::unordered_map<std::string, std::string> &fileSizeCache,
                                    std::vector<std::string> &allVerifiedRecords,
                                    const std::array<SearchModeOption, 3> &searchModes,
                                    const std::size_t searchModeIndex,
                                    const std::array<SortKeyOption, 3> &sortKeys,
                                    const std::size_t sortKeyIndex,
                                    const SortOrder sortOrder,
                                    const std::string &searchQuery,
                                    std::string &appliedSearch,
                                    std::size_t &selectedIndex) {
            if (session.loadCompleted || !session.chartsDone.load() || !session.recordsDone.load()) return;

            std::scoped_lock lock(session.loadMutex);
            if (!session.pendingChartResult.has_value() || !session.pendingRecords.has_value()) return;

            chartList = std::move(session.pendingChartResult->chartList);
            fileSizeCache = std::move(session.pendingChartResult->fileSizeCache);
            allVerifiedRecords = std::move(*session.pendingRecords);
            session.pendingChartResult.reset();
            session.pendingRecords.reset();

            chartList.setSearchMode(searchModes[searchModeIndex].mode);
            chartList.sort(sortKeys[sortKeyIndex].key, sortOrder);
            chartList.setSearchQuery(searchQuery);
            appliedSearch = searchQuery;
            selectedIndex = 0;
            session.loadCompleted = true;
        }

        std::vector<std::string> collectVisibleChartIds(ChartListLoadSession &session,
                                                        ChartListInstance &chartList,
                                                        const std::string &searchQuery,
                                                        std::string &appliedSearch,
                                                        std::size_t &selectedIndex) {
            std::vector<std::string> ids;

            if (session.loadCompleted) {
                if (appliedSearch != searchQuery) {
                    chartList.setSearchQuery(searchQuery);
                    appliedSearch = searchQuery;
                    selectedIndex = 0;
                }

                const auto &loadedIds = chartList.filteredOrderedChartIDs();
                ids.assign(loadedIds.begin(), loadedIds.end());
                return ids;
            }

            std::scoped_lock lock(session.loadMutex);
            return session.progressiveIDs;
        }

        SelectedChartSnapshot resolveSelectedSnapshot(const std::vector<std::string> &ids,
                                                      std::size_t &selectedIndex,
                                                      const ChartListInstance &chartList) {
            SelectedChartSnapshot snapshot;
            if (ids.empty()) return snapshot;

            selectedIndex = std::min(selectedIndex, ids.size() - 1);
            snapshot.hasSelection = true;
            snapshot.index = selectedIndex;
            snapshot.chartId = ids[selectedIndex];
            snapshot.hasEntry = chartList.items().contains(snapshot.chartId);
            return snapshot;
        }

        GradeVisual gradeFromAccuracy(float accPercent);
        AchievementVisual achievementFromStats(const ChartPlayStats &stats);
        ftxui::Element colorizedToken(const std::string &token, const ftxui::Color &baseColor, bool rainbow);
        struct LeaderboardView;
        LeaderboardView buildLeaderboard(const std::vector<std::string> &records,
                                         const std::string &chartId,
                                         bool byAccuracy,
                                         const std::string &selfUid);
        ftxui::Element renderRightColumn(const ThemePalette &palette,
                                         const std::function<std::string(const std::string &)> &tr,
                                         const RightPanelState &state,
                                         const LeaderboardView &leaderboard,
                                         bool rankByAccuracy,
                                         const std::string &statsUID);
        ftxui::Element renderChartListBaseLayout(const ThemePalette &palette,
                                                 const std::function<std::string(const std::string &)> &tr,
                                                 const ftxui::Element &content,
                                                 bool loadCompleted,
                                                 const std::string &loadFailureMessage,
                                                 std::size_t chartsLoaded,
                                                 std::size_t totalChartFiles,
                                                 std::size_t recordsLoaded,
                                                 std::size_t totalRecordLines,
                                                 const std::string &selectedText,
                                                 const std::string &rightStatusText);
        ftxui::Element composeChartListScene(const ThemePalette &palette,
                                             const std::function<std::string(const std::string &)> &tr,
                                             const ftxui::Element &base,
                                             bool showStartConfirm,
                                             const std::string &pendingStartName);

        RightPanelState buildRightPanelState(const SelectedChartSnapshot &selection,
                                             const ChartListInstance &chartList,
                                             const bool loadCompleted) {
            RightPanelState panelState{};
            if (!selection.hasSelection) return panelState;

            const auto it = chartList.items().find(selection.chartId);
            if (it == chartList.items().end()) {
                if (!loadCompleted) {
                    panelState.nameText = selection.chartId;
                }
                return panelState;
            }

            const auto &entry = it->second;
            const auto &chart = entry.chart;
            panelState.nameText = chart.getDisplayName().empty() ? selection.chartId : chart.getDisplayName();
            panelState.artistText = chart.getArtist().empty() ? "-" : chart.getArtist();
            panelState.charterText = chart.getCharter().empty() ? "-" : chart.getCharter();
            panelState.diffText = formatFloat(chart.getDifficulty(), 1);
            panelState.keyText = std::to_string(chart.getKeyCount());
            panelState.playCountText = std::to_string(entry.stats.playCount);
            panelState.bestScoreText = std::to_string(entry.stats.bestScore);

            const float bestAccPercent = accuracyPercent(entry.stats.bestAccuracy);
            panelState.bestAccText = formatFloat(bestAccPercent, 2) + "%";
            panelState.hasPlayRecord = entry.stats.playCount > 0;
            if (panelState.hasPlayRecord) {
                panelState.bestRatingText = formatFloat(static_cast<float>(entry.stats.bestSingleRating), 2);
                panelState.currentGrade = gradeFromAccuracy(bestAccPercent);
                panelState.currentAchievement = achievementFromStats(entry.stats);
            }
            return panelState;
        }

        ftxui::Elements buildChartListRows(const ThemePalette &palette,
                                           const std::function<std::string(const std::string &)> &tr,
                                           const std::vector<std::string> &ids,
                                           const ChartListInstance &chartList,
                                           const std::unordered_map<std::string, std::string> &fileSizeCache,
                                           const std::size_t selectedIndex) {
            using namespace ftxui;

            Elements rows;
            if (ids.empty()) {
                rows.push_back(text(tr("ui.chart_select.no_charts")) | color(toColor(palette.textMuted)));
                return rows;
            }

            for (std::size_t i = 0; i < ids.size(); ++i) {
                const auto it = chartList.items().find(ids[i]);
                std::string title = ids[i];
                std::string sizeText;
                Element rightInfo = text("");

                if (it != chartList.items().end()) {
                    const auto &entry = it->second;
                    if (!entry.chart.getDisplayName().empty()) title = entry.chart.getDisplayName();

                    const auto szIt = fileSizeCache.find(ids[i]);
                    if (szIt != fileSizeCache.end()) sizeText = szIt->second;

                    if (entry.stats.playCount > 0) {
                        const float acc = accuracyPercent(entry.stats.bestAccuracy);
                        const GradeVisual grade = gradeFromAccuracy(acc);
                        const AchievementVisual ach = achievementFromStats(entry.stats);

                        Elements rightParts;
                        rightParts.push_back(colorizedToken(grade.label, grade.color, grade.rainbow));
                        if (!ach.label.empty()) {
                            rightParts.push_back(text(" | ") | color(toColor(palette.textMuted)));
                            rightParts.push_back(colorizedToken(ach.label, ach.color, ach.rainbow));
                        }
                        rightInfo = hbox(std::move(rightParts));
                    }
                }

                Elements leftParts;
                leftParts.push_back(text(title));
                if (!sizeText.empty()) {
                    leftParts.push_back(text(" "));
                    leftParts.push_back(text(sizeText) | dim | color(toColor(palette.textMuted)));
                }

                Element row = hbox({
                    hbox(std::move(leftParts)) | flex,
                    rightInfo,
                });

                if (i == selectedIndex) {
                    row = row | bold |
                          color(highContrastOn(palette.accentPrimary)) |
                          bgcolor(toColor(palette.accentPrimary));
                } else {
                    row = row | color(toColor(palette.textPrimary));
                }
                rows.push_back(row);
            }

            return rows;
        }

        ftxui::Element renderChartListColumn(const ThemePalette &palette,
                                             const std::function<std::string(const std::string &)> &tr,
                                             const bool focusSearch,
                                             const ftxui::Element &searchInputElement,
                                             const std::vector<std::string> &ids,
                                             const ChartListInstance &chartList,
                                             const std::unordered_map<std::string, std::string> &fileSizeCache,
                                             const std::size_t selectedIndex) {
            using namespace ftxui;

            Element searchBox = vbox({searchInputElement});
            searchBox = window(text("  " + tr("ui.chart_select.search") + " "), searchBox) |
                        color(toColor(focusSearch ? palette.accentPrimary : palette.borderNormal)) |
                        bgcolor(toColor(palette.surfacePanel));

            Element listBox = vbox(buildChartListRows(palette, tr, ids, chartList, fileSizeCache, selectedIndex)) |
                              borderRounded |
                              color(toColor(!focusSearch ? palette.accentPrimary : palette.borderNormal)) |
                              bgcolor(toColor(palette.surfacePanel)) |
                              frame |
                              flex;

            return vbox({searchBox, listBox}) | size(WIDTH, EQUAL, 42);
        }

        ftxui::Element renderChartListBottomBar(const ThemePalette &palette,
                                                const bool loadCompleted,
                                                const std::string &loadFailureMessage,
                                                const std::size_t chartsLoaded,
                                                const std::size_t totalChartFiles,
                                                const std::size_t recordsLoaded,
                                                const std::size_t totalRecordLines,
                                                const std::string &selectedText,
                                                const std::string &rightStatusText,
                                                const std::function<std::string(const std::string &)> &tr) {
            using namespace ftxui;

            Element bottomBar;
            if (!loadCompleted) {
                if (loadFailureMessage.empty()) {
                    const std::size_t loaded = std::min(chartsLoaded, totalChartFiles) +
                                               std::min(recordsLoaded, totalRecordLines);
                    const std::size_t total = totalChartFiles + totalRecordLines;
                    std::string progressText;
                    if (total == 0) {
                        // Totals not yet known — show discovered count with an empty bar.
                        progressText = buildProgressBar(0) + "  " +
                                       std::to_string(chartsLoaded + recordsLoaded);
                    } else {
                        const int percent = static_cast<int>((loaded * 100) / total);
                        progressText = buildProgressBar(percent) + "  " + std::to_string(percent) + "%";
                    }

                    bottomBar = hbox({
                        filler(),
                        text(progressText) | color(toColor(palette.accentPrimary)) | bold,
                        filler(),
                    });
                } else {
                    bottomBar = hbox({
                        text(std::string("Load failed: ") + loadFailureMessage) | color(ftxui::Color::RedLight),
                        filler(),
                        text(tr("ui.chart_select.hint")) | color(toColor(palette.textMuted)),
                    });
                }
            } else {
                bottomBar = hbox({
                    text(tr("ui.chart_select.status.selected") + selectedText) | color(toColor(palette.textMuted)),
                    filler(),
                    text(rightStatusText) | color(toColor(palette.textMuted)),
                });
            }

            return bottomBar | size(HEIGHT, EQUAL, 1);
        }

        struct LeaderboardEntry {
            std::string uid;
            std::string name;
            uint32_t score = 0;
            float accuracy = 0.0f;
        };

        struct LeaderboardView {
            std::vector<LeaderboardEntry> top;
            int selfRank   = -1;
            bool selfInTop = false;
            LeaderboardEntry self;
        };

        struct ChartListUiState {
            std::size_t searchModeIndex = 0;
            std::size_t sortKeyIndex = 0;
            SortOrder sortOrder = SortOrder::Ascending;
            std::string searchQuery;
            std::string appliedSearch;
            std::size_t selectedIndex = 0;
            bool focusSearch = true;
            std::string actionStatus;
            bool showStartConfirm = false;
            std::string pendingStartName;
            bool leaderboardByAccuracy = false;
            std::string leaderboardCacheKey;
            bool leaderboardCacheByAcc = false;
            LeaderboardView cachedLeaderboard;
        };

        struct ChartListEventContext {
            ChartListLoadSession &session;
            ChartListUiState &uiState;
            ChartListInstance &chartList;
            const std::array<SearchModeOption, 3> &searchModes;
            const std::array<SortKeyOption, 3> &sortKeys;
            const std::function<std::string(const std::string &)> &tr;
            const std::function<void()> &requestExit;
            ftxui::Component &container;
        };

        bool dispatchChartListEvent(const ftxui::Event &event, ChartListEventContext &ctx) {
            if (!ctx.session.loadCompleted) {
                if (event == ftxui::Event::Escape || event == ftxui::Event::Character('q')) {
                    ctx.requestExit();
                }
                return true;
            }

            if (ctx.uiState.showStartConfirm) {
                if (event == ftxui::Event::Escape || event == ftxui::Event::Character('n') ||
                    event == ftxui::Event::Character('N')) {
                    ctx.uiState.showStartConfirm = false;
                    ctx.uiState.actionStatus = ctx.tr("ui.chart_select.action.cancelled");
                    return true;
                }

                if (event == ftxui::Event::Return || event == ftxui::Event::Character('y') ||
                    event == ftxui::Event::Character('Y')) {
                    ctx.uiState.showStartConfirm = false;
                    ctx.uiState.actionStatus =
                        ctx.tr("ui.chart_select.action.start_placeholder") + ctx.uiState.pendingStartName;
                    return true;
                }

                return true;
            }

            if (event == ftxui::Event::Escape) {
                ctx.requestExit();
                return true;
            }

            if (event == ftxui::Event::Tab) {
                ctx.uiState.focusSearch = !ctx.uiState.focusSearch;
                return true;
            }

            if (!ctx.uiState.focusSearch && event == ftxui::Event::Character('q')) {
                ctx.requestExit();
                return true;
            }

            if (!ctx.uiState.focusSearch) {
                if (event == ftxui::Event::Character('f') || event == ftxui::Event::Character('F')) {
                    ctx.uiState.searchModeIndex = (ctx.uiState.searchModeIndex + 1) % ctx.searchModes.size();
                    ctx.chartList.setSearchMode(ctx.searchModes[ctx.uiState.searchModeIndex].mode);
                    ctx.uiState.selectedIndex = 0;
                    ctx.uiState.actionStatus =
                        ctx.tr("ui.chart_select.action.search_mode_switched") +
                        ctx.tr(ctx.searchModes[ctx.uiState.searchModeIndex].labelKey);
                    return true;
                }

                if (event == ftxui::Event::Character('l') || event == ftxui::Event::Character('L')) {
                    ctx.uiState.leaderboardByAccuracy = !ctx.uiState.leaderboardByAccuracy;
                    ctx.uiState.actionStatus =
                        ctx.tr("ui.chart_select.action.leaderboard_mode_switched") +
                        ctx.tr(ctx.uiState.leaderboardByAccuracy
                                   ? "ui.chart_select.leaderboard.mode_acc"
                                   : "ui.chart_select.leaderboard.mode_score");
                    return true;
                }

                if (event == ftxui::Event::Character('r') || event == ftxui::Event::Character('R')) {
                    ctx.uiState.sortKeyIndex = (ctx.uiState.sortKeyIndex + 1) % ctx.sortKeys.size();
                    ctx.chartList.sort(ctx.sortKeys[ctx.uiState.sortKeyIndex].key, ctx.uiState.sortOrder);
                    ctx.uiState.selectedIndex = 0;
                    ctx.uiState.actionStatus =
                        ctx.tr("ui.chart_select.action.sort_key_switched") +
                        ctx.tr(ctx.uiState.sortOrder == SortOrder::Ascending
                                   ? ctx.sortKeys[ctx.uiState.sortKeyIndex].ascLabelKey
                                   : ctx.sortKeys[ctx.uiState.sortKeyIndex].descLabelKey);
                    return true;
                }

                if (event == ftxui::Event::Character('o') || event == ftxui::Event::Character('O')) {
                    ctx.uiState.sortOrder = ctx.uiState.sortOrder == SortOrder::Ascending
                                                ? SortOrder::Descending
                                                : SortOrder::Ascending;
                    ctx.chartList.sort(ctx.sortKeys[ctx.uiState.sortKeyIndex].key, ctx.uiState.sortOrder);
                    ctx.uiState.selectedIndex = 0;
                    ctx.uiState.actionStatus =
                        ctx.tr("ui.chart_select.action.sort_order_switched") +
                        ctx.tr(ctx.uiState.sortOrder == SortOrder::Ascending
                                   ? ctx.sortKeys[ctx.uiState.sortKeyIndex].ascLabelKey
                                   : ctx.sortKeys[ctx.uiState.sortKeyIndex].descLabelKey);
                    return true;
                }

                if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character('k')) {
                    const auto &ids = ctx.chartList.filteredOrderedChartIDs();
                    if (!ids.empty()) {
                        ctx.uiState.selectedIndex = std::min(ctx.uiState.selectedIndex, ids.size() - 1);
                        ctx.uiState.selectedIndex = (ctx.uiState.selectedIndex + ids.size() - 1) % ids.size();
                    }
                    return true;
                }

                if (event == ftxui::Event::ArrowDown || event == ftxui::Event::Character('j')) {
                    const auto &ids = ctx.chartList.filteredOrderedChartIDs();
                    if (!ids.empty()) {
                        ctx.uiState.selectedIndex = std::min(ctx.uiState.selectedIndex, ids.size() - 1);
                        ctx.uiState.selectedIndex = (ctx.uiState.selectedIndex + 1) % ids.size();
                    }
                    return true;
                }

                if (event == ftxui::Event::Return) {
                    const auto &ids = ctx.chartList.filteredOrderedChartIDs();
                    if (ids.empty()) {
                        ctx.uiState.actionStatus = ctx.tr("ui.chart_select.action.no_selection");
                        return true;
                    }

                    ctx.uiState.selectedIndex = std::min(ctx.uiState.selectedIndex, ids.size() - 1);
                    const auto it = ctx.chartList.items().find(ids[ctx.uiState.selectedIndex]);
                    ctx.uiState.pendingStartName =
                        (it != ctx.chartList.items().end() && !it->second.chart.getDisplayName().empty())
                            ? it->second.chart.getDisplayName()
                            : ids[ctx.uiState.selectedIndex];
                    ctx.uiState.showStartConfirm = true;
                    return true;
                }
            }

            if (ctx.uiState.focusSearch) return ctx.container->OnEvent(event);
            return true;
        }

        void updateLeaderboardCacheIfNeeded(ChartListUiState &uiState,
                                            const std::vector<std::string> &allVerifiedRecords,
                                            const std::string &currentChartId,
                                            const bool rankByAccuracy,
                                            const std::string &statsUID) {
            if (currentChartId != uiState.leaderboardCacheKey || rankByAccuracy != uiState.leaderboardCacheByAcc) {
                uiState.cachedLeaderboard =
                    buildLeaderboard(allVerifiedRecords, currentChartId, rankByAccuracy, statsUID);
                uiState.leaderboardCacheKey = currentChartId;
                uiState.leaderboardCacheByAcc = rankByAccuracy;
            }
        }

        ftxui::Element renderChartListFrame(const ThemePalette &palette,
                                            const std::function<std::string(const std::string &)> &tr,
                                            ChartListLoadSession &session,
                                            ChartListInstance &chartList,
                                            std::vector<std::string> &allVerifiedRecords,
                                            std::unordered_map<std::string, std::string> &fileSizeCache,
                                            ChartListUiState &uiState,
                                            const std::array<SearchModeOption, 3> &searchModes,
                                            const std::array<SortKeyOption, 3> &sortKeys,
                                            const std::string &statsUID,
                                            const ftxui::Component &searchInput) {
            using namespace ftxui;

            tryFinalizeLoadSession(session,
                                   chartList,
                                   fileSizeCache,
                                   allVerifiedRecords,
                                   searchModes,
                                   uiState.searchModeIndex,
                                   sortKeys,
                                   uiState.sortKeyIndex,
                                   uiState.sortOrder,
                                   uiState.searchQuery,
                                   uiState.appliedSearch,
                                   uiState.selectedIndex);

            std::string loadFailureMessage;
            if (!session.loadCompleted && session.loadFailed.load()) {
                std::scoped_lock lock(session.loadMutex);
                loadFailureMessage = session.loadError;
            }

            std::vector<std::string> ids =
                collectVisibleChartIds(session, chartList, uiState.searchQuery, uiState.appliedSearch, uiState.selectedIndex);
            const SelectedChartSnapshot selection = resolveSelectedSnapshot(ids, uiState.selectedIndex, chartList);
            const bool hasSelection = selection.hasSelection;

            Element leftColumn = renderChartListColumn(palette,
                                                       tr,
                                                       uiState.focusSearch,
                                                       searchInput->Render(),
                                                       ids,
                                                       chartList,
                                                       fileSizeCache,
                                                       uiState.selectedIndex);

            const RightPanelState rightPanelState = buildRightPanelState(selection, chartList, session.loadCompleted);

            const std::string selectedText = hasSelection
                                                 ? (rightPanelState.nameText.empty() || rightPanelState.nameText == "-"
                                                        ? selection.chartId
                                                        : rightPanelState.nameText)
                                                 : tr("ui.chart_select.no_charts");

            const bool rankByAccuracy = uiState.leaderboardByAccuracy;
            const std::string currentChartId = hasSelection ? selection.chartId : "";
            updateLeaderboardCacheIfNeeded(uiState, allVerifiedRecords, currentChartId, rankByAccuracy, statsUID);

            Element rightColumn = renderRightColumn(palette,
                                                    tr,
                                                    rightPanelState,
                                                    uiState.cachedLeaderboard,
                                                    rankByAccuracy,
                                                    statsUID);

            Element content = hbox({
                                   leftColumn,
                                   text("  "),
                                   rightColumn,
                               }) |
                              bgcolor(toColor(palette.surfacePanel)) |
                              flex;

            const std::string searchModeText = tr(searchModes[uiState.searchModeIndex].labelKey);
            const std::string sortText = tr(uiState.sortOrder == SortOrder::Ascending
                                                ? sortKeys[uiState.sortKeyIndex].ascLabelKey
                                                : sortKeys[uiState.sortKeyIndex].descLabelKey);
            const std::string sortTextWithArrow = sortText + (uiState.sortOrder == SortOrder::Ascending ? " ↑" : " ↓");

            Element base = renderChartListBaseLayout(
                palette,
                tr,
                content,
                session.loadCompleted,
                loadFailureMessage,
                session.chartsLoaded.load(),
                session.totalChartFiles.load(),
                session.recordsLoaded.load(),
                session.totalRecordLines.load(),
                selectedText,
                searchModeText + " | " + sortTextWithArrow + " | " + uiState.actionStatus);

            Element composed = composeChartListScene(palette, tr, base, uiState.showStartConfirm, uiState.pendingStartName);
            TransitionBackdrop::update(composed);
            return composed;
        }


        bool parseLeaderboardRecord(const std::string &record,
                                    std::string* outUid,
                                    std::string* outChartId,
                                    std::string* outName,
                                    uint32_t* outScore,
                                    float* outAccuracy
            ) {
            std::istringstream iss(record);
            std::vector<std::string> fields;
            std::string token;
            while (iss >> token) fields.push_back(token);
            if (fields.size() < 6) return false;

            const bool uidFormat = (fields.size() >= 7) && string_utils::isDigitsOnly(fields[0]);
            if (!uidFormat) return false;

            try{
                *outUid      = fields[0];
                *outChartId  = fields[1];
                *outName     = fields[3];
                *outScore    = static_cast<uint32_t>(std::stoul(fields[4]));
                *outAccuracy = std::stof(fields[5]);
                return true;
            }
            catch (...){
                return false;
            }
        }

        LeaderboardView buildLeaderboard(const std::vector<std::string> &records,
                                         const std::string &chartId,
                                         const bool byAccuracy,
                                         const std::string &selfUid
            ) {
            LeaderboardView out;
            if (chartId.empty()) return out;

            std::unordered_map<std::string, LeaderboardEntry> bestByUid;
            for (const auto &raw: records){
                std::string uid;
                std::string cid;
                std::string name;
                uint32_t score = 0;
                float accuracy = 0.0f;
                if (!parseLeaderboardRecord(raw, &uid, &cid, &name, &score, &accuracy)) continue;
                if (cid != chartId) continue;

                auto it = bestByUid.find(uid);
                if (it == bestByUid.end()){
                    bestByUid[uid] = {uid, name.empty() ? uid : name, score, accuracy};
                    continue;
                }

                const bool better = byAccuracy
                                        ? (accuracy > it->second.accuracy ||
                                           (accuracy == it->second.accuracy && score > it->second.score))
                                        : (score > it->second.score ||
                                           (score == it->second.score && accuracy > it->second.accuracy));

                if (better){
                    it->second.name     = name.empty() ? uid : name;
                    it->second.score    = score;
                    it->second.accuracy = accuracy;
                }
            }

            std::vector<LeaderboardEntry> all;
            all.reserve(bestByUid.size());
            for (const auto &[_, entry]: bestByUid) all.push_back(entry);

            std::ranges::stable_sort(all, [&](const LeaderboardEntry &a, const LeaderboardEntry &b) {
                if (byAccuracy){
                    if (a.accuracy != b.accuracy) return a.accuracy > b.accuracy;
                    if (a.score != b.score) return a.score > b.score;
                }
                else{
                    if (a.score != b.score) return a.score > b.score;
                    if (a.accuracy != b.accuracy) return a.accuracy > b.accuracy;
                }
                return a.uid < b.uid;
            });

            const std::size_t topCount = std::min<std::size_t>(99, all.size());
            out.top.assign(all.begin(), all.begin() + static_cast<long long>(topCount));

            for (std::size_t i = 0; i < all.size(); ++i){
                if (!selfUid.empty() && all[i].uid == selfUid){
                    out.selfRank  = static_cast<int>(i + 1);
                    out.self      = all[i];
                    out.selfInTop = (i < topCount);
                    break;
                }
            }
            return out;
        }

        GradeVisual gradeFromAccuracy(const float accPercent) {
            if (accPercent >= 100.0f) return {"SSS+", ftxui::Color::White, true};
            if (accPercent >= 99.5f) return {"SSS", ftxui::Color::RGB(255, 215, 0), false};
            if (accPercent >= 99.0f) return {"SS+", ftxui::Color::RGB(192, 192, 192), false};
            if (accPercent >= 98.0f) return {"SS", ftxui::Color::RGB(255, 80, 40), false};
            if (accPercent >= 97.0f) return {"S+", ftxui::Color::RGB(150, 0, 0), false};
            if (accPercent >= 95.0f) return {"S", ftxui::Color::RGB(170, 85, 255), false};
            if (accPercent >= 90.0f) return {"A", ftxui::Color::RGB(255, 105, 180), false};
            if (accPercent >= 80.0f) return {"B", ftxui::Color::RGB(255, 240, 160), false};
            if (accPercent >= 70.0f) return {"C", ftxui::Color::RGB(120, 200, 255), false};
            if (accPercent >= 60.0f) return {"D", ftxui::Color::RGB(120, 255, 120), false};
            return {"-", ftxui::Color::RGB(130, 130, 130), false};
        }

        AchievementVisual achievementFromStats(const ChartPlayStats &stats) {
            if (stats.hasULT) return {"ULT", ftxui::Color::White, true};
            if (stats.hasAP) return {"AP", ftxui::Color::RGB(255, 215, 0), false};
            if (stats.hasFC) return {"FC", ftxui::Color::RGB(192, 192, 192), false};
            return {};
        }

        const std::array<ftxui::Color, 7> kRainbow = {
            ftxui::Color::RGB(255, 0, 0),
            ftxui::Color::RGB(255, 127, 0),
            ftxui::Color::RGB(255, 255, 0),
            ftxui::Color::RGB(0, 200, 0),
            ftxui::Color::RGB(0, 120, 255),
            ftxui::Color::RGB(75, 0, 130),
            ftxui::Color::RGB(148, 0, 211),
        };

        const std::map<char, std::array<std::string, 6>> kBlockGlyph = {
            {'A', {" █████╗ ", "██╔══██╗", "███████║", "██╔══██║", "██║  ██║", "╚═╝  ╚═╝"}},
            {'B', {"██████╗ ", "██╔══██╗", "██████╔╝", "██╔══██╗", "██████╔╝", "╚═════╝ "}},
            {'C', {" ██████╗", "██╔════╝", "██║     ", "██║     ", "╚██████╗", " ╚═════╝"}},
            {'D', {"██████╗ ", "██╔══██╗", "██║  ██║", "██║  ██║", "██████╔╝", "╚═════╝ "}},
            {'F', {"███████╗", "██╔════╝", "█████╗  ", "██╔══╝  ", "██║     ", "╚═╝     "}},
            {'L', {"██╗     ", "██║     ", "██║     ", "██║     ", "███████╗", "╚══════╝"}},
            {'P', {"██████╗ ", "██╔══██╗", "██████╔╝", "██╔═══╝ ", "██║     ", "╚═╝     "}},
            {'S', {"███████╗", "██╔════╝", "███████╗", "╚════██║", "███████║", "╚══════╝"}},
            {'T', {"████████╗", "╚══██╔══╝", "   ██║   ", "   ██║   ", "   ██║   ", "   ╚═╝   "}},
            {'U', {"██╗   ██╗", "██║   ██║", "██║   ██║", "██║   ██║", "╚██████╔╝", " ╚═════╝ "}},
            {'+', {"  ██╗  ", "██████╗", "╚═██╔═╝", "  ╚═╝  ", "       ", "       "}},
        };

        ftxui::Element colorizedToken(const std::string &token, const ftxui::Color &baseColor, const bool rainbow) {
            using namespace ftxui;
            if (!rainbow) return text(token) | bold | color(baseColor);

            Elements chars;
            std::size_t colorIndex = 0;
            for (char ch: token){
                if (ch == ' '){
                    chars.push_back(text(" "));
                    continue;
                }
                chars.push_back(text(std::string(1, ch)) | bold | color(kRainbow[colorIndex % kRainbow.size()]));
                ++colorIndex;
            }
            return hbox(std::move(chars));
        }

        ftxui::Element blockArtWord(const std::string &word, const ftxui::Color &baseColor, const bool rainbow) {
            using namespace ftxui;
            std::string core     = word;
            bool superscriptPlus = false;
            if (!core.empty() && core.back() == '+'){
                superscriptPlus = true;
                core.pop_back();
            }

            std::array<std::string, 6> lines = {"", "", "", "", "", ""};
            for (char ch: core){
                const char up = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                const auto it = kBlockGlyph.find(up);
                if (it == kBlockGlyph.end()) continue;
                for (std::size_t i = 0; i < lines.size(); ++i){
                    lines[i] += it->second[i] + std::string(" ");
                }
            }
            if (superscriptPlus){
                const auto plusIt = kBlockGlyph.find('+');
                if (plusIt != kBlockGlyph.end()){
                    for (std::size_t i = 0; i < lines.size(); ++i){
                        if (i < 4){
                            lines[i] += plusIt->second[i];
                        }
                    }
                }
            }

            return vbox({
                            colorizedToken(lines[0], baseColor, rainbow),
                            colorizedToken(lines[1], baseColor, rainbow),
                            colorizedToken(lines[2], baseColor, rainbow),
                            colorizedToken(lines[3], baseColor, rainbow),
                            colorizedToken(lines[4], baseColor, rainbow),
                            colorizedToken(lines[5], baseColor, rainbow),
                        });
        }

        ftxui::Element renderMetaPanel(const ThemePalette &palette,
                                       const std::function<std::string(const std::string &)> &tr,
                                       const RightPanelState &state) {
            using namespace ftxui;

            auto infoRow = [&](const std::string &labelKey, const std::string &value) {
                return hbox({
                    text(tr(labelKey)) | color(toColor(palette.textMuted)),
                    text(value) | bold | color(toColor(palette.textPrimary)),
                });
            };

            Element metaPanel = vbox({
                infoRow("ui.chart_select.meta.name", state.nameText),
                infoRow("ui.chart_select.meta.artist", state.artistText),
                infoRow("ui.chart_select.meta.charter", state.charterText),
                infoRow("ui.chart_select.meta.difficulty", state.diffText),
                infoRow("ui.chart_select.meta.keys", state.keyText),
            });

            return window(text("  " + tr("ui.chart_select.meta.title") + " "), metaPanel) |
                   color(toColor(palette.accentPrimary)) |
                   bgcolor(toColor(palette.surfacePanel)) |
                   size(HEIGHT, LESS_THAN, 12);
        }

        ftxui::Element renderScorePanel(const ThemePalette &palette,
                                        const std::function<std::string(const std::string &)> &tr,
                                        const RightPanelState &state) {
            using namespace ftxui;

            auto scoreInfoRow = [&](const std::string &labelKey, const std::string &value) {
                return hbox({
                    text(tr(labelKey)) | color(toColor(palette.textMuted)),
                    filler(),
                    text(value) | bold | color(toColor(palette.textPrimary)),
                });
            };

            Elements scoreRows = {
                scoreInfoRow("ui.chart_select.score.play_count", state.playCountText),
                scoreInfoRow("ui.chart_select.score.best_score", state.bestScoreText),
                scoreInfoRow("ui.chart_select.score.best_accuracy", state.bestAccText),
            };
            if (state.hasPlayRecord) {
                scoreRows.push_back(scoreInfoRow("ui.chart_select.score.best_rating", state.bestRatingText));
            }

            Element artBlock = text("");
            if (state.hasPlayRecord) {
                Elements lines;
                lines.push_back(blockArtWord(state.currentGrade.label,
                                             state.currentGrade.color,
                                             state.currentGrade.rainbow));
                if (!state.currentAchievement.label.empty()) {
                    lines.push_back(blockArtWord(state.currentAchievement.label,
                                                 state.currentAchievement.color,
                                                 state.currentAchievement.rainbow));
                }
                artBlock = vbox(std::move(lines));
            }

            Element scorePanel = vbox({
                vbox(std::move(scoreRows)),
                text(""),
                artBlock,
            }) |
            size(WIDTH, GREATER_THAN, 52) |
            flex;

            return window(text("  " + tr("ui.chart_select.score.title") + " "), scorePanel) |
                   color(toColor(palette.accentPrimary)) |
                   bgcolor(toColor(palette.surfacePanel)) |
                   flex;
        }

        ftxui::Element renderLeaderboardPanel(const ThemePalette &palette,
                                              const std::function<std::string(const std::string &)> &tr,
                                              const LeaderboardView &leaderboard,
                                              const bool rankByAccuracy,
                                              const std::string &statsUID) {
            using namespace ftxui;

            Elements rankRows;
            if (leaderboard.top.empty()) {
                rankRows.push_back(text(tr("ui.chart_select.leaderboard.no_records")) |
                                   color(toColor(palette.textMuted)));
            } else {
                for (std::size_t i = 0; i < leaderboard.top.size(); ++i) {
                    const int rank = static_cast<int>(i + 1);
                    const auto &e = leaderboard.top[i];

                    Color rankColor = toColor(palette.textPrimary);
                    if (rank == 1) rankColor = Color::RGB(255, 215, 0);
                    else if (rank == 2) rankColor = Color::RGB(192, 192, 192);
                    else if (rank == 3) rankColor = Color::RGB(205, 127, 50);

                    std::string metric = rankByAccuracy
                                             ? (formatFloat(accuracyPercent(e.accuracy), 2) + "%")
                                             : std::to_string(e.score);

                    Element row = hbox({
                        text(std::to_string(rank) + ".") | color(rankColor),
                        text(" "),
                        text(e.name) | color(rankColor),
                        filler(),
                        text(metric) | color(rankColor),
                    });

                    if (!statsUID.empty() && e.uid == statsUID) {
                        row = row | bgcolor(Color::RGB(45, 45, 45));
                    }
                    rankRows.push_back(row);
                }
            }

            std::string selfText = tr("ui.chart_select.leaderboard.self_rank");
            if (leaderboard.selfRank > 0) {
                const std::string selfMetric = rankByAccuracy
                                                   ? (formatFloat(accuracyPercent(leaderboard.self.accuracy), 2) + "%")
                                                   : std::to_string(leaderboard.self.score);
                selfText += std::to_string(leaderboard.selfRank) + " - " + selfMetric;
            } else {
                selfText += "- -";
            }

            Element leaderboardBody = vbox({
                text(rankByAccuracy
                         ? tr("ui.chart_select.leaderboard.mode_acc")
                         : tr("ui.chart_select.leaderboard.mode_score")) |
                color(toColor(palette.textMuted)),
                separator(),
                vbox(std::move(rankRows)) | frame | flex,
                separator(),
                text(selfText) | color(toColor(palette.textMuted)),
            });

            return window(text("  " + tr("ui.chart_select.leaderboard.title") + " "), leaderboardBody) |
                   color(toColor(palette.accentPrimary)) |
                   bgcolor(toColor(palette.surfacePanel)) |
                   size(WIDTH, EQUAL, 34);
        }

        ftxui::Element renderRightColumn(const ThemePalette &palette,
                                         const std::function<std::string(const std::string &)> &tr,
                                         const RightPanelState &state,
                                         const LeaderboardView &leaderboard,
                                         const bool rankByAccuracy,
                                         const std::string &statsUID) {
            using namespace ftxui;

            Element metaPanel = renderMetaPanel(palette, tr, state);
            Element scorePanel = renderScorePanel(palette, tr, state);
            Element leaderboardPanel = renderLeaderboardPanel(palette, tr, leaderboard, rankByAccuracy, statsUID);

            Element lowerPanels = hbox({
                scorePanel,
                text(" "),
                leaderboardPanel,
            }) |
            flex;

            return vbox({
                metaPanel,
                lowerPanels,
            }) |
            flex;
        }

        ftxui::Element renderChartListTopBar(const ThemePalette &palette,
                                             const std::function<std::string(const std::string &)> &tr) {
            using namespace ftxui;
            return hbox({
                text(tr("ui.chart_select.title")) | bold | color(toColor(palette.accentPrimary)),
                filler(),
                text(tr("ui.chart_select.hint")) | color(toColor(palette.textMuted)),
            });
        }

        ftxui::Element renderStartConfirmOverlay(const ThemePalette &palette,
                                                 const std::function<std::string(const std::string &)> &tr,
                                                 const std::string &pendingStartName) {
            using namespace ftxui;

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
                confirmBody) |
            color(toColor(palette.accentPrimary)) |
            bgcolor(toColor(palette.surfacePanel)) |
            size(WIDTH, EQUAL, 58);

            return hbox({filler(), confirmPanel, filler()}) | vcenter | flex;
        }

        ftxui::Element renderChartListBaseLayout(const ThemePalette &palette,
                                                 const std::function<std::string(const std::string &)> &tr,
                                                 const ftxui::Element &content,
                                                 const bool loadCompleted,
                                                 const std::string &loadFailureMessage,
                                                 const std::size_t chartsLoaded,
                                                 const std::size_t totalChartFiles,
                                                 const std::size_t recordsLoaded,
                                                 const std::size_t totalRecordLines,
                                                 const std::string &selectedText,
                                                 const std::string &rightStatusText) {
            using namespace ftxui;

            Element topBar = renderChartListTopBar(palette, tr);
            Element bottomBar = renderChartListBottomBar(palette,
                                                         loadCompleted,
                                                         loadFailureMessage,
                                                         chartsLoaded,
                                                         totalChartFiles,
                                                         recordsLoaded,
                                                         totalRecordLines,
                                                         selectedText,
                                                         rightStatusText,
                                                         tr);

            return vbox({
                       topBar,
                       separator(),
                       content,
                       separator(),
                       bottomBar,
                   }) |
                   bgcolor(toColor(palette.surfaceBg)) |
                   color(toColor(palette.textPrimary)) |
                   flex;
        }

        ftxui::Element composeChartListScene(const ThemePalette &palette,
                                             const std::function<std::string(const std::string &)> &tr,
                                             const ftxui::Element &base,
                                             const bool showStartConfirm,
                                             const std::string &pendingStartName) {
            if (!showStartConfirm) {
                return ftxui::dbox({base, MessageOverlay::render(palette)});
            }

            ftxui::Element overlay = renderStartConfirmOverlay(palette, tr, pendingStartName);
            return ftxui::dbox({base, overlay, MessageOverlay::render(palette)});
        }

        struct ChartListLoadStartContext {
            ChartListLoadSession &session;
            ftxui::ScreenInteractive &screen;
            const std::string &chartsRoot;
            const std::string &statsUID;
            std::thread &chartLoader;
            std::thread &recordLoader;
        };

        std::thread startChartLoaderThread(ChartListLoadStartContext &ctx) {
            return std::thread([
                session = &ctx.session,
                screen = &ctx.screen,
                chartsRoot = ctx.chartsRoot,
                statsUID = ctx.statsUID
            ] {
                try {
                    std::vector<std::string> ids =
                        discoverChartIDs(chartsRoot, session->chartsLoaded, screen, session->keepRunning);
                    if (!session->keepRunning.load(std::memory_order_relaxed)) return;

                    // Set the total now that discovery is complete (avoids a separate pre-scan).
                    session->totalChartFiles.store(ids.size(), std::memory_order_relaxed);
                    session->chartsLoaded.store(ids.size(), std::memory_order_relaxed);

                    {
                        std::scoped_lock lock(session->loadMutex);
                        session->progressiveIDs = std::move(ids);
                    }

                    ChartLoadResult result;
                    result.chartList.refresh(chartsRoot, statsUID);
                    std::error_code ec;
                    for (const auto &[id, entry]: result.chartList.items()) {
                        if (!session->keepRunning.load(std::memory_order_relaxed)) return;
                        const auto sz = std::filesystem::file_size(entry.chartFilePath, ec);
                        if (!ec) result.fileSizeCache[id] = formatFileSize(sz);
                        ec.clear();
                    }

                    {
                        std::scoped_lock lock(session->loadMutex);
                        session->pendingChartResult = std::move(result);
                    }

                    session->chartsDone = true;
                    if (session->keepRunning.load(std::memory_order_relaxed)) {
                        screen->PostEvent(ftxui::Event::Custom);
                    }
                } catch (...) {
                    {
                        std::scoped_lock lock(session->loadMutex);
                        session->loadFailed = true;
                        session->loadError = getCurrentExceptionMessage();
                    }
                    session->chartsDone = true;
                    if (session->keepRunning.load(std::memory_order_relaxed)) {
                        screen->PostEvent(ftxui::Event::Custom);
                    }
                }
            });
        }

        std::thread startRecordLoaderThread(ChartListLoadStartContext &ctx) {
            return std::thread([session = &ctx.session, screen = &ctx.screen] {
                try {
                    auto records = AuthenticatedUserService::loadAllVerifiedRecords();
                    // Set totals now that loading is complete (avoids a separate pre-read of proof.db).
                    const std::size_t count = records.size();
                    session->totalRecordLines.store(count, std::memory_order_relaxed);
                    session->recordsLoaded.store(count, std::memory_order_relaxed);
                    {
                        std::scoped_lock lock(session->loadMutex);
                        session->pendingRecords = std::move(records);
                    }
                    session->recordsDone = true;
                    if (session->keepRunning.load(std::memory_order_relaxed)) {
                        screen->PostEvent(ftxui::Event::Custom);
                    }
                } catch (...) {
                    {
                        std::scoped_lock lock(session->loadMutex);
                        session->loadFailed = true;
                        session->loadError = getCurrentExceptionMessage();
                    }
                    session->recordsDone = true;
                    if (session->keepRunning.load(std::memory_order_relaxed)) {
                        screen->PostEvent(ftxui::Event::Custom);
                    }
                }
            });
        }

        void startChartListLoadSession(ChartListLoadStartContext &ctx) {
            std::thread startedChartLoader;
            std::thread startedRecordLoader;
            try {
                startedChartLoader = startChartLoaderThread(ctx);
                startedRecordLoader = startRecordLoaderThread(ctx);
            } catch (...) {
                ctx.session.keepRunning = false;
                ctx.session.loadFailed = true;
                {
                    std::scoped_lock lock(ctx.session.loadMutex);
                    ctx.session.loadError = getCurrentExceptionMessage();
                }
                if (startedChartLoader.joinable()) startedChartLoader.join();
                if (startedRecordLoader.joinable()) startedRecordLoader.join();
                ctx.session.chartsDone = true;
                ctx.session.recordsDone = true;
                return;
            }

            ctx.chartLoader = std::move(startedChartLoader);
            ctx.recordLoader = std::move(startedRecordLoader);
        }

        struct ChartListComponentState {
            ThemePalette palette;
            std::string statsUID;
            ChartListInstance chartList;
            std::vector<std::string> allVerifiedRecords;
            std::unordered_map<std::string, std::string> fileSizeCache;
            ChartListLoadSession session;
            std::thread chartLoader;
            std::thread recordLoader;
            ChartListUiState uiState;
            std::array<SearchModeOption, 3> searchModes = {
                {
                    {ChartSearchMode::DisplayName, "ui.chart_select.search_mode.display_name"},
                    {ChartSearchMode::Artist, "ui.chart_select.search_mode.artist"},
                    {ChartSearchMode::Charter, "ui.chart_select.search_mode.charter"},
                }
            };
            std::array<SortKeyOption, 3> sortKeys = {
                {
                    {
                        ChartListSortKey::DisplayName,
                        "ui.chart_select.sort.display_name_asc",
                        "ui.chart_select.sort.display_name_desc"
                    },
                    {
                        ChartListSortKey::Difficulty,
                        "ui.chart_select.sort.difficulty_asc",
                        "ui.chart_select.sort.difficulty_desc"
                    },
                    {
                        ChartListSortKey::BestAccuracy,
                        "ui.chart_select.sort.best_accuracy_asc",
                        "ui.chart_select.sort.best_accuracy_desc"
                    },
                }
            };
            bool routed = false;
            std::atomic<bool> exitRequested{false};

            ~ChartListComponentState() {
                session.keepRunning = false;
                if (chartLoader.joinable()) chartLoader.join();
                if (recordLoader.joinable()) recordLoader.join();
            }
        };
    } // namespace

    ftxui::Component ChartListUI::component(ftxui::ScreenInteractive &screen,
                                            const std::function<void(UIScene)> &onRoute) {
        using namespace ftxui;

        I18nService::instance().ensureLocaleLoaded(RuntimeConfigs::locale);
        auto tr = [](const std::string &key) {
            return I18nService::instance().get(key);
        };

        auto state = std::make_shared<ChartListComponentState>();
        state->palette = ThemeAdapter::resolveFromRuntime();
        state->uiState.actionStatus = tr("ui.chart_select.action.idle");

        AppDirs::init();
        ProofedRecordsDAO::setDataDir(AppDirs::dataDir());

        if (const auto currentUser = AuthenticatedUserService::currentUser();
            currentUser.has_value() && !AuthenticatedUserService::isGuestUser()){
            state->statsUID = std::to_string(currentUser->getUID());
        }
        else{
            const char* uidEnv = std::getenv("TERM4K_UI_UID");
            state->statsUID    = uidEnv ? uidEnv : "";
        }

        ChartListLoadStartContext loadStartContext{
            state->session,
            screen,
            AppDirs::chartsDir(),
            state->statsUID,
            state->chartLoader,
            state->recordLoader,
        };
        startChartListLoadSession(loadStartContext);

        state->chartList.setSearchMode(state->searchModes[state->uiState.searchModeIndex].mode);
        state->chartList.sort(state->sortKeys[state->uiState.sortKeyIndex].key, state->uiState.sortOrder);

        auto routeToStartMenu = [state, onRoute] {
            if (state->routed) return;
            state->routed = true;
            if (onRoute) onRoute(UIScene::StartMenu);
        };

        auto requestExit = [state, routeToStartMenu] {
            bool expected = false;
            if (!state->exitRequested.compare_exchange_strong(expected, true)) return;
            state->session.keepRunning = false;
            routeToStartMenu();
            // routeToStartMenu() posts Event::Custom which drives the deferred
            // scene-swap in UIBus — no need to call ExitLoopClosure() here,
            // which in the single-loop design would terminate the global loop.
        };

        InputOption searchInputOption;
        searchInputOption.placeholder = tr("ui.chart_select.search_placeholder");
        searchInputOption.transform   = [state](const InputState &inputState) {
            // Keep input line background consistent with panel instead of using default white focus style.
            return inputState.element |
                   color(toColor(inputState.is_placeholder ? state->palette.textMuted : state->palette.textPrimary)) |
                   bgcolor(toColor(state->palette.surfacePanel));
        };

        auto searchInput = Input(&state->uiState.searchQuery, searchInputOption);
        auto container   = Container::Vertical({searchInput});

        auto root = Renderer(container, [state, tr, searchInput] {
            return renderChartListFrame(state->palette,
                                        tr,
                                        state->session,
                                        state->chartList,
                                        state->allVerifiedRecords,
                                        state->fileSizeCache,
                                        state->uiState,
                                        state->searchModes,
                                        state->sortKeys,
                                        state->statsUID,
                                        searchInput);
        });

        const std::function<std::string(const std::string &)> trFn = tr;
        const std::function<void()> requestExitFn = requestExit;

        auto app = CatchEvent(root, [state, trFn, requestExitFn, container](const Event &event) mutable {
            if (MessageOverlay::handleEvent(event)) {
                return true;
            }

            ChartListEventContext eventContext{
                state->session,
                state->uiState,
                state->chartList,
                state->searchModes,
                state->sortKeys,
                trFn,
                requestExitFn,
                container,
            };


            if (dispatchChartListEvent(event, eventContext)) {
                return true;
            }

            return false;
        });

        return app;
    }
} // namespace ui
