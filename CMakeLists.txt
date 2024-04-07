cmake_minimum_required(VERSION 3.19)

project(zulip-tui VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(zulip-tui
        source/main.cpp
)

target_link_libraries(zulip-tui PRIVATE
        ncurses
        ${OPENSSL_LIBRARIES}
        curl
)
