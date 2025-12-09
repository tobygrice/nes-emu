#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>

#include "../../include/CPU/CPU.h"
#include "../../include/TestBus.h"
// #include "../../include/CPU/OpCode.h"
#include "../../include/Logger.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

/**
 * Tests all documented opcodes from Tom Harte nes6502 tests:
 * https://github.com/SingleStepTests/65x02/tree/main/nes6502
 */

// Define a test fixture for CPU tests.
class CPUHarteTests : public ::testing::Test {
 protected:
  Logger logger;
  TestBus bus;
  CPU cpu;

  CPUHarteTests() : logger(), bus(), cpu(&bus, &logger) {}
};

struct CPUTestState {
  uint16_t pc;
  uint8_t s;           // sp
  uint8_t a, x, y, p;  // p = processor status
  std::vector<std::pair<uint16_t, uint8_t>> ram;
};

// Function to load JSON file
json load_json(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open file: " + filename);
  }
  json j;
  file >> j;
  return j;
}

// Convert JSON object to CPUTestState
CPUTestState parse_state(const json &j) {
  CPUTestState state;
  state.pc = j["pc"];
  state.s = j["s"];
  state.a = j["a"];
  state.x = j["x"];
  state.y = j["y"];
  state.p = j["p"];  // status

  for (const auto &entry : j["ram"]) {
    state.ram.emplace_back(entry[0], entry[1]);
  }
  return state;
}

TEST_F(CPUHarteTests, runAllHarteTests) {
  uint num_passed_tests = 0;
  // logger.mute();

  for (uint16_t opcode = 0x00; opcode <= 0xFF; opcode++) {
    const OpCode *op = OpCode::getOpCode(opcode);  // look up opcode
    if (op) {
      if (!op->isDocumented) continue;  // only test documented opcodes
    } else {
      continue;
    }
    // unpredictable results make some unstable undocmented opcodes almost
    // impossible to emulate consistently. Nestest provides best possible
    // testing for undocumented opcodes.

    std::string filename =
        std::format("../tests/CPU/nes6502-TESTS/{:02x}.json", opcode);
    std::cout << "STARTING TEST " << std::format("{:#04x}\n", opcode);

    json tests = load_json(filename);

    for (const auto &test : tests) {
      CPUTestState initial = parse_state(test["initial"]);
      CPUTestState expected = parse_state(test["final"]);

      uint8_t expectedCycles = test["cycles"].size();
      uint8_t actualCycles = 0;

      cpu.TEST_setA(initial.a);
      cpu.TEST_setX(initial.x);
      cpu.TEST_setY(initial.y);
      cpu.TEST_setStatus(initial.p);
      cpu.TEST_setPC(initial.pc);
      cpu.TEST_setSP(initial.s);
      for (const auto &[addr, val] : initial.ram) {
        bus.write(addr, val);
      }

      cpu.tick(); // start executing new instruction
      actualCycles++;
      // continue ticking until instruction is complete:
      while (cpu.TEST_getCyclesRemainingInCurrentInstr() > 0) {
        cpu.tick();
        actualCycles++;
      }

      ASSERT_EQ(actualCycles, expectedCycles)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      ASSERT_EQ(cpu.TEST_getA(), expected.a)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      ASSERT_EQ(cpu.TEST_getX(), expected.x)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      ASSERT_EQ(cpu.TEST_getY(), expected.y)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      ASSERT_EQ(cpu.TEST_getStatus(), expected.p)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      ASSERT_EQ(cpu.TEST_getPC(), expected.pc)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      ASSERT_EQ(cpu.TEST_getSP(), expected.s)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      for (const auto &[addr, expectedRead] : expected.ram) {
        // Compare the single byte at 'addr'
        uint8_t actualRead = bus.read(addr);
        ASSERT_EQ(actualRead, expectedRead)
            << "Mismatch at mem addr " << std::uppercase
            << std::format("{:#06x} (", addr) << addr << ") @ instruction "
            << test["name"] << " after passing " << num_passed_tests
            << " tests.";
      }
      num_passed_tests++;
    }
  }
}

// Main entry point for the tests.
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}