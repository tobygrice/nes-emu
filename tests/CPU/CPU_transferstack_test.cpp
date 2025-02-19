#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU.h"
#include "../../include/OpCode.h"
#include "../../include/TestBus.h"

// Define a test fixture for CPU tests.
class CPUTransferStackTest : public ::testing::Test {
 protected:
  TestBus bus;  // create a shared bus
  CPU cpu;      // CPU instance that uses the shared bus

  CPUTransferStackTest() : bus(), cpu(&bus) {}
};

TEST_F(CPUTransferStackTest, PHATest) {
  std::vector<uint8_t> program = {0xA9, 0b01101110,  // LDA 0b01101110
                                  0x48,              // PHA
                                  0x00, 0x00};       // BRK
  cpu.loadProgram(program);
  cpu.resetInterrupt();
  uint8_t initialSP = cpu.getSP();
  cpu.executeProgram();
  EXPECT_EQ(cpu.getSP(), initialSP - 4);  // BRK pushes 3 bytes to stack
  EXPECT_EQ(cpu.memRead8(0x0100 | initialSP), 0b01101110);  // manual read
}

TEST_F(CPUTransferStackTest, PLATest) {
  std::vector<uint8_t> program = {0x68,         // PLA
                                  0x00, 0x00};  // BRK
  cpu.loadProgram(program);
  cpu.resetInterrupt();
  cpu.push(0b01101110);
  cpu.executeProgram();

  EXPECT_EQ(cpu.getSP(), 0xFF - 3);  // BRK pushes 3 bytes to stack
  EXPECT_EQ(cpu.getA(), 0b01101110) << "A register should be loaded from stack";
}
