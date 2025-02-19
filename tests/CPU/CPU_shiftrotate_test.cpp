#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU.h"
#include "../../include/OpCode.h"
#include "../../include/TestBus.h"
#include "../../include/Logger.h"

// Define a test fixture for CPU tests.
class CPUShiftRotateTest : public ::testing::Test {
 protected:
  TestBus bus;  // create a shared bus
  CPU cpu;      // CPU instance that uses the shared bus
  Logger logger;

  CPUShiftRotateTest() : bus(), cpu(&bus, &logger) {}
};

// Test ASL flag cleared
TEST_F(CPUShiftRotateTest, ASLFlagCleared) {
  cpu.memWrite8(0x50, 0b01101110);
  std::vector<uint8_t> program = {0x06, 0x50,  // ASL $50
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.memRead8(0x50), 0b11011100);
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_CARRY)
      << "Carry flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO)
      << "Zero flag should not be set.";
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should be set.";
}

// Test ASL flag set
TEST_F(CPUShiftRotateTest, ASLFlagSet) {
  cpu.memWrite8(0x50, 0b10101110);
  std::vector<uint8_t> program = {0x06, 0x50,  // ASL $50
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.memRead8(0x50), 0b01011100);
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_CARRY) << "Carry flag should be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO)
      << "Zero flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should not be set.";
}

// Test ASL on accumulator
TEST_F(CPUShiftRotateTest, ASLAccumulator) {
  std::vector<uint8_t> program = {0xA9, 0b10111011,  // LDA #$BB
                                  0x0A,              // ASL (ACC)
                                  0x00};             // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0b01110110);
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_CARRY) << "Carry flag should be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO)
      << "Zero flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should not be set.";
}

// Test LSR flag cleared
TEST_F(CPUShiftRotateTest, LSRFlagCleared) {
  cpu.memWrite8(0x50, 0b01101110);
  std::vector<uint8_t> program = {0x46, 0x50,  // LSR $50
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.memRead8(0x50), 0b00110111);
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_CARRY)
      << "Carry flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO)
      << "Zero flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should not be set.";
}

// Test LSR flag set
TEST_F(CPUShiftRotateTest, LSRFlagSet) {
  cpu.memWrite8(0x50, 0b10101101);
  std::vector<uint8_t> program = {0x46, 0x50,  // LSR $50
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.memRead8(0x50), 0b01010110);
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_CARRY) << "Carry flag should be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO)
      << "Zero flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should not be set.";
}

// Test LSR on accumulator
TEST_F(CPUShiftRotateTest, LSRAccumulator) {
  std::vector<uint8_t> program = {0xA9, 0b01101110,  // LDA 0b01101110
                                  0x4A,              // LSR (acc)
                                  0x00, 0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0b00110111);
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_CARRY)
      << "Carry flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO)
      << "Zero flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should not be set.";
}

// Test ROL carry set->clear
TEST_F(CPUShiftRotateTest, ROLFlagCleared) {
  cpu.memWrite8(0x50, 0b01101110);
  std::vector<uint8_t> program = {0x38,         // SEC
                                  0x26, 0x50,   // ROL $50
                                  0x00, 0x00};  // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.memRead8(0x50), 0b11011101);
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_CARRY)
      << "Carry flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO)
      << "Zero flag should not be set.";
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should be set.";
}

// Test ROL carry clear->set
TEST_F(CPUShiftRotateTest, ROLFlagSet) {
  cpu.memWrite8(0x50, 0b10101101);
  std::vector<uint8_t> program = {0x26, 0x50,   // ROL $50
                                  0x00, 0x00};  // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.memRead8(0x50), 0b01011010);
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_CARRY) << "Carry flag should be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO)
      << "Zero flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should not be set.";
}

// Test ROL on accumulator carry set->set
TEST_F(CPUShiftRotateTest, ROLAccumulator) {
  std::vector<uint8_t> program = {0xA9, 0b11101110,  // LDA 0b01101110
                                  0x38,              // SEC
                                  0x2A,              // ROL (acc)
                                  0x00, 0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0b11011101);
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_CARRY) << "Carry flag should be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO)
      << "Zero flag should not be set.";
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should be set.";
}

// Test ROR with carry set -> clear
TEST_F(CPUShiftRotateTest, RORFlagCleared) {
  cpu.memWrite8(0x50, 0b01101110);
  std::vector<uint8_t> program = {
      0x38,        // SEC (Set Carry Flag)
      0x66, 0x50,  // ROR $50
      0x00, 0x00   // BRK
  };
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.memRead8(0x50), 0b10110111);
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_CARRY)
      << "Carry flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO)
      << "Zero flag should not be set.";
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should be set.";
}

// Test ROR with carry clear -> set
TEST_F(CPUShiftRotateTest, RORFlagSet) {
  cpu.memWrite8(0x50, 0b10101101);
  std::vector<uint8_t> program = {
      0x18,        // CLC (Clear Carry Flag)
      0x66, 0x50,  // ROR $50
      0x00, 0x00   // BRK
  };
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.memRead8(0x50), 0b01010110);
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_CARRY) << "Carry flag should be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO)
      << "Zero flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should not be set.";
}

// Test ROR on accumulator with carry set -> set
TEST_F(CPUShiftRotateTest, RORAccumulator) {
  std::vector<uint8_t> program = {
      0xA9, 0b11101101,  // LDA #$EE (Load Accumulator with 0b11101110)
      0x38,              // SEC (Set Carry Flag)
      0x6A,              // ROR (Rotate Right Accumulator)
      0x00, 0x00         // BRK
  };
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0b11110110);
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_CARRY) << "Carry flag should be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO)
      << "Zero flag should not be set.";
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should be set.";
}