#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU/CPU.h"
#include "../../include/CPU/OpCode.h"
#include "../../include/TestBus.h"
#include "../../include/Logger.h"

// Define a test fixture for CPU tests.
class CPUTransferStackTest : public ::testing::Test {
 protected:
  TestBus bus;  // create a shared bus
  CPU cpu;      // CPU instance that uses the shared bus
  Logger logger;

  CPUTransferStackTest() : bus(), cpu(&bus, &logger) {}
};

TEST_F(CPUTransferStackTest, PHATest) {
  std::vector<uint8_t> program = {0xA9, 0b01101110,  // LDA 0b01101110
                                  0x48,              // PHA
                                  0x00, 0x00};       // BRK
  cpu.loadProgram(program);
  cpu.in_RESET();
  uint8_t initialSP = cpu.getSP();
  cpu.executeProgram();
  EXPECT_EQ(cpu.getSP(), initialSP - 4);  // BRK pushes 3 bytes to stack
  EXPECT_EQ(cpu.memRead8(0x0100 | initialSP), 0b01101110);  // manual read
}

TEST_F(CPUTransferStackTest, PLATest) {
  std::vector<uint8_t> program = {0x68,         // PLA
                                  0x00, 0x00};  // BRK
  cpu.loadProgram(program);
  cpu.in_RESET();
  cpu.push(0b01101110);
  cpu.executeProgram();

  // stack starts at 0xFD after resetInterrupt
  EXPECT_EQ(cpu.getSP(), 0xFD - 3);  // BRK pushes 3 bytes to stack
  EXPECT_EQ(cpu.getA(), 0b01101110) << "A register should be loaded from stack";
}
