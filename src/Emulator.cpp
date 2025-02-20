#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/CPU.h"
#include "../include/Logger.h"
#include "../include/MMU.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    throw std::invalid_argument(
        "Expected single argument specifiying name of .nes file");
  }

  std::ifstream file(argv[1], std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file: " + std::string(argv[1]));
  }

  // read rom file into vector
  std::vector<uint8_t> raw((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

  Logger logger = Logger();
  MMU mmu = MMU(raw);            // instantiate memory management unit (bus)
  CPU cpu = CPU(&mmu, &logger);  // instantiate CPU and provide bus

  cpu.resetInterrupt();
  cpu.setPC(0xC000);  // overwrite reset vector for incomplete emulators
  cpu.executeProgram();

  return 0;
}