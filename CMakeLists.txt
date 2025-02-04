cmake_minimum_required(VERSION 3.14)

# Set the project name and version
project(NES_EMU VERSION 1.0)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "-g -Wall")

# Enable testing
enable_testing()

# Use FetchContent to download Google Test automatically
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Function to simplify test target creation
function(add_nes_test TEST_NAME TEST_SRC MODULE_SRC)
    add_executable(${TEST_NAME} ${TEST_SRC} ${MODULE_SRC})
    target_link_libraries(${TEST_NAME} PRIVATE gtest_main)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Add tests using the function
add_nes_test(runCPUTests tests/CPU_test.cpp src/CPU.cpp)
# add_nes_test(runPPUTests tests/PPU_test.cpp src/PPU.cpp)
# add_nes_test(runAPUTests tests/APU_test.cpp src/APU.cpp)