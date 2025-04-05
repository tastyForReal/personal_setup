cmake_minimum_required(VERSION 3.11)
project(setup_desktop_env)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(setup_desktop_env main.cpp command_executor.cpp)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(MAIN_LIBRARIES "-static" ${CURL_STATIC_LIBRARIES})
else()
    set(MAIN_LIBRARIES "")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

target_link_libraries(setup_desktop_env PRIVATE ${MAIN_LIBRARIES})