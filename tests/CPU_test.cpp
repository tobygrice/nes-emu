// test_cpu.cpp
#include "../include/CPU.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../include/OpCode.h"

// Define a test fixture for CPU tests.
class CPUTest : public ::testing::Test {
 protected:
  CPU cpu;
};

// Test memory read/write for single bytes.
TEST_F(CPUTest, MemoryReadWrite8) {
  cpu.memWrite8(0x1234, 0xAB);
  EXPECT_EQ(cpu.memRead8(0x1234), 0xAB);
}

// Test memory read/write for 16-bit values (little-endian).
TEST_F(CPUTest, MemoryReadWrite16) {
  cpu.memWrite16(0x2000, 0xBEEF);
  EXPECT_EQ(cpu.memRead16(0x2000), 0xBEEF);
}

// Test updateZeroAndNegativeFlags with a zero result.
TEST_F(CPUTest, UpdateFlagsZero) {
  cpu.setStatus(0);
  cpu.updateZeroAndNegativeFlags(0);
  EXPECT_TRUE(cpu.getStatus() & 0b00000010)
      << "Zero flag should be set for result 0.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000)
      << "Negative flag should not be set for 0.";
}

// Test updateZeroAndNegativeFlags with a result that has the high bit set.
TEST_F(CPUTest, UpdateFlagsNegative) {
  cpu.setStatus(0);
  cpu.updateZeroAndNegativeFlags(0x80);
  EXPECT_FALSE(cpu.getStatus() & 0b00000010)
      << "Zero flag should not be set for nonzero result.";
  EXPECT_TRUE(cpu.getStatus() & 0b10000000)
      << "Negative flag should be set when bit 7 is 1.";
}

// Test updateZeroAndNegativeFlags with a nonzero, non-negative value.
TEST_F(CPUTest, UpdateFlagsNonZeroNonNegative) {
  cpu.setStatus(0xFF);  // Start with all flags set.
  cpu.updateZeroAndNegativeFlags(0x05);
  EXPECT_FALSE(cpu.getStatus() & 0b00000010)
      << "Zero flag should be cleared for nonzero result.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000)
      << "Negative flag should be cleared if bit 7 is 0.";
}

// Test that loadProgram correctly copies the program into memory
// and sets the reset vector to the start of cartridge ROM (0x8000).
TEST_F(CPUTest, LoadProgram) {
  std::vector<uint8_t> program = {0xA9, 0x05,
                                  0x00};  // LDA immediate 0x05, then BRK
  cpu.loadProgram(program);
  EXPECT_EQ(cpu.memRead8(0x8000), 0xA9);
  EXPECT_EQ(cpu.memRead8(0x8001), 0x05);
  EXPECT_EQ(cpu.memRead8(0x8002), 0x00);
  // Check the reset vector at 0xFFFC.
  EXPECT_EQ(cpu.memRead16(0xFFFC), 0x8000);
}

// Test resetInterrupt to ensure registers are cleared and the PC is set to the
// reset vector.
TEST_F(CPUTest, ResetInterrupt) {
  cpu.setA(0xFF);
  cpu.setX(0xFF);
  cpu.setY(0xFF);
  cpu.setStatus(0xFF);
  cpu.setPC(0x1234);

  // Set the reset vector to 0x8000.
  cpu.memWrite16(0xFFFC, 0x8000);
  cpu.resetInterrupt();
  EXPECT_EQ(cpu.getPC(), 0x8000);
  EXPECT_EQ(cpu.getA(), 0);
  EXPECT_EQ(cpu.getX(), 0);
  EXPECT_EQ(cpu.getY(), 0);
  EXPECT_EQ(cpu.getStatus(), 0);
}

