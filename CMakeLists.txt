cmake_minimum_required(VERSION 3.19)

project(zulip-tui VERSION 0.0.0 LANGUAGES CXX)

add_executable(zulip-tui
        source/main.cpp
)

target_link_libraries(zulip-tui PRIVATE
        ncurses
        curl

)