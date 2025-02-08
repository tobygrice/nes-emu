#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU.h"
#include "../../include/OpCode.h"

// Define a test fixture for CPU tests.
class CPULogicalTest : public ::testing::Test {
 protected:
  CPU cpu;
};

// Test AND with Immediate addressing and zero/neg false
TEST_F(CPULogicalTest, ANDImmediate) {
  std::vector<uint8_t> program = {0xA9, 0b01001100,  // LDA #0b01001100
                                  0x29, 0b10101101,  // ADC $50
                                  0x00};             // BRK

  // expect zero and negative status bits = 0
  // expect A register = 0b00001100
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0b00001100); // 0x0C
  EXPECT_FALSE(cpu.getStatus() & 0b00000010)
      << "Zero flag should not be set for 0x0C.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000)
      << "Negative flag should not be set for 0x0C.";
}

// Test AND with Zero page addressing and negative result
TEST_F(CPULogicalTest, ANDZeroPageNegResult) {
  cpu.memWrite8(0x50, 0b11101001); // Store 0b11101001 at address 0x50.
  std::vector<uint8_t> program = {0xA9, 0b10101100,  // LDA #$10
                                  0x25, 0x50,  // AND $50
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0b10101000); // A8 or -0x58 or -88
  EXPECT_FALSE(cpu.getStatus() & 0b00000010)
      << "Zero flag should not be set for 0xA8.";
  EXPECT_TRUE(cpu.getStatus() & 0b10000000)
      << "Negative flag should be set for 0xA8.";
}

// Test AND with Absolute addressing and a zero result
TEST_F(CPULogicalTest, ANDAbsoluteZeroResult) {
  cpu.memWrite8(0x1234, 0x00); // Store 0x00 at address 0x1234.
  std::vector<uint8_t> program = {0xA9, 0b11101101, // LDA #$10
                                  0x2D, 0x34, 0x12, // AND $1234
                                  0x00};            // BRK
  cpu.loadAndExecute(program);

  EXPECT_FALSE(cpu.getA()); // A reg should be zero
  EXPECT_TRUE(cpu.getStatus() & 0b00000010)
      << "Zero flag should be set for 0x00.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000)
      << "Negative flag should not be set for 0x00.";
}