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
# FetchContent_Declare(
#   googletest
#   GIT_REPOSITORY https://github.com/google/googletest.git
#   GIT_TAG v1.15.2
#   GIT_SHALLOW OFF
# )
# FetchContent_MakeAvailable(googletest)

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
# Fetch SDL https://www.libsdl.org 
# ------------------------------------------------
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.8
)
FetchContent_MakeAvailable(SDL3)

# ------------------------------------------------
# Main executable target
# ------------------------------------------------
add_executable(nesemu
  src/CPU/CPU.cpp
  src/CPU/OpCode.cpp
  src/PPU/PPU.cpp
  src/PPU/Registers/PPUAddr.cpp
  src/Renderer/Renderer.cpp
  src/Cartridge.cpp
  src/Clock.cpp
  src/Emulator.cpp
  src/Logger.cpp
)
target_include_directories(nesemu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(nesemu PRIVATE -g -Wall)
target_link_libraries(nesemu PRIVATE nlohmann_json::nlohmann_json SDL3::SDL3)

# ------------------------------------------------
# Helper function to create tests
# ------------------------------------------------
# function(add_nes_test TEST_NAME TEST_SRC)
#   add_executable(${TEST_NAME} ${TEST_SRC} ${ARGN})

#   target_link_libraries(${TEST_NAME}
#     PRIVATE
#     gtest_main
#     nlohmann_json::nlohmann_json
#     SDL3::SDL3
#   )

#   add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
# endfunction()

# # ------------------------------------------------
# # Create your test targets
# # ------------------------------------------------
# add_nes_test(runCPUHarteTests
#   src/CPU/CPU.cpp
#   src/CPU/OpCode.cpp
#   src/PPU/PPU.cpp
#   src/PPU/Registers/PPUAddr.cpp
#   src/Logger.cpp
#   src/Cartridge.cpp
#   tests/CPU/CPU_Harte.cpp
# )

# add_nes_test(runCPUNestest
#   src/CPU/CPU.cpp
#   src/CPU/OpCode.cpp
#   src/PPU/PPU.cpp
#   src/PPU/Registers/PPUAddr.cpp
#   src/Logger.cpp
#   src/Cartridge.cpp
#   tests/CPU/CPU_Nestest.cpp
# )

# add_nes_test(runPPUTests
#   src/Cartridge.cpp
#   src/PPU/PPU.cpp
#   src/PPU/Registers/PPUAddr.cpp
#   tests/PPU/PPU_core_logic_test.cpp
# )

# add_nes_test(runPPUTests tests/PPU_test.cpp src/PPU.cpp)
# add_nes_test(runAPUTests tests/APU_test.cpp src/APU.cpp)