// Test the executeProgram loop by loading a simple program
// that executes LDA immediate (0xA9) followed by an illegal opcode (0xFF)
// which will cause a runtime_error to be thrown.
TEST_F(CPUTest, UnknownOpcode) {
  // Program: LDA immediate 0x99, then an unknown opcode 0xFF.
  std::vector<uint8_t> program = {0xA9, 0x99, 0xFF};
  cpu.loadProgram(program);
  cpu.resetInterrupt();
  try {
    cpu.executeProgram();
    FAIL() << "Expected std::runtime_error due to unknown opcode.";
  } catch (const std::runtime_error& e) {
    std::string errMsg = e.what();
    EXPECT_NE(errMsg.find("Unknown opcode"), std::string::npos)
        << "Error message should mention unknown opcode.";
    // Verify that the LDA instruction was executed correctly.
    EXPECT_EQ(cpu.getA(), 0x99);
  } catch (...) {
    FAIL() << "Expected std::runtime_error due to unknown opcode.";
  }
}

// Test the BRK handler by calling the BRK opcode directly.
TEST_F(CPUTest, BRKHandler) {
  // Set initial conditions.
  cpu.setPC(0x8000);    // Original PC.
  cpu.setSP(0xFF);      // Stack pointer starts at 0xFF.
  cpu.setStatus(0x00);  // Clear status flags.

  // Set the BRK interrupt vector to 0x9000.
  cpu.memWrite16(0xFFFE, 0x9000);

  // Simulate that the CPU has fetched a BRK instruction and advanced the PC
  // by 2. That is, before calling op_BRK(), PC should be 0x8000 + 2 = 0x8002.
  cpu.setPC(0x8002);

  // Call op_BRK.
  cpu.op_BRK(AddressingMode::NoneAddressing);

  // According to our revised op_BRK, the return address pushed should be (PC -
  // 1). With PC == 0x8002, return address = 0x8002 - 1 = 0x8001.
  uint8_t expectedHigh = 0x80;
  uint8_t expectedLow = 0x01;

  // After pushing three bytes, the stack pointer should be 0xFF - 3 = 0xFC.
  EXPECT_EQ(cpu.getSP(), 0xFC);

  // The new PC should be loaded from the BRK interrupt vector.
  EXPECT_EQ(cpu.getPC(), 0x9000);

  // The Interrupt Disable flag (bit 2) should now be set.
  EXPECT_TRUE(cpu.getStatus() & 0b00000100);

  // Verify the values pushed onto the stack.
  // The order of pushes: first the high byte of (return address),
  // then the low byte of (return address), then the status (with break flag
  // set).
  uint8_t pushedHigh = cpu.memRead8(0x0100 + 0xFF);    // at address 0x01FF.
  uint8_t pushedLow = cpu.memRead8(0x0100 + 0xFE);     // at address 0x01FE.
  uint8_t pushedStatus = cpu.memRead8(0x0100 + 0xFD);  // at address 0x01FD.

  EXPECT_EQ(pushedHigh, expectedHigh)
      << "High byte of pushed PC should be 0x80.";
  EXPECT_EQ(pushedLow, expectedLow) << "Low byte of pushed PC should be 0x01.";
  EXPECT_EQ(pushedStatus, 0x00 | 0b00010000)
      << "Status pushed onto stack should have the break flag (bit 4) set.";
}

// Test LDA (Load Accumulator) with Immediate addressing.
TEST_F(CPUTest, LDAImmediate) {
  std::vector<uint8_t> program = {0xA9, 0x42, 0x00};
  cpu.loadAndExecute(program);
  EXPECT_EQ(cpu.getA(), 0x42);
  EXPECT_FALSE(cpu.getStatus() & 0b00000010)
      << "Zero flag should not be set for 0x42.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000)
      << "Negative flag should not be set for 0x42.";
}

// Test LDA Immediate with a zero operand (should set the zero flag).
TEST_F(CPUTest, LDAImmediateZero) {
  std::vector<uint8_t> program = {0xA9, 0x00, 0x00};
  cpu.loadAndExecute(program);
  EXPECT_EQ(cpu.getA(), 0x00);
  EXPECT_TRUE(cpu.getStatus() & 0b00000010)
      << "Zero flag should be set when operand is 0.";
}

