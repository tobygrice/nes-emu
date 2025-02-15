#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <vector>
#include <tuple>
#include <string>
#include <format>
#include <iostream>

#include "../../include/CPU.h"
#include "../../include/OpCode.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

struct CPUState {
  uint16_t pc;
  uint8_t s;           // sp
  uint8_t a, x, y, p;  // p = processor status
  std::vector<std::pair<uint16_t, uint8_t>> ram;
};

// Load JSON file from disk.
json load_json(const std::string &filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Cannot open file: " + filename);
  }
  json j;
  file >> j;
  return j;
}

// Convert a JSON object to a CPUState.
CPUState parse_state(const json &j) {
  CPUState state;
  state.pc = j["pc"];
  state.s  = j["s"];
  state.a  = j["a"];
  state.x  = j["x"];
  state.y  = j["y"];
  state.p  = j["p"];

  for (const auto &entry : j["ram"]) {
    // Each entry is an array: [address, value]
    state.ram.emplace_back(entry[0].get<uint16_t>(), entry[1].get<uint8_t>());
  }
  return state;
}

// Structure representing a single test case.
struct CPUHarteTestCase {
  std::string testName;
  CPUState initial;
  CPUState expected;
  // The cycles field is loaded here for completeness but isn't used directly.
  std::vector<std::tuple<uint16_t, uint8_t, std::string>> cycles;
};

// Load all test cases from the JSON files.
std::vector<CPUHarteTestCase> load_all_tests() {
  std::vector<CPUHarteTestCase> testCases;
  
  // Iterate over 00.json to FF.json.
  for (uint16_t fileIndex = 0x00; fileIndex <= 0xFF; fileIndex++) {
    // Construct filename (adjust the relative path as needed).
    std::string filename = std::format("../tests/CPU/nes6502-TESTS/{:02x}.json", fileIndex);
    if (!fs::exists(filename)) {
      std::cerr << "File does not exist: " << filename << "\n";
      continue;
    }
    
    json tests_json = load_json(filename);
    // Each file contains an array of test objects.
    for (const auto &test : tests_json) {
      CPUHarteTestCase testCase;
      testCase.testName = test["name"].get<std::string>();
      testCase.initial  = parse_state(test["initial"]);
      testCase.expected = parse_state(test["final"]);
      
      for (const auto &c : test["cycles"]) {
        uint16_t addr = c[0].get<uint16_t>();
        uint8_t val   = c[1].get<uint8_t>();
        std::string op = c[2].get<std::string>();
        testCase.cycles.emplace_back(addr, val, op);
      }
      testCases.push_back(testCase);
    }
  }
  return testCases;
}

// Define a parameterized test fixture.
class CPUHarteParameterized : public ::testing::TestWithParam<CPUHarteTestCase> {
 protected:
  // Each test gets its own CPU instance.
  CPU cpu;
};

TEST_P(CPUHarteParameterized, ExecuteTest) {
  const CPUHarteTestCase &tc = GetParam();

  // Initialize the CPU using the test case's initial state.
  cpu.setA(tc.initial.a);
  cpu.setX(tc.initial.x);
  cpu.setY(tc.initial.y);
  cpu.setStatus(tc.initial.p);
  cpu.setPC(tc.initial.pc);
  cpu.setSP(tc.initial.s);
  
  // Write the initial RAM values.
  for (const auto &pair : tc.initial.ram) {
    cpu.memWrite8(pair.first, pair.second);
  }

  // Execute the test (this should run the opcode and update the CPU state).
  cpu.executeHarteTest();

  // Compare CPU state with expected state.
  EXPECT_EQ(cpu.getA(), tc.expected.a) << " @ instruction " << tc.testName;
  EXPECT_EQ(cpu.getX(), tc.expected.x) << " @ instruction " << tc.testName;
  EXPECT_EQ(cpu.getY(), tc.expected.y) << " @ instruction " << tc.testName;
  EXPECT_EQ(cpu.getStatus(), tc.expected.p) << " @ instruction " << tc.testName;
  EXPECT_EQ(cpu.getPC(), tc.expected.pc) << " @ instruction " << tc.testName;
  EXPECT_EQ(cpu.getSP(), tc.expected.s) << " @ instruction " << tc.testName;

  // Verify that memory values match the expected state.
  for (const auto &pair : tc.expected.ram) {
    uint16_t addr = pair.first;
    uint8_t expected_val = pair.second;
    EXPECT_EQ(cpu.memRead8(addr), expected_val)
      << "Mismatch at memory address 0x" << std::hex << addr 
      << " @ instruction " << tc.testName;
  }
}

// Instantiate the parameterized tests.
INSTANTIATE_TEST_SUITE_P(
  CPUHarteTestsInstantiation,
  CPUHarteParameterized,
  ::testing::ValuesIn(load_all_tests()),
  // Optional: a custom name generator that uses the testName field.
  [](const ::testing::TestParamInfo<CPUHarteTestCase>& info) {
    // Replace any spaces or non-alphanumeric characters if necessary.
    std::string name = info.param.testName;
    // Remove spaces (or any custom sanitization) for a valid test name.
    for (auto &ch : name) {
      if (!std::isalnum(ch)) {
        ch = '_';
      }
    }
    return name;
  }
);