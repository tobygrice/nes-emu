#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU.h"
#include "../../include/OpCode.h"
#include "../../include/TestBus.h"
#include "../../include/Logger.h"

class CPULogicalTest : public ::testing::Test {
 protected:
  TestBus bus;  // create a shared bus
  CPU cpu;      // CPU instance that uses the shared bus
  Logger logger;

  CPULogicalTest() : bus(), cpu(&bus, &logger) {}
};

// Test AND with Immediate addressing and zero/neg false
TEST_F(CPULogicalTest, ANDImmediate) {
  std::vector<uint8_t> program = {0xA9, 0b01001100,  // LDA #0b01001100
                                  0x29, 0b10101101,  // ADC $50
                                  0x00};             // BRK

  // expect zero and negative status bits = 0
  // expect A register = 0b00001100
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0b00001100);  // 0x0C
  EXPECT_FALSE(cpu.getStatus() & 0b00000010)
      << "Zero flag should not be set for 0x0C.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000)
      << "Negative flag should not be set for 0x0C.";
}

// Test AND with Zero page addressing and negative result
TEST_F(CPULogicalTest, ANDZeroPageNegResult) {
  cpu.memWrite8(0x50, 0b11101001);  // Store 0b11101001 at address 0x50.
  std::vector<uint8_t> program = {0xA9, 0b10101100,  // LDA #$10
                                  0x25, 0x50,        // AND $50
                                  0x00};             // BRK
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0b10101000);  // A8 or -0x58 or -88
  EXPECT_FALSE(cpu.getStatus() & 0b00000010)
      << "Zero flag should not be set for 0xA8.";
  EXPECT_TRUE(cpu.getStatus() & 0b10000000)
      << "Negative flag should be set for 0xA8.";
}

// Test AND with Absolute addressing and a zero result
TEST_F(CPULogicalTest, ANDAbsoluteZeroResult) {
  cpu.memWrite8(0x1234, 0x00);  // Store 0x00 at address 0x1234.
  std::vector<uint8_t> program = {0xA9, 0b11101101,        // LDA #$10
                                  0x2D, 0x34,       0x12,  // AND $1234
                                  0x00};                   // BRK
  cpu.loadAndExecute(program);

  EXPECT_FALSE(cpu.getA());  // A reg should be zero
  EXPECT_TRUE(cpu.getStatus() & 0b00000010)
      << "Zero flag should be set for 0x00.";
  EXPECT_FALSE(cpu.getStatus() & 0b10000000)
      << "Negative flag should not be set for 0x00.";
}

// Test BIT Absolute with a nonzero AND result and with operand having bits 7
// and 6 set.
//   A = 0xFF, operand = 0xC0 (binary 1100 0000)
//   A & operand = 0xC0 (nonzero) → Zero flag clear.
//   Additionally, Negative flag should be set (bit 7 = 1)
//   and Overflow flag should be set (bit 6 = 1).
//   Expected cycle count: LDA (2) + BIT absolute (4) + BRK (7) = 13 cycles.
TEST_F(CPULogicalTest, BITAbsolute_NonZeroResult) {
  cpu.memWrite8(0x2000, 0xC0);  // Store operand at address 0x2000.
  std::vector<uint8_t> program = {
      0xA9, 0xFF,        // LDA #$FF
      0x2C, 0x00, 0x20,  // BIT $2000 (absolute addressing)
      0x00               // BRK
  };
  cpu.loadAndExecute(program);

  // Accumulator remains unchanged.
  EXPECT_EQ(cpu.getA(), 0xFF);
  // (A & operand) = 0xFF & 0xC0 = 0xC0, so Zero flag must be clear.
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO) << "Zero flag should be clear.";
  // Bits 7 and 6 of operand (0xC0) are 1 → Negative and Overflow flags should
  // be set.
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should be set.";
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_OVERFLOW)
      << "Overflow flag should be set.";
  //   EXPECT_EQ(cpu.getCycleCount(), 13)
  //       << "Cycle count should be 13 (2+4+7) for BIT absolute.";
}

