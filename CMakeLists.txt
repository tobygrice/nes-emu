cmake_minimum_required(VERSION 3.14)

project(NES_EMU VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(CTest)
include(FetchContent)

# Build the hardware implementation once, and share it with every test.
add_library(nescore STATIC
  src/CPU/CPU.cpp
  src/CPU/OpCode.cpp
  src/PPU/PPU.cpp
  src/PPU/Registers/PPUAddr.cpp
  src/Cartridge.cpp
  src/Logger.cpp
)
target_include_directories(nescore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

function(nes_warnings TARGET_NAME)
  if(MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()
nes_warnings(nescore)

FetchContent_Declare(SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.2.8
)
FetchContent_MakeAvailable(SDL3)

# Keep SDL dependencies out of the CPU, PPU and cartridge implementation.
add_library(nesfrontend STATIC src/Renderer/Renderer.cpp src/Clock.cpp)
target_link_libraries(nesfrontend PUBLIC nescore SDL3::SDL3)
nes_warnings(nesfrontend)

# Windows loads SDL from beside each executable.
function(copy_sdl_runtime TARGET_NAME)
  get_target_property(SDL_LIBRARY_TYPE SDL3::SDL3 TYPE)
  if(WIN32 AND SDL_LIBRARY_TYPE STREQUAL "SHARED_LIBRARY")
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:SDL3::SDL3>"
        "$<TARGET_FILE_DIR:${TARGET_NAME}>"
      VERBATIM
    )
  endif()
endfunction()

add_executable(nesemu src/Emulator.cpp)
target_link_libraries(nesemu PRIVATE nesfrontend)
nes_warnings(nesemu)
copy_sdl_runtime(nesemu)

if(BUILD_TESTING)
  if(MSVC)
    # Match the DLL runtime used by this project's test executables.
    set(gtest_force_shared_crt ON CACHE BOOL "Use the shared MSVC runtime" FORCE)
  endif()
  FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
  )
  FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
  )
  FetchContent_MakeAvailable(googletest nlohmann_json)

  function(add_nes_test TEST_NAME TEST_SRC)
    add_executable(${TEST_NAME} ${TEST_SRC})
    nes_warnings(${TEST_NAME})
    target_link_libraries(${TEST_NAME} PRIVATE gtest_main nescore ${ARGN})
    target_compile_definitions(${TEST_NAME} PRIVATE
      NES_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )

    # Preserve plain ctest support for Visual Studio multi-config builds.
    set_target_properties(${TEST_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    )
    foreach(CONFIG_TYPE IN LISTS CMAKE_CONFIGURATION_TYPES)
      string(TOUPPER "${CONFIG_TYPE}" CONFIG_TYPE_UPPER)
      set_target_properties(${TEST_NAME} PROPERTIES
        "RUNTIME_OUTPUT_DIRECTORY_${CONFIG_TYPE_UPPER}" "${CMAKE_BINARY_DIR}/tests"
      )
    endforeach()

    # This signature avoids per-configuration guards in CTest's generated file.
    add_test(${TEST_NAME} "${CMAKE_COMMAND}" -E env --
      "${CMAKE_BINARY_DIR}/tests/${TEST_NAME}${CMAKE_EXECUTABLE_SUFFIX}"
    )
  endfunction()

  add_nes_test(runCPUHarteTests tests/CPU/CPU_Harte.cpp nlohmann_json::nlohmann_json)
  add_nes_test(runCPUNestest tests/CPU/CPU_Nestest.cpp nesfrontend)
  add_nes_test(runPPUTimingTests tests/PPU/PPU_Timing.cpp)
  add_nes_test(runPPUSpriteZeroTests tests/PPU/PPU_SpriteZero.cpp)
  add_nes_test(runPPUNestest tests/PPU/PPU_Nestest.cpp nesfrontend)
  copy_sdl_runtime(runCPUNestest)
  copy_sdl_runtime(runPPUNestest)

  add_nes_test(runPPUScrollingTests tests/PPU/PPU_Scrolling.cpp)
  add_nes_test(runCoreRegressionTests tests/Core/Core_Regression.cpp)
  add_nes_test(runROMSmokeTests tests/Integration/ROM_Smoke.cpp nesfrontend)
  copy_sdl_runtime(runROMSmokeTests)
  set_tests_properties(runROMSmokeTests PROPERTIES LABELS "local-rom" TIMEOUT 300)
endif()
