cmake_minimum_required(VERSION 3.28)
project(term4k VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Release-build optimisation ──────────────────────────────────────────────
# Interprocedural optimisation (LTO) for all targets that opt in.
include(CheckIPOSupported)
check_ipo_supported(RESULT TERM4K_IPO_SUPPORTED OUTPUT _ipo_err)

# Per-target Release flags for GCC / Clang.
# (MSVC sets /O2 by default in Release; no extra action needed there.)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options($<$<CONFIG:Release>:-O3>
                        $<$<CONFIG:Release>:-DNDEBUG>)
endif()
# ────────────────────────────────────────────────────────────────────────────

find_package(OpenSSL REQUIRED)

include(GNUInstallDirs)
include(FetchContent)

# Centralize all FetchContent dependencies under the repo root so multiple
# cmake-build-* directories do not duplicate downloaded sources/sub-builds.
set(TERM4K_FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.cmake-deps"
    CACHE PATH "Shared directory for FetchContent dependencies")
file(MAKE_DIRECTORY "${TERM4K_FETCHCONTENT_BASE_DIR}")
set(FETCHCONTENT_BASE_DIR "${TERM4K_FETCHCONTENT_BASE_DIR}"
    CACHE PATH "FetchContent base directory" FORCE)
message(STATUS "term4k deps: FetchContent base dir = ${FETCHCONTENT_BASE_DIR}")

option(TERM4K_ENABLE_RPM "Enable RPM package generation when rpmbuild is available" ON)
option(TERM4K_STRICT_RPM_CHECK "Fail configure when TERM4K_ENABLE_RPM=ON but rpmbuild is missing" OFF)
set(TERM4K_ENABLE_TUI ON CACHE BOOL "Enable FTXUI-based terminal homepage UI" FORCE)
option(TERM4K_USE_SYSTEM_DEPS "Prefer system-installed dependencies before FetchContent fallback" OFF)

if (TERM4K_USE_SYSTEM_DEPS)
    find_path(TERM4K_SYSTEM_MINIAUDIO_INCLUDE_DIR miniaudio.h)
endif ()

# ── stb (single-header image loader, used by CoverArtRenderer) ──────────────
FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG        31c1ad37456438565541f4919958214b6e762fb4  # stb_image v2.30
)
FetchContent_MakeAvailable(stb)
set(TERM4K_STB_INCLUDE_DIR "${stb_SOURCE_DIR}")
message(STATUS "term4k deps: using FetchContent stb from ${TERM4K_STB_INCLUDE_DIR}")
# ────────────────────────────────────────────────────────────────────────────

if (TERM4K_SYSTEM_MINIAUDIO_INCLUDE_DIR)
    set(TERM4K_MINIAUDIO_IMPL "${CMAKE_BINARY_DIR}/term4k_miniaudio_impl.c")
    file(WRITE "${TERM4K_MINIAUDIO_IMPL}" "#define MINIAUDIO_IMPLEMENTATION\n#include <miniaudio.h>\n")

    add_library(term4k_miniaudio STATIC "${TERM4K_MINIAUDIO_IMPL}")
    target_include_directories(term4k_miniaudio PUBLIC "${TERM4K_SYSTEM_MINIAUDIO_INCLUDE_DIR}")
    set(TERM4K_MINIAUDIO_TARGET term4k_miniaudio)
    set(TERM4K_MINIAUDIO_INCLUDE_DIR "${TERM4K_SYSTEM_MINIAUDIO_INCLUDE_DIR}")
    message(STATUS "term4k deps: using system miniaudio from ${TERM4K_SYSTEM_MINIAUDIO_INCLUDE_DIR}")
else ()
    FetchContent_Declare(
            miniaudio
            GIT_REPOSITORY https://github.com/mackron/miniaudio.git
            GIT_TAG 0.11.22
    )
    FetchContent_MakeAvailable(miniaudio)
    set(TERM4K_MINIAUDIO_TARGET miniaudio)
    set(TERM4K_MINIAUDIO_INCLUDE_DIR "${miniaudio_SOURCE_DIR}")
    message(STATUS "term4k deps: using FetchContent miniaudio")
