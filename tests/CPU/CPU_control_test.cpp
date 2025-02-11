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
  // Set IRQ interrupt vector to 0x9000.
  cpu.memWrite16(0xFFFE, 0x9000);

  // Include a padding byte after BRK since BRK is a two-byte opcode.
  std::vector<uint8_t> program = {
    0xA9, 0x42, // LDA #$42
    0x00, 0x00  // BRK (0x00 opcode plus one padding byte)
  };
  cpu.loadAndExecute(program);

  // After pushing three bytes, the stack pointer should be 0xFF - 3 = 0xFC.
  EXPECT_EQ(cpu.getSP(), 0xFC);
  EXPECT_EQ(cpu.getPC(), 0x9000) 
      << "New PC should be loaded from the BRK interrupt vector.";
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_INTERRUPT)
      << "Interrupt flag should be set.";

  // Verify the values pushed onto the stack.
  // The push order for BRK is:
  //   first push: high byte of the return address,
  //   second push: low byte of the return address,
  //   third push: status (with the break flag set).
  // With the program loaded at 0x8000:
  //   LDA #$42 takes 2 bytes (PC becomes 0x8002),
  //   then BRK is fetched (opcode at 0x8002) and a padding byte is fetched,
  //   and op_BRK does an extra PC increment, so the return address pushed is 0x8004.
  //
  // Thus, we expect:
  //   High byte = 0x80,
  //   Low byte = 0x04.
  //
  // Note: The order of pops depends on how your pop() is implemented.
  // If push() writes high byte first then low then status,
  // a typical pop sequence is:
  //   first pop returns status,
  //   second pop returns low byte,
  //   third pop returns high byte.
  uint8_t pushedStatus = cpu.pop();  // should be the status (with break flag set)
  uint8_t pushedLow    = cpu.pop();    // should be 0x04 (low byte of return address)
  uint8_t pushedHigh   = cpu.pop();    // should be 0x80 (high byte of return address)

  EXPECT_EQ(pushedHigh, 0x80)
      << "High byte of pushed PC should be 0x80.";
  EXPECT_EQ(pushedLow, 0x04)
      << "Low byte of pushed PC should be 0x04.";
  EXPECT_TRUE(pushedStatus & cpu.FLAG_BREAK)
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