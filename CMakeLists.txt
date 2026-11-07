cmake_minimum_required(VERSION 3.16)

project(bulls_and_cows VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(game_common SHARED
    src/common/game.c
)

target_include_directories(game_common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(game_common
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

include(CTest)

if(BUILD_TESTING)
    add_executable(game_common_tests
        tests/test_game.c
    )

    target_link_libraries(game_common_tests PRIVATE game_common)
    target_compile_options(game_common_tests PRIVATE -Wall -Wextra -Wpedantic)

    add_test(NAME game_common_tests COMMAND game_common_tests)
endif()
