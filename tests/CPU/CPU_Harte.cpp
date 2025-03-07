#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>

#include "../../include/CPU/CPU.h"
#include "../../include/Logger.h"
#include "../../include/CPU/OpCode.h"
#include "../../include/TestBus.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

/**
 * Tests all documented opcodes from Tom Harte nes6502 tests:
 * https://github.com/SingleStepTests/65x02/tree/main/nes6502
 */

// Define a test fixture for CPU tests.
class CPUHarteTests : public ::testing::Test {
 protected:
  TestBus bus;  // create a shared bus
  CPU cpu;      // CPU instance that uses the shared bus
  Logger logger;
  
  CPUHarteTests() : bus(), cpu(&bus, &logger) {}
};

struct CPUState {
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

// Convert JSON object to CPUState
CPUState parse_state(const json &j) {
  CPUState state;
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

  for (uint16_t opcode = 0x00; opcode <= 0xFF; opcode++) {
    const OpCode *op = getOpCode(opcode);  // look up opcode
    if (!op->isDocumented) continue;       // only test documented opcodes
    // unpredictable results make some unstable undocmented opcodes almost
    // impossible to emulate consistently. Nestest provides best possible
    // testing for undocumented opcodes.

    std::string filename =
        std::format("../tests/CPU/nes6502-TESTS/{:02x}.json", opcode);
    std::cout << "STARTING TEST " << std::format("{:#04x}\n", opcode);

    json tests = load_json(filename);

    for (const auto &test : tests) {
      CPUState initial = parse_state(test["initial"]);
      CPUState expected = parse_state(test["final"]);
      
      uint8_t expectedCycles = test["cycles"].size();

      cpu.setA(initial.a);
      cpu.setX(initial.x);
      cpu.setY(initial.y);
      cpu.setStatus(initial.p);
      cpu.setPC(initial.pc);
      cpu.setSP(initial.s);
      cpu.resetCycles();
      for (const auto &[addr, val] : initial.ram) {
        cpu.memWrite8(addr, val);
      }

      uint8_t actualCycles = cpu.executeInstruction();

      ASSERT_EQ(cpu.getA(), expected.a)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      ASSERT_EQ(cpu.getX(), expected.x)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      ASSERT_EQ(cpu.getY(), expected.y)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      ASSERT_EQ(cpu.getStatus(), expected.p)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      ASSERT_EQ(cpu.getPC(), expected.pc)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      ASSERT_EQ(cpu.getSP(), expected.s)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      for (const auto &[addr, val] : expected.ram) {
        // Compare the single byte at 'addr'
        ASSERT_EQ(cpu.memRead8(addr), val)
            << "Mismatch at mem addr " << std::uppercase
            << std::format("{:#06x} (", addr) << addr << ") @ instruction "
            << test["name"] << " after passing " << num_passed_tests
            << " tests.";
      }
      ASSERT_EQ(actualCycles, expectedCycles)
          << " @ instruction " << test["name"] << " after passing "
          << num_passed_tests << " tests.";
      num_passed_tests++;
    }
    std::cout << "PASSED TEST " << std::format("{:#04x}\n", opcode);
  }
}

// Main entry point for the tests.
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}