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
  cpu.op_BRK(0x0);

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