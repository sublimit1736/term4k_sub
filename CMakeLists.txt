cmake_minimum_required(VERSION 3.28)
project(term4k VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenSSL REQUIRED)

include(GNUInstallDirs)

option(TERM4K_BUILD_TESTS "Build term4k unit tests" ON)

add_executable(term4k src/main/main.cpp
        src/main/services/AudioService.cpp
        includes/miniaudio_impl.cpp
        src/main/services/ChartIOService.cpp
        src/main/services/ChartCatalogService.cpp
        src/main/entities/Chart.cpp
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
        src/main/instances/ChartListInstance.cpp
        src/main/instances/UserLoginInstance.cpp
        src/main/instances/UserStatInstance.cpp
        src/main/instances/AdminStatInstance.cpp
        src/main/utils/ErrorNotifier.cpp
        src/main/config/AppDirs.cpp
        src/main/config/RuntimeConfigs.cpp
        src/main/config/RuntimeConfigs.h
)

target_link_libraries(term4k OpenSSL::Crypto)
target_include_directories(term4k PRIVATE includes src/main)

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
            src/test/unit/instances/ChartListInstanceTests.cpp
            src/test/unit/instances/UserLoginInstanceTests.cpp
            src/test/unit/instances/UserStatInstanceTests.cpp
            src/test/unit/instances/AdminStatInstanceTests.cpp
            src/main/utils/PrefixTrie.cpp
            src/main/utils/ErrorNotifier.cpp
            src/main/utils/LiteDBUtils.cpp
            src/main/config/AppDirs.cpp
            src/main/config/RuntimeConfigs.cpp
            src/main/services/I18nService.cpp
            src/main/services/ChartIOService.cpp
            src/main/services/ChartCatalogService.cpp
            src/main/services/AuthenticatedUserService.cpp
            src/main/services/AudioService.cpp
            src/main/services/UserLoginService.cpp
            src/main/instances/ChartListInstance.cpp
            src/main/instances/UserLoginInstance.cpp
            src/main/instances/UserStatInstance.cpp
            src/main/instances/AdminStatInstance.cpp
            src/main/dao/ProofedRecordsDAO.cpp
            src/main/dao/UserAccountsDAO.cpp
            src/main/entities/Chart.cpp
            src/main/utils/JsonUtils.cpp
            src/main/entities/User.cpp
            includes/miniaudio_impl.cpp
    )

    target_include_directories(term4k_tests PRIVATE includes src/main src/test)
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
install(DIRECTORY src/resources/i18n/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/term4k/i18n
        COMPONENT Runtime)

# Runtime: create empty chart directory skeleton -> /usr/share/term4k/charts/
# (actual chart files are provided by users)
install(CODE
    "file(MAKE_DIRECTORY \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/term4k/charts\")"
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
        src/main/instances/ChartListInstance.h
        src/main/instances/UserLoginInstance.h
        src/main/instances/UserStatInstance.h
        src/main/instances/AdminStatInstance.h
        src/main/dao/ProofedRecordsDAO.h
        src/main/dao/UserAccountsDAO.h
        src/main/utils/LiteDBUtils.h
        src/main/entities/Chart.h
        src/main/utils/JsonUtils.h
        src/main/entities/Rating.h
        src/main/entities/User.h
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
