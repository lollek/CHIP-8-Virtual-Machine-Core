cmake_minimum_required(VERSION 2.8)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Tests. Turn on with 'cmake -Dtest=ON'.
option(test "Build all tests." OFF)
if (test)
  add_subdirectory(lib/googletest)
  enable_testing()
  include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

  add_executable(test_chip8core test/test_emulator_init.cc
    test/test_emulator_load_file_to_ram.cc
    test/test_emulator_fetch_opcode.cc
    test/test_emulator_handle_opcode.cc)
  target_link_libraries(test_chip8core gtest gtest_main)
  target_link_libraries(test_chip8core chip8core)
  add_test(test_chip8core test_chip8core)
endif()

# Main project - chip8core library
set(PROJECT_NAME chip8core)
project(${PROJECT_NAME})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -std=c++11")
set(PROJECT_SOURCE_DIR src)
set(CHIP8_VERSION_MAJOR 1)
set(CHIP8_VERSION_MINOR 0)

include_directories(${chip8core_SOURCE_DIR}/include)
add_library(chip8core src/Emulator.cc)
