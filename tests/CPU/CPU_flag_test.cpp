#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/CPU.h"
#include "../../include/OpCode.h"

// Define a test fixture for CPU tests.
class CPUFlagTest : public ::testing::Test {
 protected:
  CPU cpu;
};