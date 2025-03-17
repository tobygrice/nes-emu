#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU/CPU.h"
#include "../../include/CPU/OpCode.h"
#include "../../include/TestBus.h"
#include "../../include/Logger.h"

// Define a test fixture for CPU tests.
class CPUArithmeticTest : public ::testing::Test {
  protected:
  TestBus bus;  // create a shared bus
  CPU cpu;  // CPU instance that uses the shared bus
  Logger logger;

  CPUArithmeticTest() : bus(), cpu(&bus, &logger) {}
};


// Test ADC (Add with Carry) with Immediate addressing.
TEST_F(CPUArithmeticTest, ADCImmediate) {
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0x69, 0x20,  // ADC #$20
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x30);  // 0x10 + 0x20 = 0x30
  EXPECT_FALSE(cpu.getStatus() & 0b00000010) << "Zero flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000)
      << "Negative flag should not be set.";
  EXPECT_FALSE(cpu.getStatus() & 0b00000001) << "Carry flag should not be set.";
}

// Test ADC with Zero Page addressing.
TEST_F(CPUArithmeticTest, ADCZeroPage) {
  cpu.memWrite8(0x50, 0x20);                   // Store 0x20 at address 0x50.
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0x65, 0x50,  // ADC $50
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x30);
}

// Test ADC with Zero Page, X addressing.
TEST_F(CPUArithmeticTest, ADCZeroPageX) {
  cpu.memWrite8(0x52, 0x20);                   // Store 0x20 at address 0x52.
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0xA2, 0x02,  // LDX #$02
                                  0x75, 0x50,  // ADC $50,X
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x30);
}

// Test ADC with Absolute addressing.
TEST_F(CPUArithmeticTest, ADCAbsolute) {
  cpu.memWrite8(0x1234, 0x20);                 // Store 0x20 at address 0x1234.
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0x6D, 0x34, 0x12,  // ADC $1234
                                  0x00};             // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x30);
}

// Test ADC with Absolute, X addressing.
TEST_F(CPUArithmeticTest, ADCAbsoluteX) {
  cpu.memWrite8(0x1236, 0x20);                 // Store 0x20 at address 0x1236.
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0xA2, 0x02,  // LDX #$02
                                  0x7D, 0x34, 0x12,  // ADC $1234,X
                                  0x00};             // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x30);
}

// Test ADC with Absolute, Y addressing.
TEST_F(CPUArithmeticTest, ADCAbsoluteY) {
  cpu.memWrite8(0x1236, 0x20);                 // Store 0x20 at address 0x1236.
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0xA0, 0x02,  // LDY #$02
                                  0x79, 0x34, 0x12,  // ADC $1234,Y
                                  0x00};             // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x30);
}

// Test ADC with Indirect, X addressing.
TEST_F(CPUArithmeticTest, ADCIndirectX) {
  cpu.memWrite8(0x0050, 0x34);  // Pointer low byte
  cpu.memWrite8(0x0051, 0x12);  // Pointer high byte
  cpu.memWrite8(0x1234, 0x20);  // Value at the resolved address.
  std::vector<uint8_t> program = {
      0xA9, 0x10,  // LDA #$10
      0xA2, 0x02,  // LDX #$02
      0x61, 0x4E,  // ADC ($4E,X) (4E+2=50 -> address 0x1234)
      0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x30);
}

// Test ADC with Indirect, Y addressing.
TEST_F(CPUArithmeticTest, ADCIndirectY) {
  cpu.memWrite8(0x0050, 0x34);                 // Pointer low byte
  cpu.memWrite8(0x0051, 0x12);                 // Pointer high byte
  cpu.memWrite8(0x1236, 0x20);                 // Value at the resolved address.
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0xA0, 0x02,  // LDY #$02
                                  0x71, 0x50,  // ADC ($50),Y (0x1234 + 2)
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x30);
}

// Test ADC with Carry flag set.
TEST_F(CPUArithmeticTest, ADCCarrySet) {
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0x69, 0x20,  // ADC #$20
                                  0x00};       // BRK
  cpu.loadProgram(program);
  cpu.in_RESET();
  cpu.setStatus(cpu.getStatus() | 0x01); // set Carry flag
  cpu.executeProgram();
  EXPECT_EQ(cpu.getA(), 0x31);  // 0x10 + 0x20 + 1 = 0x31
}

// Test ADC causing an overflow.
TEST_F(CPUArithmeticTest, ADCOverflow) {
  std::vector<uint8_t> program = {0xA9, 0x50,  // LDA #$50
                                  0x69, 0x50,  // ADC #$50
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_TRUE(cpu.getStatus() & 0b01000000) << "Overflow flag should be set.";
}

// Test ADC causing a carry.
TEST_F(CPUArithmeticTest, ADCCausingCarry) {
  std::vector<uint8_t> program = {0xA9, 0xFF,  // LDA #$FF
                                  0x69, 0x01,  // ADC #$01
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x00);
  EXPECT_TRUE(cpu.getStatus() & 0b00000010) << "Zero flag should be set.";
  EXPECT_TRUE(cpu.getStatus() & 0b00000001) << "Carry flag should be set.";
}