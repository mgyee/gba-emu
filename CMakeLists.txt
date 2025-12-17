cmake_minimum_required(VERSION 3.10)

project(gba-emu C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

file(GLOB SOURCES "src/*.c")

add_executable(gba-emu ${SOURCES})

target_include_directories(gba-emu PRIVATE include ${SDL2_INCLUDE_DIRS})

target_compile_options(gba-emu PRIVATE -Wall -Wextra)

target_link_libraries(gba-emu PRIVATE ${SDL2_LIBRARIES})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(gba-emu PRIVATE DEBUG)
endif()
