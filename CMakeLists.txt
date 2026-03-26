cmake_minimum_required(VERSION 3.28)
project(term4k VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)

include(GNUInstallDirs)

option(TERM4K_BUILD_TESTS "Build term4k unit tests" ON)

add_executable(term4k main.cpp
        src/services/AudioService.cpp
        includes/miniaudio_impl.cpp
        src/services/ChartIOService.cpp
        src/services/ChartCatalogService.cpp
        src/entities/Chart.cpp
        src/utils/JsonUtils.cpp
        src/entities/Rating.cpp
        src/entities/User.cpp
        src/dao/ProofedRecordsDAO.cpp
        src/dao/UserAccountsDAO.cpp
        src/utils/LiteDBUtils.cpp
        src/utils/PrefixTrie.cpp
        src/services/I18nService.cpp
        src/services/AuthenticatedUserService.cpp
        src/services/UserLoginService.cpp
        src/instances/ChartListInstance.cpp
        src/instances/UserLoginInstance.cpp
        src/instances/UserStatInstance.cpp
        src/instances/AdminStatInstance.cpp
        src/utils/ErrorNotifier.cpp
        src/config/AppDirs.cpp
        src/config/RuntimeConfigs.cpp
        src/config/RuntimeConfigs.h
)

target_link_libraries(term4k OpenSSL::Crypto)
target_include_directories(term4k PRIVATE includes src)

if (TERM4K_BUILD_TESTS)
    enable_testing()

    add_executable(term4k_tests
            includes/catch_amalgamated.cpp
            test/test_main.cpp
            test/unit/utils/PrefixTrieTests.cpp
            test/unit/utils/ErrorNotifierTests.cpp
            test/unit/utils/LiteDBUtilsTests.cpp
            test/unit/utils/AppDirsTests.cpp
            test/unit/config/RuntimeConfigsTests.cpp
            test/unit/services/I18nTests.cpp
            test/unit/services/ChartIOTests.cpp
            test/unit/services/ChartCatalogServiceTests.cpp
            test/unit/services/SongPlayerTests.cpp
            test/unit/services/UserServiceTests.cpp
            test/unit/services/AutheticatedUserServiceTests.cpp
            test/unit/instances/ChartListInstanceTests.cpp
            test/unit/instances/UserLoginInstanceTests.cpp
            test/unit/instances/UserStatInstanceTests.cpp
            test/unit/instances/AdminStatInstanceTests.cpp
            src/utils/PrefixTrie.cpp
            src/utils/ErrorNotifier.cpp
            src/utils/LiteDBUtils.cpp
            src/config/AppDirs.cpp
            src/config/RuntimeConfigs.cpp
            src/services/I18nService.cpp
            src/services/ChartIOService.cpp
            src/services/ChartCatalogService.cpp
            src/services/AuthenticatedUserService.cpp
            src/services/AudioService.cpp
            src/services/UserLoginService.cpp
            src/instances/ChartListInstance.cpp
            src/instances/UserLoginInstance.cpp
            src/instances/UserStatInstance.cpp
            src/instances/AdminStatInstance.cpp
            src/dao/ProofedRecordsDAO.cpp
            src/dao/UserAccountsDAO.cpp
            src/entities/Chart.cpp
            src/utils/JsonUtils.cpp
            src/entities/User.cpp
            includes/miniaudio_impl.cpp
    )

    target_include_directories(term4k_tests PRIVATE includes src test)
    target_link_libraries(term4k_tests PRIVATE OpenSSL::Crypto)

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
install(DIRECTORY i18n/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/term4k/i18n
        COMPONENT Runtime)

# Runtime: create empty chart directory skeleton -> /usr/share/term4k/charts/
# (actual chart files are provided by users)
install(CODE
    "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/term4k/charts\")"
    COMPONENT Runtime)

# Development package: public headers -> /usr/include/term4k/
set(TERM4K_PUBLIC_HEADERS
        src/config/AppDirs.h
        src/config/RuntimeConfigs.h
        src/services/ChartIOService.h
        src/services/ChartCatalogService.h
        src/services/I18nService.h
        src/services/AuthenticatedUserService.h
        src/services/UserLoginService.h
        src/services/AudioService.h
        src/instances/ChartListInstance.h
        src/instances/UserLoginInstance.h
        src/instances/UserStatInstance.h
        src/instances/AdminStatInstance.h
        src/dao/ProofedRecordsDAO.h
        src/dao/UserAccountsDAO.h
        src/utils/LiteDBUtils.h
        src/entities/Chart.h
        src/utils/JsonUtils.h
        src/entities/Rating.h
        src/entities/User.h
        src/utils/ErrorNotifier.h)

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
# DEB is enabled by default; RPM is added if rpmbuild is available.
set(CPACK_GENERATOR "DEB")
find_program(TERM4K_RPMBUILD_EXECUTABLE rpmbuild)
if (TERM4K_RPMBUILD_EXECUTABLE)
    set(CPACK_GENERATOR "DEB;RPM")
endif()
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)

set(CPACK_DEBIAN_PACKAGE_SECTION      "games")
set(CPACK_DEBIAN_PACKAGE_PRIORITY     "optional")
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
else()
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")
endif()

# Per-component DEB package names
set(CPACK_DEBIAN_RUNTIME_PACKAGE_NAME     "term4k")
set(CPACK_DEBIAN_DEVELOPMENT_PACKAGE_NAME "term4k-dev")

# Runtime dependencies:
#   libssl3      - OpenSSL 3.x runtime (AES, SHA-256)
#   libc6        - GNU C runtime
#   libstdc++6   - GNU C++ standard library runtime
set(CPACK_DEBIAN_RUNTIME_PACKAGE_DEPENDS
        "libssl3 (>= 3.0.0), libc6 (>= 2.35), libstdc++6 (>= 12)")

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

# DEB post-install script creates /var/lib/term4k/ and /etc/term4k/
# and applies expected ownership/permissions.
set(CPACK_DEBIAN_RUNTIME_PACKAGE_CONTROL_EXTRA
        "${CMAKE_SOURCE_DIR}/packaging/postinst")

include(CPack)
