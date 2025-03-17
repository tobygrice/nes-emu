#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU/CPU.h"
#include "../../include/CPU/OpCode.h"
#include "../../include/TestBus.h"
#include "../../include/Logger.h"

// Define a test fixture for CPU tests.
class CPUCompareTest : public ::testing::Test {
 protected:
  TestBus bus;  // create a shared bus
  CPU cpu;  // CPU instance that uses the shared bus
  Logger logger;

  CPUCompareTest() : bus(), cpu(&bus, &logger) {}
};