// Test BIT Absolute with a zero AND result.
//   A = 0x08, operand = 0xC0
//   A & operand = 0x08 & 0xC0 = 0x00, so the Zero flag should be set.
//   However, Negative and Overflow flags are still updated from the operand:
//     Negative = 1 (bit7 of 0xC0) and Overflow = 1 (bit6 of 0xC0).
//   Expected cycle count remains 13 cycles.
TEST_F(CPULogicalTest, BITAbsolute_ZeroResult) {
  cpu.memWrite8(0x2000, 0xC0);  // Operand with bits 7 and 6 set.
  std::vector<uint8_t> program = {
      0xA9, 0x08,        // LDA #$08
      0x2C, 0x00, 0x20,  // BIT $2000 (absolute)
      0x00               // BRK
  };
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x08);
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_ZERO) << "Zero flag should be set.";
  // Even though (A & operand) is zero, BIT still transfers operand bits 7 and
  // 6:
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should be set.";
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_OVERFLOW)
      << "Overflow flag should be set.";
  //   EXPECT_EQ(cpu.getCycleCount(), 13)
  //       << "Cycle count should be 13 (2+4+7) for BIT absolute.";
}

// Test BIT Absolute with an operand that has only bit 6 set (overflow flag) but
// not bit 7.
//   A = 0xFF, operand = 0x40 (binary 0100 0000)
//   A & operand = 0xFF & 0x40 = 0x40 (nonzero) → Zero flag clear.
//   Negative flag should be clear (bit 7 = 0), and Overflow flag should be set.
//   Cycle count should be 13.
TEST_F(CPULogicalTest, BITAbsolute_OverflowOnly) {
  cpu.memWrite8(0x2000, 0x40);  // Operand with only bit 6 set.
  std::vector<uint8_t> program = {
      0xA9, 0xFF,        // LDA #$FF
      0x2C, 0x00, 0x20,  // BIT $2000 (absolute)
      0x00               // BRK
  };
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0xFF);
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO) << "Zero flag should be clear.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should be clear.";
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_OVERFLOW)
      << "Overflow flag should be set.";
  //   EXPECT_EQ(cpu.getCycleCount(), 13)
  //       << "Cycle count should be 13 (2+4+7) for BIT absolute.";
}

// Test BIT Zero Page with a nonzero AND result.
//   Use zero page address 0x0040 with operand = 0x40.
//   LDA #$FF ensures (A & operand) = 0x40 (nonzero) → Zero flag clear.
//   And the operand 0x40 has bit 6 set (so overflow flag set) but bit 7 clear.
//   Expected cycle count: LDA (2) + BIT zero page (3) + BRK (7) = 12 cycles.
TEST_F(CPULogicalTest, BITZeroPage_NonZeroResult) {
  cpu.memWrite8(0x0040, 0x40);  // Store operand in zero page.
  std::vector<uint8_t> program = {
      0xA9, 0xFF,  // LDA #$FF
      0x24, 0x40,  // BIT $40 (zero page addressing)
      0x00         // BRK
  };
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0xFF);
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_ZERO) << "Zero flag should be clear.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should be clear.";
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_OVERFLOW)
      << "Overflow flag should be set.";
  //   EXPECT_EQ(cpu.getCycleCount(), 12)
  //       << "Cycle count should be 12 (2+3+7) for BIT zero page.";
}

// Test BIT Zero Page with a zero AND result.
//   Use zero page address 0x0040 with operand = 0x40.
//   LDA #$00 makes (A & operand) = 0, so the Zero flag should be set.
//   But the operand still transfers its bit 6 into the Overflow flag.
//   Expected cycle count is 12.
TEST_F(CPULogicalTest, BITZeroPage_ZeroResult) {
  cpu.memWrite8(0x0040, 0x40);  // Operand with bit 6 set.
  std::vector<uint8_t> program = {
      0xA9, 0x00,  // LDA #$00
      0x24, 0x40,  // BIT $40 (zero page)
      0x00         // BRK
  };
  cpu.loadAndExecute(program);

  EXPECT_EQ(cpu.getA(), 0x00);
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_ZERO) << "Zero flag should be set.";
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should be clear.";
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_OVERFLOW)
      << "Overflow flag should be set.";
  //   EXPECT_EQ(cpu.getCycleCount(), 12)
  //       << "Cycle count should be 12 (2+3+7) for BIT zero page.";
}