#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU.h"
#include "../../include/OpCode.h"

// Define a test fixture for CPU tests.
class CPUShiftRotateTest : public ::testing::Test {
 protected:
  CPU cpu;
};

// Test ASL flag cleared
TEST_F(CPUShiftRotateTest, ASLFlagCleared) {
  cpu.memWrite8(0x50, 0b01101110);
  std::vector<uint8_t> program = {0x06, 0x50,  // ASL $50
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.memRead8(0x50), 0b11011100);
  EXPECT_FALSE(cpu.getStatus() & 0b00000001) << "Carry flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & 0b00000010) << "Zero flag should not be set.";
  EXPECT_TRUE(cpu.getStatus() & 0b10000000) << "Negative flag should be set.";
}

// Test ASL flag set
TEST_F(CPUShiftRotateTest, ASLFlagSet) {
  cpu.memWrite8(0x50, 0b10101110);
  std::vector<uint8_t> program = {0x06, 0x50,  // ASL $50
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.memRead8(0x50), 0b01011100);
  EXPECT_TRUE(cpu.getStatus() & 0b00000001) << "Carry flag should be set.";
  EXPECT_FALSE(cpu.getStatus() & 0b00000010) << "Zero flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000) << "Negative flag should not be set.";
}

// Test ASL on accumulator
TEST_F(CPUShiftRotateTest, ASLAccumulator) {
  std::vector<uint8_t> program = {0xA9, 0b10111011, // LDA #$BB
                                  0x0A,             // ASL (ACC)
                                  0x00};            // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0b01110110);
  EXPECT_TRUE(cpu.getStatus() & 0b00000001) << "Carry flag should be set.";
  EXPECT_FALSE(cpu.getStatus() & 0b00000010) << "Zero flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000) << "Negative flag should not be set.";
}

// Test LSR flag cleared
TEST_F(CPUShiftRotateTest, LSRFlagCleared) {
  cpu.memWrite8(0x50, 0b01101110);
  std::vector<uint8_t> program = {0x46, 0x50,  // LSR $50
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.memRead8(0x50), 0b00110111);
  EXPECT_FALSE(cpu.getStatus() & 0b00000001) << "Carry flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & 0b00000010) << "Zero flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000) << "Negative flag should not be set.";
}

// Test LSR flag set
TEST_F(CPUShiftRotateTest, LSRFlagSet) {
  cpu.memWrite8(0x50, 0b10101101);
  std::vector<uint8_t> program = {0x46, 0x50,  // LSR $50
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.memRead8(0x50), 0b01010110);
  EXPECT_TRUE(cpu.getStatus() & 0b00000001) << "Carry flag should be set.";
  EXPECT_FALSE(cpu.getStatus() & 0b00000010) << "Zero flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000) << "Negative flag should not be set.";
}

// Test LSR on accumulator
TEST_F(CPUShiftRotateTest, LSRAccumulator) {
  std::vector<uint8_t> program = {0xA9, 0b01101110, // LDA 0b01101110
                                  0x4A,             // LSR (acc)
                                  0x00};            // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0b00110111);
  EXPECT_FALSE(cpu.getStatus() & 0b00000001) << "Carry flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & 0b00000010) << "Zero flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000) << "Negative flag should not be set.";
}