cmake_minimum_required(VERSION 3.28)
project(term4k VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)

include(GNUInstallDirs)

option(TERM4K_BUILD_TESTS "Build term4k unit tests" ON)
option(TERM4K_ENABLE_RPM "Enable RPM package generation when rpmbuild is available" ON)
option(TERM4K_STRICT_RPM_CHECK "Fail configure when TERM4K_ENABLE_RPM=ON but rpmbuild is missing" OFF)

set(TERM4K_CORE_SOURCES
        src/main/services/AudioService.cpp
        includes/miniaudio_impl.cpp
        src/main/services/ChartIOService.cpp
        src/main/services/ChartCatalogService.cpp
        src/main/entities/Chart.cpp
        src/main/entities/GameplayChartData.cpp
        src/main/entities/GameplayResult.cpp
        src/main/entities/SettingsDraft.cpp
        src/main/utils/JsonUtils.cpp
        src/main/entities/Rating.cpp
        src/main/entities/User.cpp
        src/main/dao/ProofedRecordsDAO.cpp
        src/main/dao/UserAccountsDAO.cpp
        src/main/utils/LiteDBUtils.cpp
        src/main/utils/PrefixTrie.cpp
        src/main/services/I18nService.cpp
        src/main/services/AuthenticatedUserService.cpp
        src/main/services/UserLoginService.cpp
        src/main/services/GameplayChartService.cpp
        src/main/services/GameplayStatsService.cpp
        src/main/services/GameplayClockService.cpp
        src/main/services/GameplaySessionService.cpp
        src/main/services/GameplayRecordService.cpp
        src/main/services/SettingsService.cpp
        src/main/ui/StartMenuUI.cpp
        src/main/ui/AdminUserStatUI.cpp
        src/main/ui/ChartListUI.cpp
        src/main/ui/GameplayUI.cpp
        src/main/ui/GameplaySettlementUI.cpp
        src/main/ui/UserStatUI.cpp
        src/main/ui/UserLoginUI.cpp
        src/main/ui/UserSettingsUI.cpp
        src/main/ui/UIThreadRuntime.cpp
        src/main/instances/ChartListInstance.cpp
        src/main/instances/UserLoginInstance.cpp
        src/main/instances/UserStatInstance.cpp
        src/main/instances/AdminStatInstance.cpp
        src/main/instances/GameplayInstance.cpp
        src/main/instances/GameplaySettlementInstance.cpp
        src/main/instances/SettingsInstance.cpp
        src/main/utils/ErrorNotifier.cpp
        src/main/config/AppDirs.cpp
        src/main/config/RuntimeConfigs.cpp)

add_library(term4k_core STATIC ${TERM4K_CORE_SOURCES})
add_library(term4k::core ALIAS term4k_core)
target_include_directories(term4k_core PUBLIC includes src/main)
target_link_libraries(term4k_core PUBLIC OpenSSL::Crypto)

add_executable(term4k src/main/main.cpp)
target_link_libraries(term4k PRIVATE term4k::core)

if (TERM4K_BUILD_TESTS)
    enable_testing()

    add_executable(term4k_tests
            includes/catch_amalgamated.cpp
            src/test/test_main.cpp
            src/test/unit/utils/PrefixTrieTests.cpp
            src/test/unit/utils/ErrorNotifierTests.cpp
            src/test/unit/utils/LiteDBUtilsTests.cpp
            src/test/unit/utils/AppDirsTests.cpp
            src/test/unit/config/RuntimeConfigsTests.cpp
            src/test/unit/services/I18nTests.cpp
            src/test/unit/services/ChartIOTests.cpp
            src/test/unit/services/ChartCatalogServiceTests.cpp
            src/test/unit/services/SongPlayerTests.cpp
            src/test/unit/services/UserServiceTests.cpp
            src/test/unit/services/AuthenticatedUserServiceTests.cpp
            src/test/unit/services/GameplayRecordServiceTests.cpp
            src/test/unit/instances/ChartListInstanceTests.cpp
            src/test/unit/instances/UserLoginInstanceTests.cpp
            src/test/unit/instances/UserStatInstanceTests.cpp
            src/test/unit/instances/AdminStatInstanceTests.cpp
            src/test/unit/instances/GameplayInstanceTests.cpp
            src/test/unit/instances/GameplaySettlementInstanceTests.cpp
            src/test/unit/instances/SettingsInstanceTests.cpp
            src/test/unit/ui/UIThreadRuntimeTests.cpp
    )

    target_include_directories(term4k_tests PRIVATE src/test)
    target_link_libraries(term4k_tests PRIVATE term4k::core)

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

# Runtime: create empty chart directory skeleton -> /usr/share/term4k/charts/
# (actual chart files are provided by users)
install(CODE
    "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_FULL_DATADIR}/term4k/charts\")"
    COMPONENT Runtime)

# Development package: public headers -> /usr/include/term4k/
set(TERM4K_PUBLIC_HEADERS
        src/main/config/AppDirs.h
        src/main/config/RuntimeConfigs.h
        src/main/services/ChartIOService.h
        src/main/services/ChartCatalogService.h
        src/main/services/I18nService.h
        src/main/services/AuthenticatedUserService.h
        src/main/services/UserLoginService.h
        src/main/services/AudioService.h
        src/main/services/GameplayChartService.h
        src/main/services/GameplayStatsService.h
        src/main/services/GameplayClockService.h
        src/main/services/GameplaySessionService.h
        src/main/services/GameplayRecordService.h
        src/main/services/SettingsService.h
        src/main/ui/StartMenuUI.h
        src/main/ui/AdminUserStatUI.h
        src/main/ui/ChartListUI.h
        src/main/ui/GameplayUI.h
        src/main/ui/GameplaySettlementUI.h
        src/main/ui/UserStatUI.h
        src/main/ui/UserLoginUI.h
        src/main/ui/UserSettingsUI.h
        src/main/ui/UIThreadRuntime.h
        src/main/instances/ChartListInstance.h
        src/main/instances/UserLoginInstance.h
        src/main/instances/UserStatInstance.h
        src/main/instances/AdminStatInstance.h
        src/main/instances/GameplayInstance.h
        src/main/instances/GameplaySettlementInstance.h
        src/main/instances/SettingsInstance.h
        src/main/dao/ProofedRecordsDAO.h
        src/main/dao/UserAccountsDAO.h
        src/main/utils/LiteDBUtils.h
        src/main/entities/Chart.h
        src/main/entities/GameplayResult.h
        src/main/entities/SettingsDraft.h
        src/main/utils/JsonUtils.h
        src/main/entities/Rating.h
        src/main/entities/User.h
        src/main/entities/GameplayChartData.h
        src/main/utils/ErrorNotifier.h)

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
