cmake_minimum_required(VERSION 3.10)

project(Chess)

find_package(SFML 2.6 COMPONENTS graphics window system audio REQUIRED)

add_executable(chess
    src/main.cpp
)

target_link_libraries(chess
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
)
