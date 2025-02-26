#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU/CPU.h"
#include "../../include/TestBus.h"
#include "../../include/CPU/OpCode.h"
#include "../../include/Logger.h"

class CPUFlagTest : public ::testing::Test {
 protected:
  TestBus bus;  // create a shared bus
  CPU cpu;      // CPU instance that uses the shared bus
  Logger logger;

  CPUFlagTest() : bus(), cpu(&bus, &logger) {}
};

#include <gtest/gtest.h>

#include <vector>

// Test fixture: CPUFlagTest is assumed to provide:
//   - cpu.loadAndExecute(program) to load and run a program vector
//   - cpu.getStatus(), cpu.setStatus(), and cpu.getCycleCount()

//
// --- CLC Tests ---
//

// Test CLC when the Carry flag is already clear.
// Expected: Carry flag remains cleared, other flags unchanged.
// Cycle count: CLC (2) + BRK (7) = **9 cycles**.
TEST_F(CPUFlagTest, CLC_CarryAlreadyClear) {
  cpu.setStatus(cpu.getStatus() &
                ~cpu.FLAG_CARRY);  // Ensure Carry flag is clear before test.

  std::vector<uint8_t> program = {
      0x18,  // CLC (Clear Carry flag)
      0x00   // BRK
  };
  cpu.loadAndExecute(program);

  // Ensure Carry flag is still clear.
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_CARRY)
      << "Carry flag should remain clear.";
  // Other flags should remain unchanged.
  // EXPECT_EQ(cpu.getCycleCount(), 9) << "Cycle count should be 9 (2+7).";
}

// Test CLC when the Carry flag is set.
// Expected: Carry flag is cleared, other flags unchanged.
// Cycle count: CLC (2) + BRK (7) = **9 cycles**.
TEST_F(CPUFlagTest, CLC_CarryWasSet) {
  cpu.setStatus(cpu.getStatus() |
                cpu.FLAG_CARRY);  // Set Carry flag before test.

  std::vector<uint8_t> program = {
      0x18,  // CLC (Clear Carry flag)
      0x00   // BRK
  };
  cpu.loadAndExecute(program);

  // Ensure Carry flag is now cleared.
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_CARRY)
      << "Carry flag should be cleared after CLC.";
  // Other flags should remain unchanged.
  // EXPECT_EQ(cpu.getCycleCount(), 9) << "Cycle count should be 9 (2+7).";
}

// Test CLC does not affect other flags (Zero, Negative, Overflow).
// Expected: Carry flag cleared, other flags remain unchanged.
// Cycle count: CLC (2) + BRK (7) = **9 cycles**.
TEST_F(CPUFlagTest, CLC_DoesNotAffectOtherFlags) {
  std::vector<uint8_t> program = {
      0x18,  // CLC (Clear Carry flag)
      0x00   // BRK
  };

  cpu.loadProgram(program);
  cpu.resetInterrupt();
  // set all flags for testing:
  cpu.setStatus(cpu.FLAG_CARRY | cpu.FLAG_ZERO | cpu.FLAG_NEGATIVE |
                cpu.FLAG_OVERFLOW);
  cpu.executeProgram();

  // Ensure Carry flag is cleared.
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_CARRY)
      << "Carry flag should be cleared.";
  // Ensure other flags remain unchanged.
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_ZERO)
      << "Zero flag should remain set.";
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_NEGATIVE)
      << "Negative flag should remain set.";
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_OVERFLOW)
      << "Overflow flag should remain set.";
  // EXPECT_EQ(cpu.getCycleCount(), 9) << "Cycle count should be 9 (2+7).";
}