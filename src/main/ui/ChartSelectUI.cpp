#include "ui/ChartSelectUI.h"

#include "config/AppDirs.h"
#include "config/RuntimeConfigs.h"
#include "dao/ProofedRecordsDAO.h"
#include "instances/ChartListInstance.h"
#include "services/AuthenticatedUserService.h"
#include "services/I18nService.h"
#include "ui/UIColors.h"
#include "utils/StringUtils.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace ui {
namespace {


std::string formatFloat(const float value, const int precision = 2) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
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
    if (static_cast<double>(sizeBytes) >= mb) {
        out << (static_cast<double>(sizeBytes) / mb) << "MB";
    } else {
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
    bool rainbow = false;
};

struct AchievementVisual {
    std::string label;
    ftxui::Color color = ftxui::Color::Default;
    bool rainbow = false;
};

struct LeaderboardEntry {
    std::string uid;
    std::string name;
    uint32_t score = 0;
    float accuracy = 0.0f;
};

struct LeaderboardView {
    std::vector<LeaderboardEntry> top;
    int selfRank = -1;
    bool selfInTop = false;
    LeaderboardEntry self;
};


bool parseLeaderboardRecord(const std::string &record,
                            std::string *outUid,
                            std::string *outChartId,
                            std::string *outName,
                            uint32_t *outScore,
                            float *outAccuracy) {
    std::istringstream iss(record);
    std::vector<std::string> fields;
    std::string token;
    while (iss >> token) fields.push_back(token);
    if (fields.size() < 6) return false;

    const bool uidFormat = (fields.size() >= 7) && string_utils::isDigitsOnly(fields[0]);
    if (!uidFormat) return false;

    try {
        *outUid = fields[0];
        *outChartId = fields[1];
        *outName = fields[3];
        *outScore = static_cast<uint32_t>(std::stoul(fields[4]));
        *outAccuracy = std::stof(fields[5]);
        return true;
    } catch (...) {
        return false;
    }
}

LeaderboardView buildLeaderboard(const std::vector<std::string> &records,
                                 const std::string &chartId,
                                 const bool byAccuracy,
                                 const std::string &selfUid) {
    LeaderboardView out;
    if (chartId.empty()) return out;

    std::unordered_map<std::string, LeaderboardEntry> bestByUid;
    for (const auto &raw : records) {
        std::string uid;
        std::string cid;
        std::string name;
        uint32_t score = 0;
        float accuracy = 0.0f;
        if (!parseLeaderboardRecord(raw, &uid, &cid, &name, &score, &accuracy)) continue;
        if (cid != chartId) continue;

        auto it = bestByUid.find(uid);
        if (it == bestByUid.end()) {
            bestByUid[uid] = {uid, name.empty() ? uid : name, score, accuracy};
            continue;
        }

        const bool better = byAccuracy
            ? (accuracy > it->second.accuracy ||
               (accuracy == it->second.accuracy && score > it->second.score))
            : (score > it->second.score ||
               (score == it->second.score && accuracy > it->second.accuracy));

        if (better) {
            it->second.name = name.empty() ? uid : name;
            it->second.score = score;
            it->second.accuracy = accuracy;
        }
    }

    std::vector<LeaderboardEntry> all;
    all.reserve(bestByUid.size());
    for (const auto &it : bestByUid) all.push_back(it.second);

    std::stable_sort(all.begin(), all.end(), [&](const LeaderboardEntry &a, const LeaderboardEntry &b) {
        if (byAccuracy) {
            if (a.accuracy != b.accuracy) return a.accuracy > b.accuracy;
            if (a.score != b.score) return a.score > b.score;
        } else {
            if (a.score != b.score) return a.score > b.score;
            if (a.accuracy != b.accuracy) return a.accuracy > b.accuracy;
        }
        return a.uid < b.uid;
    });

    const std::size_t topCount = std::min<std::size_t>(99, all.size());
    out.top.assign(all.begin(), all.begin() + static_cast<long long>(topCount));

    for (std::size_t i = 0; i < all.size(); ++i) {
        if (!selfUid.empty() && all[i].uid == selfUid) {
            out.selfRank = static_cast<int>(i + 1);
            out.self = all[i];
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
};

ftxui::Element colorizedToken(const std::string &token, const ftxui::Color &baseColor, const bool rainbow) {
    using namespace ftxui;
    if (!rainbow) return text(token) | bold | color(baseColor);

    Elements chars;
    std::size_t colorIndex = 0;
    for (char ch : token) {
        if (ch == ' ') {
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
    std::string core = word;
    bool superscriptPlus = false;
    if (!core.empty() && core.back() == '+') {
        superscriptPlus = true;
        core.pop_back();
    }

    std::array<std::string, 6> lines = {"", "", "", "", "", ""};
    for (char ch : core) {
        const char up = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        const auto it = kBlockGlyph.find(up);
        if (it == kBlockGlyph.end()) continue;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            lines[i] += it->second[i] + std::string(" ");
        }
    }
    if (superscriptPlus) lines[0] += " +";

    return vbox({
        colorizedToken(lines[0], baseColor, rainbow),
        colorizedToken(lines[1], baseColor, rainbow),
        colorizedToken(lines[2], baseColor, rainbow),
        colorizedToken(lines[3], baseColor, rainbow),
        colorizedToken(lines[4], baseColor, rainbow),
        colorizedToken(lines[5], baseColor, rainbow),
    });
}

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
    const std::vector<std::string> allVerifiedRecords = AuthenticatedUserService::loadAllVerifiedRecords();

    // Pre-compute chart file sizes once; avoids a syscall per item per render frame.
    std::unordered_map<std::string, std::string> fileSizeCache;
    {
        std::error_code ec;
        for (const auto &[id, entry] : chartList.items()) {
            const auto sz = std::filesystem::file_size(entry.chartFilePath, ec);
            if (!ec) fileSizeCache[id] = formatFileSize(sz);
            ec.clear();
        }
    }

    // Leaderboard cache: rebuild only when the selected chart or sort mode changes.
    std::string leaderboardCacheKey;
    bool leaderboardCacheByAcc = false;
    LeaderboardView cachedLeaderboard;

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
    bool leaderboardByAccuracy = false;

    auto screen = ScreenInteractive::Fullscreen();
    InputOption searchInputOption;
    searchInputOption.placeholder = tr("ui.chart_select.search_placeholder");
    searchInputOption.transform = [&](const InputState &state) {
        // Keep input line background consistent with panel instead of using default white focus style.
        return state.element |
               color(toColor(state.is_placeholder ? palette.textMuted : palette.textPrimary)) |
               bgcolor(toColor(palette.surfacePanel));
    };

    auto searchInput = Input(&searchQuery, searchInputOption);
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
                std::string sizeText;
                Element rightInfo = text("");

                if (it != chartList.items().end()) {
                    const auto &entry = it->second;
                    if (!entry.chart.getDisplayName().empty()) title = entry.chart.getDisplayName();

                    // Use precomputed size; avoids a per-item syscall every frame.
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
                listRows.push_back(row);
            }
        }

        Element searchBox = vbox({searchInput->Render()});

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
        std::string bestRatingText;
        bool hasPlayRecord = false;
        GradeVisual currentGrade;
        AchievementVisual currentAchievement;

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
                const float bestAccPercent = accuracyPercent(entry.stats.bestAccuracy);
                bestAccText = formatFloat(bestAccPercent, 2) + "%";

                hasPlayRecord = entry.stats.playCount > 0;
                if (hasPlayRecord) {
                    bestRatingText = formatFloat(static_cast<float>(entry.stats.bestSingleRating), 2);
                    currentGrade = gradeFromAccuracy(bestAccPercent);
                    currentAchievement = achievementFromStats(entry.stats);
                }
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

        Elements scoreRows = {
            infoRow("ui.chart_select.score.play_count", playCountText),
            infoRow("ui.chart_select.score.best_score", bestScoreText),
            infoRow("ui.chart_select.score.best_accuracy", bestAccText),
        };
        if (hasPlayRecord) {
            scoreRows.push_back(infoRow("ui.chart_select.score.best_rating", bestRatingText));
        }

        Element artBlock = text("");
        if (hasPlayRecord) {
            Elements lines;
            lines.push_back(blockArtWord(currentGrade.label, currentGrade.color, currentGrade.rainbow));
            if (!currentAchievement.label.empty()) {
                lines.push_back(blockArtWord(currentAchievement.label,
                                             currentAchievement.color,
                                             currentAchievement.rainbow));
            }
            artBlock = vbox(std::move(lines));
        }

        Element scorePanel = hbox({
                                vbox(std::move(scoreRows)),
                                filler(),
                                artBlock,
                            });

        scorePanel = window(text("  " + tr("ui.chart_select.score.title") + " "), scorePanel) |
                     color(toColor(palette.accentPrimary)) |
                     bgcolor(toColor(palette.surfacePanel)) |
                     flex;

        const bool rankByAccuracy = leaderboardByAccuracy;
        const std::string currentChartId = hasSelection ? ids[selectedIndex] : "";

        // Only rebuild the leaderboard when the selected chart or ranking mode changes.
        if (currentChartId != leaderboardCacheKey || rankByAccuracy != leaderboardCacheByAcc) {
            cachedLeaderboard   = buildLeaderboard(allVerifiedRecords, currentChartId, rankByAccuracy, statsUID);
            leaderboardCacheKey = currentChartId;
            leaderboardCacheByAcc = rankByAccuracy;
        }
        const LeaderboardView &leaderboard = cachedLeaderboard;

        Elements rankRows;
        if (leaderboard.top.empty()) {
            rankRows.push_back(text(tr("ui.chart_select.leaderboard.no_records")) | color(toColor(palette.textMuted)));
        } else {
            for (std::size_t i = 0; i < leaderboard.top.size(); ++i) {
                const int rank = static_cast<int>(i + 1);
                const auto &e = leaderboard.top[i];

                ftxui::Color rankColor = toColor(palette.textPrimary);
                if (rank == 1) rankColor = ftxui::Color::RGB(255, 215, 0);
                else if (rank == 2) rankColor = ftxui::Color::RGB(192, 192, 192);
                else if (rank == 3) rankColor = ftxui::Color::RGB(205, 127, 50);

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
                    row = row | bgcolor(ftxui::Color::RGB(45, 45, 45));
                }
                rankRows.push_back(row);
            }
        }

        std::string selfText = tr("ui.chart_select.leaderboard.self_rank");
        if (leaderboard.selfRank > 0) {
            const std::string selfMetric = rankByAccuracy
                ? (formatFloat(accuracyPercent(leaderboard.self.accuracy), 2) + "%")
                : std::to_string(leaderboard.self.score);
            selfText += std::to_string(leaderboard.selfRank) + "  " + selfMetric;
        } else {
            selfText += "-";
        }

        Element leaderboardPanel = vbox({
                                     text(rankByAccuracy
                                              ? tr("ui.chart_select.leaderboard.mode_acc")
                                              : tr("ui.chart_select.leaderboard.mode_score")) |
                                         color(toColor(palette.textMuted)),
                                     separator(),
                                     vbox(std::move(rankRows)) | frame | flex,
                                     separator(),
                                     text(selfText) | color(toColor(palette.textMuted)),
                                 });

        leaderboardPanel = window(text("  " + tr("ui.chart_select.leaderboard.title") + " "), leaderboardPanel) |
                           color(toColor(palette.accentPrimary)) |
                           bgcolor(toColor(palette.surfacePanel)) |
                           size(WIDTH, EQUAL, 44);

        Element lowerPanels = hbox({
                                scorePanel,
                                text(" "),
                                leaderboardPanel,
                            }) | flex;

        Element rightColumn = vbox({
                                 metaPanel,
                                 lowerPanels,
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
                   color(highContrastOn(palette.accentPrimary)) |
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

    auto app = CatchEvent(root, [&](const Event &event) {
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

        if (!focusSearch && (event == Event::Character('l') || event == Event::Character('L'))) {
            leaderboardByAccuracy = !leaderboardByAccuracy;
            actionStatus = tr("ui.chart_select.action.leaderboard_mode_switched") +
                           tr(leaderboardByAccuracy
                                  ? "ui.chart_select.leaderboard.mode_acc"
                                  : "ui.chart_select.leaderboard.mode_score");
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

        if (focusSearch) return container->OnEvent(event);
        return true; // consume all events when list is focused
    });

    screen.Loop(app);
    return 0;
}

} // namespace ui

