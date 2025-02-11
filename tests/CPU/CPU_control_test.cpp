#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU.h"
#include "../../include/OpCode.h"

// Define a test fixture for CPU tests.
class CPUControlTest : public ::testing::Test {
 protected:
  CPU cpu;
};

// Test the BRK handler by calling the BRK opcode directly.
TEST_F(CPUControlTest, BRKHandler) {
  // Set the BRK interrupt vector to 0x9000.
  cpu.memWrite16(0xFFFE, 0x9000);

  std::vector<uint8_t> program = {0xA9, 0x42, 0x00};
  cpu.loadAndExecute(program);

  // After pushing three bytes, the stack pointer should be 0xFF - 3 = 0xFC.
  EXPECT_EQ(cpu.getSP(), 0xFC);
  EXPECT_EQ(cpu.getPC(), 0x9000); // new PC should be loaded from the BRK interrupt vector
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_INTERRUPT); // I flag should be set

  // Verify the values pushed onto the stack.
  // The order of pushes: first the high byte of (return address),
  // then the low byte of (return address), then the status (with break flag
  // set).
  uint8_t pushedStatus = cpu.pop();  // at address 0x01FD.
  uint8_t pushedHigh = cpu.pop();    // at address 0x01FF.
  uint8_t pushedLow = cpu.pop();     // at address 0x01FE.

  EXPECT_EQ(pushedHigh, 0x80)
      << "High byte of pushed PC should be 0x80.";
  EXPECT_EQ(pushedLow, 0x02) << "Low byte of pushed PC should be 0x01.";
  EXPECT_EQ(pushedStatus, cpu.FLAG_BREAK)
      << "Status pushed onto stack should have the break flag (bit 4) set.";
}

TEST_F(CPUControlTest, JSRandRTS) {
  std::vector<uint8_t> program = {
      0x20, 0x09, 0x80,   // JSR $8009 (init)
      0x20, 0x0c, 0x80,   // JSR $800C (loop)
      0x20, 0x12, 0x80,   // JSR $8012 (end)
      0xa2, 0x00,         // LDX #$00 (init target)
      0x60,               // RTS
      0xE8,               // INX (loop target)
      0xE0, 0x05,         // CPX #$05
      0xD0, 0xFB,         // BNE loop
      0x60,               // RTS
      0x00                // BRK (end target)
  };
  cpu.loadAndExecute(program);
  EXPECT_EQ(cpu.getX(), 0x05) << "X should be incremented until it equals 5";
}