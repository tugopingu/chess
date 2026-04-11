cmake_minimum_required(VERSION 3.20)
project(Chess)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 2.6 COMPONENTS graphics window system audio QUIET)

if(NOT SFML_FOUND)
    message(STATUS "SFML not found on system, fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        2.6.1
    )
    FetchContent_MakeAvailable(SFML)
else()
    message(STATUS "Found system SFML, using it.")
endif()

add_executable(chess src/main.cpp)
target_link_libraries(chess PRIVATE sfml-graphics sfml-window sfml-system sfml-audio)
