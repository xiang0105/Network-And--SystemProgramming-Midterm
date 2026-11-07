cmake_minimum_required(VERSION 3.16)

project(bulls_and_cows VERSION 0.1.0 LANGUAGES C)

option(ENABLE_DEBUG_LOG "Compile debug-level logging" ON)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_definitions(_POSIX_C_SOURCE=200809L)

add_library(game_common SHARED
    src/common/game.c
    src/common/log.c
    src/common/network_io.c
    src/common/protocol.c
)

if(ENABLE_DEBUG_LOG)
    target_compile_definitions(game_common PUBLIC ENABLE_DEBUG_LOG=1)
endif()

add_library(server_core STATIC
    src/server/session.c
)

target_include_directories(game_common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(server_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(server_core PUBLIC game_common)

target_compile_options(game_common
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

target_compile_options(server_core PRIVATE -Wall -Wextra -Wpedantic)

add_executable(server src/server/main.c)
target_link_libraries(server PRIVATE server_core)
target_compile_options(server PRIVATE -Wall -Wextra -Wpedantic)

add_executable(client src/client/main.c)
target_link_libraries(client PRIVATE game_common)
target_compile_options(client PRIVATE -Wall -Wextra -Wpedantic)

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

    add_executable(frame_tests
        tests/test_frame.c
    )

    add_executable(session_tests
        tests/test_session.c
    )

    add_executable(robustness_demo
        examples/robustness_demo.c
    )

    target_link_libraries(game_common_tests PRIVATE game_common)
    target_link_libraries(protocol_tests PRIVATE game_common)
    target_link_libraries(network_io_tests PRIVATE game_common)
    target_link_libraries(frame_tests PRIVATE game_common)
    target_link_libraries(session_tests PRIVATE server_core)
    target_link_libraries(robustness_demo PRIVATE game_common)
    target_compile_options(game_common_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(protocol_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(network_io_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(frame_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(session_tests PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(robustness_demo PRIVATE -Wall -Wextra -Wpedantic)

    find_program(BASH_EXECUTABLE bash REQUIRED)

    add_test(NAME game_common_tests COMMAND game_common_tests)
    add_test(NAME protocol_tests COMMAND protocol_tests)
    add_test(NAME network_io_tests COMMAND network_io_tests)
    add_test(NAME frame_tests COMMAND frame_tests)
    add_test(NAME session_tests COMMAND session_tests)
    add_test(
        NAME robustness_comparison
        COMMAND
            ${BASH_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_robustness_demo.sh
            $<TARGET_FILE:robustness_demo>
    )
endif()
