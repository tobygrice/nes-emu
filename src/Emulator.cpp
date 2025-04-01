#include <SDL3/SDL_main.h>

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "../include/NES.h"
#include "../include/Renderer/Renderer.h"

std::vector<uint8_t> readROM(char *filename) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open file: " + std::string(filename));
  }

  // read rom file into vector
  std::vector<uint8_t> romDump((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());

  return romDump;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    throw std::invalid_argument(
        "Expected single argument specifiying name of .nes file");
  }

  std::vector<uint8_t> romDump = readROM(argv[1]);  // read ROM from file
  NES nes = NES();  // instantiate a virtual NES console
  // load cartridge (triggers reset interrupt on CPU)
  nes.insertCartridge(romDump);
  nes.start();

  return 0;
}