endif ()

set(TERM4K_CORE_SOURCES
        src/audio/AudioPlayer.cpp
        src/chart/ChartFileReader.cpp
        src/chart/ChartCatalog.cpp
        src/data/Chart.cpp
        src/data/GameplayChartData.cpp
        src/data/GameplayResult.cpp
        src/data/SettingsDraft.cpp
        src/utils/JsonUtils.cpp
        src/data/Rating.cpp
        src/data/User.cpp
        src/account/RecordStore.cpp
        src/account/UserStore.cpp
        src/utils/LiteDBUtils.cpp
        src/utils/PrefixTrie.cpp
        src/platform/I18n.cpp
        src/account/UserContext.cpp
        src/account/UserSession.cpp
        src/gameplay/GameplayChartParser.cpp
        src/gameplay/GameplayStats.cpp
        src/gameplay/GameplayClock.cpp
        src/gameplay/GameplaySession.cpp
        src/gameplay/GameplayRecordWriter.cpp
        src/platform/SettingsIO.cpp
        src/platform/ThemeStore.cpp
        src/scenes/ChartListScene.cpp
        src/scenes/UserStatScene.cpp
        src/scenes/AdminStatScene.cpp
        src/scenes/GameplaySettlement.cpp
        src/scenes/SettingsScene.cpp
        src/utils/ErrorNotifier.cpp
        src/platform/AppDirs.cpp
        src/platform/RuntimeConfig.cpp
        src/ui/ThemeAdapter.cpp)

add_library(term4k_core STATIC ${TERM4K_CORE_SOURCES})
add_library(term4k::core ALIAS term4k_core)
target_include_directories(term4k_core
        PUBLIC
        src
        ${TERM4K_MINIAUDIO_INCLUDE_DIR})
target_link_libraries(term4k_core
        PUBLIC
        OpenSSL::Crypto
        ${TERM4K_MINIAUDIO_TARGET})

