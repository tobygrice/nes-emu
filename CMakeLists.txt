cmake_minimum_required(VERSION 3.14)

project(NES_EMU VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-g -Wall")

enable_testing()

include(FetchContent)

# ------------------------------------------------
# Fetch GoogleTest
# ------------------------------------------------
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main
)
FetchContent_MakeAvailable(googletest)

# ------------------------------------------------
# Fetch nlohmann/json
# ------------------------------------------------
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# ------------------------------------------------
# Define a helper function to create tests
# ------------------------------------------------
function(add_nes_test TEST_NAME TEST_SRC)
    add_executable(${TEST_NAME} ${TEST_SRC} ${ARGN})
    
    # Link with gtest and nlohmann_json.
    # This is crucial so the compiler finds <nlohmann/json.hpp>.
    target_link_libraries(${TEST_NAME}
        PRIVATE
            gtest_main
            nlohmann_json::nlohmann_json
    )
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# ------------------------------------------------
# Create your test targets
# ------------------------------------------------
add_nes_test(runHarteCPUTests
    src/CPU.cpp
    src/OpCode.cpp
    src/Logger.cpp
    tests/CPU/CPU_Harte.cpp
)

add_nes_test(runCustomCPUTests
    src/CPU.cpp
    src/OpCode.cpp
    src/Logger.cpp
    tests/CPU/CPU_arithmetic_test.cpp
    tests/CPU/CPU_branching_test.cpp
    tests/CPU/CPU_compare_test.cpp
    tests/CPU/CPU_control_test.cpp
    tests/CPU/CPU_core_test.cpp
    tests/CPU/CPU_flag_test.cpp
    tests/CPU/CPU_loadstore_test.cpp
    tests/CPU/CPU_logical_test.cpp
    tests/CPU/CPU_shiftrotate_test.cpp
    tests/CPU/CPU_transferstack_test.cpp
)

# add_nes_test(runPPUTests tests/PPU_test.cpp src/PPU.cpp)
# add_nes_test(runAPUTests tests/APU_test.cpp src/APU.cpp)