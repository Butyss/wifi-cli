cmake_minimum_required(VERSION 3.10)
project(wifi-cli C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED glib-2.0 gio-2.0 gobject-2.0)

add_executable(wifi-cli
    src/main.cpp
    src/WifiManager.cpp
    src/Network.cpp
    src/WifiTUI.cpp
    src/NmClient.cpp
)

target_include_directories(wifi-cli PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${GLIB_INCLUDE_DIRS}
)

target_link_libraries(wifi-cli rt pthread ncurses ${GLIB_LIBRARIES})

install(TARGETS wifi-cli DESTINATION bin)
install(FILES LICENSE DESTINATION share/wifi-cli)
