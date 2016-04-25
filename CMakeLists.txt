cmake_minimum_required(VERSION 3.0)

# Tests. Turn on with 'cmake -Dtest=ON'.
option(test "Build all tests." OFF)
if (test)
  message(STATUS "Tests enabled")
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
project(chip8core
  VERSION 1.1
  LANGUAGES CXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -std=c++11")
set(PROJECT_SOURCE_DIR src)

include_directories(${chip8core_SOURCE_DIR}/include)
add_library(${PROJECT_NAME} src/Emulator.cc)

# Extra tools. Turn on with 'cmake -Dtools=ON'.
option(tools "Build all extra tools." OFF)
if (tools)
  message(STATUS "Tools enabled")
  add_executable(decompiler src/decompiler.cc)
endif()