# Enable LTO for term4k_core in Release builds when supported.
if(TERM4K_IPO_SUPPORTED)
    set_property(TARGET term4k_core PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()

add_executable(term4k src/main.cpp)
target_compile_definitions(term4k PRIVATE TERM4K_VERSION="${PROJECT_VERSION}")
target_link_libraries(term4k PRIVATE term4k::core)

# Enable LTO for the main executable in Release builds when supported.
if(TERM4K_IPO_SUPPORTED)
    set_property(TARGET term4k PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()

if (TERM4K_ENABLE_TUI)
    if (TERM4K_USE_SYSTEM_DEPS)
        find_package(ftxui CONFIG QUIET)
    endif ()

    if (NOT TARGET ftxui::screen)
        FetchContent_Declare(
                ftxui
                GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
                GIT_TAG v6.1.9
        )
        FetchContent_MakeAvailable(ftxui)
        message(STATUS "term4k deps: using FetchContent FTXUI")
    else ()
        message(STATUS "term4k deps: using system FTXUI")
    endif ()

    target_sources(term4k PRIVATE
            src/ui/StartMenuUI.cpp
            src/ui/MessageOverlay.cpp
            src/ui/TransitionBackdrop.cpp
            src/ui/ChartListUI.cpp
            src/ui/CoverArtRenderer.cpp
            src/ui/TerminalImageProtocol.cpp
            src/ui/SettingsUI.cpp
            src/ui/UserStatUI.cpp
            src/ui/GameplayUI.cpp
            src/ui/GameplaySettlementUI.cpp
            src/ui/UIColors.cpp
            src/ui/UIBus.cpp
    )

    target_include_directories(term4k PRIVATE "${TERM4K_STB_INCLUDE_DIR}")

    target_compile_definitions(term4k PRIVATE TERM4K_ENABLE_TUI=1)
    target_link_libraries(term4k PRIVATE ftxui::screen ftxui::dom ftxui::component)
endif ()

if (TERM4K_BUILD_TESTS)
    if (TERM4K_USE_SYSTEM_DEPS)
        find_package(Catch2 3 CONFIG QUIET)
    endif ()

    if (NOT TARGET Catch2::Catch2WithMain)
        FetchContent_Declare(
                catch2
                GIT_REPOSITORY https://github.com/catchorg/Catch2.git
                GIT_TAG v3.7.1
        )
        FetchContent_MakeAvailable(catch2)
        message(STATUS "term4k deps: using FetchContent Catch2")
    else ()
        message(STATUS "term4k deps: using system Catch2")
    endif ()

    enable_testing()

    add_executable(term4k_tests
            tests/unit/utils/PrefixTrieTests.cpp
            tests/unit/utils/ErrorNotifierTests.cpp
            tests/unit/utils/LiteDBUtilsTests.cpp
            tests/unit/platform/AppDirsTests.cpp
            tests/unit/platform/RuntimeConfigTests.cpp
            tests/unit/platform/I18nTests.cpp
            tests/unit/chart/ChartFileReaderTests.cpp
            tests/unit/chart/ChartCatalogTests.cpp
            tests/unit/audio/AudioPlayerTests.cpp
            tests/unit/account/UserSessionTests.cpp
            tests/unit/account/UserContextTests.cpp
            tests/unit/gameplay/GameplayRecordWriterTests.cpp
            tests/unit/platform/ThemeStoreTests.cpp
            tests/unit/scenes/ChartListSceneTests.cpp
            tests/unit/scenes/UserStatSceneTests.cpp
            tests/unit/scenes/AdminStatSceneTests.cpp
            tests/unit/gameplay/GameplaySessionTests.cpp
            tests/unit/scenes/GameplaySettlementTests.cpp
            tests/unit/scenes/SettingsSceneTests.cpp
    )

    target_include_directories(term4k_tests PRIVATE tests)
    target_link_libraries(term4k_tests PRIVATE term4k::core Catch2::Catch2WithMain)

    add_test(NAME term4k_tests COMMAND term4k_tests)
    add_test(NAME term4k_tests_utils COMMAND term4k_tests "[utils]")
    add_test(NAME term4k_tests_services COMMAND term4k_tests "[services]")
endif ()

# -- Install rules -------------------------------------------------------------

# Runtime: binary -> /usr/bin/term4k
install(TARGETS term4k
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime)

# Runtime: locale files -> /usr/share/term4k/i18n/
install(DIRECTORY src/resources/i18n/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/term4k/i18n
        COMPONENT Runtime)

# Runtime: theme preset files -> /usr/share/term4k/themes/
install(DIRECTORY src/resources/themes/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/term4k/themes
        COMPONENT Runtime)

# Runtime: create empty chart directory skeleton -> /usr/share/term4k/charts/
# (actual chart files are provided by users)
install(CODE
    "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_FULL_DATADIR}/term4k/charts\")"
    COMPONENT Runtime)

# Development package: public headers -> /usr/include/term4k/
set(TERM4K_PUBLIC_HEADERS
        src/platform/AppDirs.h
        src/platform/RuntimeConfig.h
        src/chart/ChartFileReader.h
        src/chart/ChartCatalog.h
        src/platform/I18n.h
        src/account/UserContext.h
        src/account/UserSession.h
        src/audio/AudioPlayer.h
        src/gameplay/GameplayChartParser.h
        src/gameplay/GameplayStats.h
        src/gameplay/GameplayClock.h
        src/gameplay/GameplaySession.h
        src/gameplay/GameplayRecordWriter.h
        src/platform/SettingsIO.h
        src/platform/ThemeStore.h
        src/scenes/ChartListScene.h
        src/scenes/UserStatScene.h
        src/scenes/AdminStatScene.h
        src/scenes/GameplaySettlement.h
        src/scenes/SettingsScene.h
        src/account/RecordStore.h
        src/account/UserStore.h
        src/utils/LiteDBUtils.h
        src/data/Chart.h
        src/data/GameplayResult.h
        src/data/SettingsDraft.h
        src/utils/JsonUtils.h
        src/data/Rating.h
        src/data/User.h
        src/data/GameplayChartData.h
        src/utils/ErrorNotifier.h
)

install(FILES ${TERM4K_PUBLIC_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/term4k
        COMPONENT Development)

# -- CPack / Linux packaging ---------------------------------------------------

set(CPACK_PACKAGE_NAME        "term4k")
set(CPACK_PACKAGE_VERSION     "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A terminal-based rhythm-game framework")
set(CPACK_PACKAGE_CONTACT     "term4k contributors")

# Component definitions
set(CPACK_COMPONENTS_ALL Runtime Development)
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME     "term4k runtime")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION
        "The term4k binary, locale files under /usr/share/term4k/i18n, and chart directory skeleton.")
set(CPACK_COMPONENT_DEVELOPMENT_DISPLAY_NAME "term4k development files")
set(CPACK_COMPONENT_DEVELOPMENT_DESCRIPTION
        "Public C++ headers for embedding or extending term4k.")
set(CPACK_COMPONENT_DEVELOPMENT_DEPENDS      Runtime)

# Enable common Linux package formats.
# DEB is always enabled; RPM can be toggled with TERM4K_ENABLE_RPM.
set(TERM4K_CPACK_GENERATORS DEB)
if (TERM4K_ENABLE_RPM)
    find_program(TERM4K_RPMBUILD_EXECUTABLE rpmbuild)
    if (TERM4K_RPMBUILD_EXECUTABLE)
        list(APPEND TERM4K_CPACK_GENERATORS RPM)
        message(STATUS "term4k packaging: RPM enabled (rpmbuild found).")
    else()
        if (TERM4K_STRICT_RPM_CHECK)
            message(FATAL_ERROR "term4k packaging: TERM4K_ENABLE_RPM=ON and TERM4K_STRICT_RPM_CHECK=ON, but rpmbuild was not found.")
        else()
            message(WARNING "term4k packaging: TERM4K_ENABLE_RPM=ON but rpmbuild was not found; generating DEB packages only.")
        endif()
    endif()
else()
    message(STATUS "term4k packaging: RPM disabled (TERM4K_ENABLE_RPM=OFF).")
endif()
list(JOIN TERM4K_CPACK_GENERATORS ";" CPACK_GENERATOR)
message(STATUS "term4k packaging: CPack generators = ${CPACK_GENERATOR}")
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)

