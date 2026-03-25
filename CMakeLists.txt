cmake_minimum_required(VERSION 3.10)
project(wifi-cli C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(wifi-cli
    src/main.cpp
    src/WpaClient.cpp
    src/WifiManager.cpp
    src/Network.cpp
    wpa_supplicant/wpa_ctrl.c
    wpa_supplicant/os_unix.c
)

target_include_directories(wifi-cli PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/wpa_supplicant
)

target_link_libraries(wifi-cli rt pthread)
