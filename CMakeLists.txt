cmake_minimum_required(VERSION 3.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(
    KawaiiTray
    VERSION 1.0.0
    DESCRIPTION "A designable kawaii tray icon for windows"
    HOMEPAGE_URL "https://github.com/zymelaii/KawaiiTray"
    LANGUAGES C CXX
)

add_subdirectory(src)
