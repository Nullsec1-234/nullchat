cmake_minimum_required(VERSION 3.22)
project(chatter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Sql)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(src/common)
add_subdirectory(src/server)
add_subdirectory(src/client)

option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest REQUIRED)
    add_subdirectory(tests)
endif()

# Install rules
install(TARGETS chatter RUNTIME DESTINATION bin)
install(TARGETS chatter-server RUNTIME DESTINATION bin)
install(FILES src/server/node/server.js DESTINATION share/chatter)

# Desktop file
if(UNIX AND NOT APPLE)
    configure_file(packaging/chatter.desktop.in ${CMAKE_CURRENT_BINARY_DIR}/chatter.desktop @ONLY)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/chatter.desktop DESTINATION share/applications)
endif()

# CPack
set(CPACK_PACKAGE_NAME "Nullchat")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Encrypted terminal-style chat application")
set(CPACK_PACKAGE_VENDOR "Nullchat Team")
set(CPACK_PACKAGE_CONTACT "nullchat@example.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Nullchat")
    set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Nullchat.lnk' '$INSTDIR\\\\bin\\\\chatter.exe'"
        "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Nullchat Server.lnk' '$INSTDIR\\\\bin\\\\chatter-server.exe'")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "nullchat@example.com")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "qt6-base-dev (>= 6.4), libssl-dev (>= 3.0), libqt6sql6-sqlite")
endif()

include(CPack)
