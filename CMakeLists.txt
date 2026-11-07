cmake_minimum_required(VERSION 3.16)

project(bulls_and_cows VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_definitions(_POSIX_C_SOURCE=200809L)

add_library(game_common SHARED
    src/common/game.c
    src/common/network_io.c
    src/common/protocol.c
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

    add_executable(protocol_tests
        tests/test_protocol.c
    )

    add_executable(network_io_tests
        tests/test_network_io.c
    )

    target_link_libraries(game_common_tests PRIVATE game_common)
    target_link_libraries(protocol_tests PRIVATE game_common)
    target_link_libraries(network_io_tests PRIVATE game_common)
    target_compile_options(game_common_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(protocol_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(network_io_tests PRIVATE -Wall -Wextra -Wpedantic)

    add_test(NAME game_common_tests COMMAND game_common_tests)
    add_test(NAME protocol_tests COMMAND protocol_tests)
    add_test(NAME network_io_tests COMMAND network_io_tests)
endif()
