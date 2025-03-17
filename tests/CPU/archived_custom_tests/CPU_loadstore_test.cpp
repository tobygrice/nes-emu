#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU/CPU.h"
#include "../../include/CPU/OpCode.h"
#include "../../include/TestBus.h"
#include "../../include/Logger.h"

// Define a test fixture for CPU tests.
class CPULoadStoreTest : public ::testing::Test {
 protected:
  TestBus bus;  // create a shared bus
  CPU cpu;  // CPU instance that uses the shared bus
  Logger logger;

  CPULoadStoreTest() : bus(), cpu(&bus, &logger) {}
};

/* ========================= LDA ========================= */
// Test LDA (Load Accumulator) with Immediate addressing.
TEST_F(CPULoadStoreTest, LDAImmediate) {
  std::vector<uint8_t> program = {0xA9, 0x42, 0x00};
  cpu.loadAndExecute(program);
  EXPECT_EQ(cpu.getA(), 0x42);
  EXPECT_FALSE(cpu.getStatus() & 0b00000010)
      << "Zero flag should not be set for 0x42.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000)
      << "Negative flag should not be set for 0x42.";
}

// Test LDA Immediate with a zero operand (should set the zero flag).
TEST_F(CPULoadStoreTest, LDAImmediateZero) {
  std::vector<uint8_t> program = {0xA9, 0x00, 0x00};
  cpu.loadAndExecute(program);
  EXPECT_EQ(cpu.getA(), 0x00);
  EXPECT_TRUE(cpu.getStatus() & 0b00000010)
      << "Zero flag should be set when operand is 0.";
}

// Test LDA Immediate with a negative operand (should set the negative flag).
TEST_F(CPULoadStoreTest, LDAImmediateNegative) {
  std::vector<uint8_t> program = {0xA9, 0x80, 0x00};
  cpu.loadAndExecute(program);
  EXPECT_EQ(cpu.getA(), 0x80);
  EXPECT_TRUE(cpu.getStatus() & 0b10000000)
      << "Negative flag should be set when operand is 0x80.";
}