set(CPACK_DEBIAN_PACKAGE_SECTION      "games")
set(CPACK_DEBIAN_PACKAGE_PRIORITY     "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Per-component DEB package names
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME     "term4k")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_NAME "term4k-dev")

# Runtime shared-library dependencies are generated automatically via shlibdeps.

# Development package dependencies:
#   term4k       - runtime package (local integration/testing)
#   libssl-dev   - OpenSSL development headers
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_DEPENDS
        "term4k (= ${PROJECT_VERSION}), libssl-dev")

# RPM metadata and dependencies
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games")
set(CPACK_RPM_RUNTIME_PACKAGE_NAME "term4k")
set(CPACK_RPM_DEVELOPMENT_PACKAGE_NAME "term4k-devel")
set(CPACK_RPM_RUNTIME_PACKAGE_REQUIRES
        "openssl >= 3.0.0, glibc >= 2.35, libstdc++ >= 12")
set(CPACK_RPM_DEVELOPMENT_PACKAGE_REQUIRES
        "term4k = ${PROJECT_VERSION}, openssl-devel")

# RPM post-install script creates /var/lib/term4k/ and /etc/term4k/
# and applies expected ownership/permissions.
set(CPACK_RPM_RUNTIME_POST_INSTALL_SCRIPT_FILE
        "${CMAKE_SOURCE_DIR}/packaging/postinst_rpm")

# DEB post-install script creates /var/lib/term4k/ and /etc/term4k/
# and applies expected ownership/permissions.
set(CPACK_DEBIAN_RUNTIME_PACKAGE_CONTROL_EXTRA
        "${CMAKE_SOURCE_DIR}/packaging/postinst")

include(CPack)
