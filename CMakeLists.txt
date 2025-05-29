cmake_minimum_required(VERSION 3.16.3)

project(faris-engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(faris-engine fen.cpp main.cpp movegen.cpp utilities.cpp)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(CTest)

add_executable(faris-engine-tests fen.cpp movegen.cpp utilities.cpp tests/perft_test_case.cpp tests/test_perft.cpp)
target_link_libraries(faris-engine-tests PRIVATE gtest_main)
target_include_directories(faris-engine-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set(TEST_DATA_FILE_NAME "perft_test_data.txt")
set(TEST_DATA_SOURCE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tests/${TEST_DATA_FILE_NAME}")
set(TEST_DATA_DEST_PATH "${CMAKE_CURRENT_BINARY_DIR}/${TEST_DATA_FILE_NAME}")

add_custom_command(
    OUTPUT ${TEST_DATA_DEST_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TEST_DATA_SOURCE_PATH} ${TEST_DATA_DEST_PATH}
    DEPENDS ${TEST_DATA_SOURCE_PATH}
    COMMENT "Copying test data file ${TEST_DATA_FILE_NAME} to build directory"
)

add_custom_target(CopyTestData ALL DEPENDS ${TEST_DATA_DEST_PATH})
add_dependencies(faris-engine-tests CopyTestData)

add_test(NAME PerftTests COMMAND faris-engine-tests)
