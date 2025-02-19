#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU.h"
#include "../../include/OpCode.h"
#include "../../include/TestBus.h"

// Define a test fixture for CPU tests.
class CPUTest : public ::testing::Test {
 protected:
  TestBus bus;  // create a shared bus
  CPU cpu;  // CPU instance that uses the shared bus

  CPUTest() : bus(), cpu(&bus) {}
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
  EXPECT_TRUE(cpu.getStatus() & cpu.FLAG_INTERRUPT);
  EXPECT_FALSE(cpu.getStatus() & cpu.FLAG_DECIMAL);
}

// Main entry point for the tests.
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}