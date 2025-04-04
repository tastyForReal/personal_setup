cmake_minimum_required(VERSION 3.11)

project(setup_desktop_env LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)

add_executable(setup_desktop_env
    main.cpp
    command_executor.cpp
    downloader.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")

target_link_libraries(setup_desktop_env PRIVATE ${CURL_LIBRARIES})