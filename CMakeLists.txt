cmake_minimum_required(VERSION 3.5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(
    KawaiiTray
    VERSION 1.1.0
    DESCRIPTION "A designable kawaii tray icon for windows"
    HOMEPAGE_URL "https://github.com/zymelaii/KawaiiTray"
    LANGUAGES C CXX
)

find_package(yaml-cpp)
if (NOT yaml-cpp_FOUND)
    add_subdirectory(deps/yaml-cpp)
endif()

add_subdirectory(src)