// Test LDA Immediate with a negative operand (should set the negative flag).
TEST_F(CPUTest, LDAImmediateNegative) {
  std::vector<uint8_t> program = {0xA9, 0x80, 0x00};
  cpu.loadAndExecute(program);
  EXPECT_EQ(cpu.getA(), 0x80);
  EXPECT_TRUE(cpu.getStatus() & 0b10000000)
      << "Negative flag should be set when operand is 0x80.";
}

// Test ADC (Add with Carry) with Immediate addressing.
TEST_F(CPUTest, ADCImmediate) {
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
TEST_F(CPUTest, ADCZeroPage) {
  cpu.memWrite8(0x50, 0x20);                   // Store 0x20 at address 0x50.
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0x65, 0x50,  // ADC $50
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x30);
}

// Test ADC with Zero Page, X addressing.
TEST_F(CPUTest, ADCZeroPageX) {
  cpu.memWrite8(0x52, 0x20);                   // Store 0x20 at address 0x52.
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0xA2, 0x02,  // LDX #$02
                                  0x75, 0x50,  // ADC $50,X
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x30);
}

// Test ADC with Absolute addressing.
TEST_F(CPUTest, ADCAbsolute) {
  cpu.memWrite8(0x1234, 0x20);                 // Store 0x20 at address 0x1234.
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0x6D, 0x34, 0x12,  // ADC $1234
                                  0x00};             // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x30);
}

// Test ADC with Absolute, X addressing.
TEST_F(CPUTest, ADCAbsoluteX) {
  cpu.memWrite8(0x1236, 0x20);                 // Store 0x20 at address 0x1236.
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0xA2, 0x02,  // LDX #$02
                                  0x7D, 0x34, 0x12,  // ADC $1234,X
                                  0x00};             // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x30);
}

// Test ADC with Absolute, Y addressing.
TEST_F(CPUTest, ADCAbsoluteY) {
  cpu.memWrite8(0x1236, 0x20);                 // Store 0x20 at address 0x1236.
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0xA0, 0x02,  // LDY #$02
                                  0x79, 0x34, 0x12,  // ADC $1234,Y
                                  0x00};             // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x30);
}

// Test ADC with Indirect, X addressing.
TEST_F(CPUTest, ADCIndirectX) {
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
TEST_F(CPUTest, ADCIndirectY) {
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
TEST_F(CPUTest, ADCCarrySet) {
  std::vector<uint8_t> program = {0xA9, 0x10,  // LDA #$10
                                  0x69, 0x20,  // ADC #$20
                                  0x00};       // BRK
  cpu.loadProgram(program);
  cpu.resetInterrupt();
  cpu.setStatus(cpu.getStatus() | 0x01); // set Carry flag
  cpu.executeProgram();
  EXPECT_EQ(cpu.getA(), 0x31);  // 0x10 + 0x20 + 1 = 0x31
}

// Test ADC causing an overflow.
TEST_F(CPUTest, ADCOverflow) {
  std::vector<uint8_t> program = {0xA9, 0x50,  // LDA #$50
                                  0x69, 0x50,  // ADC #$50
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_TRUE(cpu.getStatus() & 0b01000000) << "Overflow flag should be set.";
}

// Test ADC causing a carry.
TEST_F(CPUTest, ADCCausingCarry) {
  std::vector<uint8_t> program = {0xA9, 0xFF,  // LDA #$FF
                                  0x69, 0x01,  // ADC #$01
                                  0x00};       // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x00);
  EXPECT_TRUE(cpu.getStatus() & 0b00000010) << "Zero flag should be set.";
  EXPECT_TRUE(cpu.getStatus() & 0b00000001) << "Carry flag should be set.";
}

// Main entry point for the tests.
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}