#include <gtest/gtest.h>

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "../../include/NES.h"

std::vector<uint8_t> readNestestROM() {
  char *filename = "../tests/CPU/nestest.nes";
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file: " + std::string(filename));
  }

  // read rom file into vector
  std::vector<uint8_t> romDump((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());

  return romDump;
}

TEST(CPUNestest, runNestest) {

  std::vector<uint8_t> romDump = readNestestROM();
  NES nes = NES(romDump);  // instantiate a virtual NES console
  
  nes.cpu.triggerRES();
  for (int i=0; i<7; i++) {
    nes.cpu.tick();
  }

  nes.cpu.TEST_setPC(0xC000);
  // up to 0xC6B3 -> official instructions
  // up to 0xC66E -> all instructions
  while (nes.cpu.TEST_getPC() != 0xC66E) {
    nes.cpu.tick();
  }
}

// main entry point for the tests.
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}